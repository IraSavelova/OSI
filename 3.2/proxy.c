#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/select.h>

#define BUFFER_SIZE 8192
#define PORT 80
#define ERROR -1
#define SUCCESS 0
#define CLOSED 1
#define NOT_CLOSED 0
#define DEFAULT_PROTOCOL 0

// Структура для передачи данных
typedef struct
{
    int *socket_ptr;
} handler_data_t;

// Статическая переменная для доступа из обработчика
static handler_data_t handler_data;

void sigint_handler(int sig)
{
    (void)sig;
    printf("\nSIGINT received\n");
    if (handler_data.socket_ptr != NULL && *handler_data.socket_ptr != ERROR)
    {
        close(*handler_data.socket_ptr);
        *handler_data.socket_ptr = ERROR;
    }
    exit(EXIT_SUCCESS);
}

int get_host(char *request, char *resolved_host)
{
    char *host_start;
    const char *host_end;
    host_start = strstr(request, "http://");
    if (host_start == NULL)
    {
        return ERROR;
    }
    host_start += strlen("http://");
    host_end = strchr(host_start, '/');
    if (host_end == NULL)
    {
        host_end = strchr(host_start, ' ');
    }

    if (host_end == NULL)
    {
        host_end = host_start + strlen(host_start);
    }

    size_t host_length = host_end - host_start;

    if (host_length >= BUFFER_SIZE)
    {
        return ERROR;
    }

    if (host_length <= 0)
    {
        return ERROR;
    }

    strncpy(resolved_host, host_start, host_length);
    resolved_host[host_length] = '\0';
    return SUCCESS;
}

void *handle_client(void *arg)
{
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE], host[BUFFER_SIZE];
    int bytes_read;
    int target_socket;
    struct sockaddr_in target_addr;
    struct hostent *he;
    // Чтение HTTP-запроса от клиента
    bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_read == ERROR)
    {
        perror("Error reading client request");
        close(client_socket);
        return NULL;
    }

    int host_found = get_host(buffer, host);
    if (host_found != SUCCESS)
    {
        close(client_socket);
        return NULL;
    }

    printf("Host: %s\n", host);

    // преобразует доменное имя в IP-адрес
    he = gethostbyname(host);
    if (he == NULL)
    {
        perror("gethostbyname");
        close(client_socket);
        return NULL;
    }
    // Создание сокета для целевого сервера
    target_socket = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
    if (target_socket == ERROR)
    {
        perror("Error creating server socket");
        close(client_socket);
        return NULL;
    }

    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(PORT);
    memcpy(&target_addr.sin_addr, he->h_addr_list[0], he->h_length);
    // подключение к целевому серверу
    int err = connect(target_socket, (struct sockaddr *)&target_addr, sizeof(target_addr));
    if (err == ERROR)
    {
        perror("Error connecting to target server");
        close(client_socket);
        close(target_socket);
        return NULL;
    }
    // Отправляет оригинальный HTTP-запрос дальше
    err = send(target_socket, buffer, bytes_read, 0);
    if (err == ERROR)
    {
        perror("Error send to target server");
        close(client_socket);
        close(target_socket);
        return NULL;
    }
    fd_set readfds; // Набор дескрипторов для отслеживания чтения
    int max_fd = (client_socket > target_socket) ? client_socket : target_socket;
    int client_closed = NOT_CLOSED;
    int server_closed = NOT_CLOSED;

    while (!client_closed && !server_closed) // пока оба соединения открыты
    {
        FD_ZERO(&readfds); // Очистка набора (все биты в 0)
        if (!client_closed)
            FD_SET(client_socket, &readfds);
        if (!server_closed)
            FD_SET(target_socket, &readfds);

        err = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (err == ERROR)
        {
            perror("Error select");
            close(client_socket);
            close(target_socket);
            return NULL;
        }

        if (!client_closed && FD_ISSET(client_socket, &readfds))
        {
            bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_read == 0)
            {
                client_closed = CLOSED;
                perror("Client closed connection\n");
                continue;
            }

            if (bytes_read == ERROR)
            {
                client_closed = CLOSED;
                perror("Error reading from client\n");
                continue;
            }

            int total_sent = 0;
            while (total_sent < bytes_read)
            {
                int bytes_sent = send(target_socket, buffer + total_sent, bytes_read - total_sent, 0);
                if (bytes_sent <= 0)
                {
                    server_closed = CLOSED;
                    perror("Error sending to server");
                    break;
                }
                total_sent += bytes_sent;
            }
        }

        if (!server_closed && FD_ISSET(target_socket, &readfds))
        {
            // чтение данных от клиента в буфер
            bytes_read = recv(target_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_read == 0)
            {
                server_closed = CLOSED;
                perror("Server closed connection\n");
                continue;
            }

            if (bytes_read == ERROR)
            {
                server_closed = CLOSED;
                perror("Error reading from server\n");
                continue;
            }

            int total_sent = 0;
            // отправка данных серверу
            while (total_sent < bytes_read)
            {
                int bytes_sent = send(client_socket, buffer + total_sent, bytes_read - total_sent, 0);
                if (bytes_sent == ERROR)
                {
                    client_closed = CLOSED;
                    perror("Error sending to client");
                    break;
                }
                total_sent += bytes_sent;
            }
        }
    }
    close(client_socket);
    close(target_socket);
    return NULL;
}

int main(void)
{
    int proxy_socket = ERROR;

    // Инициализируем структуру данных
    handler_data.socket_ptr = &proxy_socket;

    // Устанавливаем обработчик
    signal(SIGINT, sigint_handler);
    struct sockaddr_in proxy_addr;
    pthread_t thread_id;

    proxy_socket = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
    if (proxy_socket == ERROR)
    {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    memset(&proxy_addr, 0, sizeof(proxy_addr));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = INADDR_ANY;
    proxy_addr.sin_port = htons(PORT);
    int bind_res = bind(proxy_socket, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr));
    if (bind_res == ERROR)
    {
        perror("Error binding socket");
        close(proxy_socket);
        return EXIT_FAILURE;
    }
    // Socket Operation MAX CONNections
    int list_res = listen(proxy_socket, SOMAXCONN);
    if (list_res == ERROR)
    {
        perror("Error listening for connections");
        close(proxy_socket);
        return EXIT_FAILURE;
    }

    printf("HTTP proxy server listening on port 80\n");

    while (true)
    {
        int client_socket;
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        client_socket = accept(proxy_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == ERROR)
        {
            perror("Error accepting connection");
            continue;
        }
        int ptr_create = pthread_create(&thread_id, NULL, handle_client, (void *)&client_socket);
        if (ptr_create == ERROR)
        {
            perror("Error creating thread");
            close(client_socket);
            continue;
        }
        pthread_detach(thread_id);
    }
    close(proxy_socket);
    return EXIT_SUCCESS;
}
