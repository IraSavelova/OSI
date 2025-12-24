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
#include <stdatomic.h>
#include <time.h>

#define BUFFER_SIZE 8192
#define PORT 80
#define ERROR -1
#define SUCCESS 0
#define CLOSED 1
#define NOT_CLOSED 0
#define DEFAULT_PROTOCOL 0
#define BEFORE_HOST_FLAG 0
#define AFTER_HOST_FLAG 1
atomic_ulong total_connections = 0;
atomic_ulong active_connections = 0;
atomic_ulong closed_connections = 0;
atomic_ulong error_connections = 0;
atomic_ulong thread_created = 0;
volatile sig_atomic_t stop_proxy = 0;
time_t start_time;
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
    stop_proxy = 1;
    if (handler_data.socket_ptr != NULL && *handler_data.socket_ptr != ERROR)
    {
        close(*handler_data.socket_ptr);
        *handler_data.socket_ptr = ERROR;
    }
}

void *monitor_thread(void *arg)
{
    (void)arg;
    while (1)
    {
        sleep(10);
        printf("\n===== PROXY MONITOR =====\n");
        printf("Uptime: %ld sec\n", time(NULL) - start_time);
        printf("Active connections: %lu\n", atomic_load(&active_connections));
        printf("Total connections:  %lu\n", atomic_load(&total_connections));
        printf("Closed connections: %lu\n", atomic_load(&closed_connections));
        printf("Errors:             %lu\n", atomic_load(&error_connections));
        printf("Created threads:           %lu\n", atomic_load(&thread_created));
        printf("=========================\n");
    }
    return NULL;
}

int get_content_length(char *buffer, int buffer_size)
{
    char *cl = strcasestr(buffer, "Content-Length:");
    if (cl == NULL)
        return ERROR; // Нет Content-Length

    cl += strlen("Content-Length:");
    while (*cl == ' ' && (cl - buffer) < buffer_size)
        cl++;

    int content_length = 0;
    sscanf(cl, "%d", &content_length);
    return content_length;
}

int get_host(char *request, char *resolved_host, int flag)
{
    char *host_start;
    const char *host_end;
    if (flag == BEFORE_HOST_FLAG)
    {
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
    }
    if (flag == AFTER_HOST_FLAG)
    {
        host_start = strstr(request, "Host:");
        if (host_start == NULL)
        {
            return ERROR;
        }
        host_start += strlen("Host:");

        while (*host_start == ' ')
        {
            host_start++;
        }

        host_end = strpbrk(host_start, " \r\n");
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
    free(arg);
    atomic_fetch_add(&active_connections, 1);

    char buffer[BUFFER_SIZE], host[BUFFER_SIZE];
    int bytes_read;
    int target_socket = ERROR;
    struct sockaddr_in target_addr;
    struct hostent *he;

    int client_closed = NOT_CLOSED;
    int server_closed = NOT_CLOSED;

    int content_length = -1;
    int header_processed = 0;
    int body_bytes_received = 0;

    /* ===== Читаем запрос клиента ===== */
    bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_read == ERROR)
        goto error;

    if (get_host(buffer, host, BEFORE_HOST_FLAG) != 0)
        if (get_host(buffer, host, AFTER_HOST_FLAG) != 0)
            goto error;

    he = gethostbyname(host);
    if (he == NULL)
        goto error;

    target_socket = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
    if (target_socket == ERROR)
        goto error;

    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(PORT);
    memcpy(&target_addr.sin_addr, he->h_addr_list[0], he->h_length);
    int err = connect(target_socket, (struct sockaddr *)&target_addr, sizeof(target_addr));
    if (err == ERROR)
        goto error;

    send(target_socket, buffer, bytes_read, 0);

    fd_set readfds;
    int max_fd = (client_socket > target_socket) ? client_socket : target_socket;

    /* ===== ОСНОВНОЙ ЦИКЛ ===== */
    while (!client_closed || !server_closed)
    {
        FD_ZERO(&readfds);
        if (!client_closed)
            FD_SET(client_socket, &readfds);
        if (!server_closed)
            FD_SET(target_socket, &readfds);

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) == ERROR)
            goto error;

        /* ===== ОТ КЛИЕНТА К СЕРВЕРУ ===== */
        if (!client_closed && FD_ISSET(client_socket, &readfds))
        {
            bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_read == ERROR)
            {
                client_closed = CLOSED;
                shutdown(target_socket, SHUT_WR);
            }
            send(target_socket, buffer, bytes_read, 0);
        }

        /* ===== ОТ СЕРВЕРА К КЛИЕНТУ ===== */
        if (!server_closed && FD_ISSET(target_socket, &readfds))
        {
            bytes_read = recv(target_socket, buffer, BUFFER_SIZE, 0);

            if (bytes_read == 0)
            {
                server_closed = CLOSED;
                shutdown(client_socket, SHUT_WR);
                continue;
            }
            if (bytes_read == ERROR)
                goto error;

            /* ---- Парсинг заголовков ---- */
            if (!header_processed)
            {
                int header_end = 0;
                for (int i = 0; i < bytes_read - 3; i++)
                {
                    if (buffer[i] == '\r' && buffer[i + 1] == '\n' &&
                        buffer[i + 2] == '\r' && buffer[i + 3] == '\n')
                    {
                        header_end = i + 4;
                        break;
                    }
                }

                if (header_end > 0)
                {
                    content_length = get_content_length(buffer, header_end);
                    header_processed = 1;

                    if (content_length > 0)
                        body_bytes_received += bytes_read - header_end;
                }
            }
            send(client_socket, buffer, bytes_read, 0);

            /* ---- Условие завершения ---- */
            if (content_length > 0 && body_bytes_received >= content_length)
            {
                shutdown(client_socket, SHUT_WR);
                shutdown(target_socket, SHUT_WR);
                server_closed = CLOSED;
                client_closed = CLOSED;
            }
        }
    }

done:
    if (client_socket != ERROR)
        close(client_socket);
    if (target_socket != ERROR)
        close(target_socket);

    atomic_fetch_add(&closed_connections, 1);
    atomic_fetch_sub(&active_connections, 1);
    return NULL;

error:
    atomic_fetch_add(&error_connections, 1);
    goto done;
}

int main(void)
{
    start_time = time(NULL);
    int proxy_socket = ERROR;

    // Инициализируем структуру данных
    handler_data.socket_ptr = &proxy_socket;

    // Устанавливаем обработчик
    signal(SIGINT, sigint_handler);
    struct sockaddr_in proxy_addr;
    pthread_t thread_id;
    pthread_t mon;
    pthread_create(&mon, NULL, monitor_thread, NULL);
    pthread_detach(mon);
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

    while (!stop_proxy)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_socket = accept(proxy_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        printf("[MAIN] Accepted socket %d\n", client_socket);
        if (client_socket == ERROR)
        {
            perror("Error accepting connection");
            if (stop_proxy)
                break;
            continue;
        }
        int *socket_for_thread = malloc(sizeof(int));
        if (socket_for_thread == NULL)
        {
            perror("malloc failed");
            close(client_socket);
            continue;
        }
        *socket_for_thread = client_socket;
        atomic_fetch_add(&total_connections, 1);
        int ptr_create = pthread_create(&thread_id, NULL, handle_client, (void *)socket_for_thread);
        if (ptr_create == ERROR)
        {
            perror("Error creating thread");
            close(client_socket);
            continue;
        }
        atomic_fetch_add(&thread_created, 1);
        pthread_detach(thread_id);
    }
    close(proxy_socket);
    return EXIT_SUCCESS;
}
