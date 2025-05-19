#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define ERROR -1
#define MIN_PORT 0
#define MAX_PORT 65535
#define BLOCK_FLAG 0

int main(int argc, char **argv)
{
    int socket_fd, port, msg_len;
    socklen_t client_length;
    char buf[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    port = atoi(argv[1]);
    if (port <= MIN_PORT || port > MAX_PORT)
    {
        fprintf(stderr, "Invalid port number\n");
        return EXIT_FAILURE;
    }
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == ERROR)
    {
        perror("Socket open error");
        return EXIT_FAILURE;
    }
    int optval = 1;
    int set_sock = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
    if (set_sock == ERROR)
    {
        perror("Error setsockport");
        return EXIT_FAILURE;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int bind_sock = bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (bind_sock == ERROR)
    {
        close(socket_fd);
        perror("Bind error");
        return EXIT_FAILURE;
    }

    client_length = sizeof(client_addr);
    while (1)
    {
        msg_len = recvfrom(socket_fd, buf, BUFFER_SIZE, BLOCK_FLAG, (struct sockaddr *)&client_addr, &client_length);
        if (msg_len == ERROR)
        {
            close(socket_fd);
            perror("Recvfrom error");
            return EXIT_FAILURE;
        }

        msg_len = sendto(socket_fd, buf, msg_len, BLOCK_FLAG, (struct sockaddr *)&client_addr, client_length);
        if (msg_len == ERROR)
        {
            close(socket_fd);
            perror("Sendto error");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
