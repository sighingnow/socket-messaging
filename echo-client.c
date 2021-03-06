/**
 * Copyright: sighingnow@gmail.com
 *
 * Echo client using TCP.
 *
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

const int BUFFER = 1024;

void usage(char const *name) { fprintf(stderr, "Usage: %s ip port\n", name); }

#define exception(...)                \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        exit(1);                      \
    } while (0)
#define logging(...)                  \
    do {                              \
        fprintf(stdout, __VA_ARGS__); \
    } while (0)

void handle(int32_t server_fd, struct sockaddr_in server) {
    int32_t read_size = -1;
    uint8_t buf[BUFFER];
    char server_ip[INET_ADDRSTRLEN];
    uint16_t server_port = ntohs(server.sin_port);
    inet_ntop(AF_INET, &(server.sin_addr), server_ip, INET_ADDRSTRLEN);
    logging("Establish connection with server %s:%d\n", server_ip, server_port);
    while (fgets((char *)buf, BUFFER, stdin) != NULL) {
        if (send(server_fd, buf, strlen((char const *)buf) + 1, 0) < 0) {
            exception("Failed to send echo to server.\n");
        }
        if ((read_size = recv(server_fd, buf, BUFFER, 0)) < 0) {
            exception("Failed to receive from server %s:%d\n", server_ip, server_port);
        }
        logging("from server %s:%d %s\n", server_ip, server_port, buf);
    }
    logging("Client are going to stop connection\n");
    close(server_fd);
}

void build_client(char const *target, uint16_t port) {
    int32_t server_fd;
    struct sockaddr_in server;
    // Create socket.
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        exception("Failed to create socket.\n");
    }
    memset(&server, 0x00, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if (inet_pton(AF_INET, target, &server.sin_addr.s_addr) < 0) {
        exception("Failed to use invalid ip address %s\n", target);
    }

    // Connect to server.
    if (connect(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        exception("Failed to establish connection to server %s:%d\n", target, port);
    }

    // Handle.
    handle(server_fd, server);

    // Bind with given port and start listening.
    logging("Connection closed.\n");
}

int main(int argc, char **argv) {
    // Get port from command-line arguments.
    if (argc != 3) {
        usage(argv[0]);
        return 0;
    }

    // Build TCP socket client.
    build_client(argv[1], atoi(argv[2]));

    return 0;
}
