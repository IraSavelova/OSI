#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define ERROR -1
#define MIN_PORT 0
#define MAX_PORT 65535

int main(int argc, char **argv)
{
    int socket_fd, port, msg_len;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char *hostname;
    char buffer[BUFFER_SIZE];

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    hostname = argv[1];
    port = atoi(argv[2]);
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
    server = gethostbyname(hostname);
    if (server == NULL)
    {
        close(socket_fd);
        fprintf(stderr, "Error, this host %s isn't exist \n", hostname);
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(port);
    while (1)
    {
        printf("Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        msg_len = sendto(socket_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (msg_len == ERROR)
        {
            close(socket_fd);
            perror("Sendto error");
            return EXIT_FAILURE;
        }
        memset(&buffer, 0, BUFFER_SIZE);
        socklen_t serverlen = sizeof(server_addr);
        msg_len = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &serverlen);
        if (msg_len == ERROR)
        {
            close(socket_fd);
            perror("Recvfrom error");
            return EXIT_FAILURE;
        }
        buffer[msg_len] = '\0';
        printf("Server reply: %s\n", buffer);
    }
    close(socket_fd);
    return EXIT_SUCCESS;
}
