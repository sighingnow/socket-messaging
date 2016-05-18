/**
 * Copyright: sighingnow@gmail.com
 *
 * Echo client using TCP.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const int BUFFER = 1024;

void usage(char const *name) {
    fprintf(stderr, "Usage: %s ip port\n", name);
}

#define exception(...) do { fprintf(stderr, __VA_ARGS__); exit(1); } while (0)
#define logging(...) do { fprintf(stdout, __VA_ARGS__); } while (0)

void handle(int32_t client_fd) {
    int32_t read_size = -1;
    uint8_t buf[BUFFER];
    while (scanf("%s", buf) != EOF) {
        if (send(client_fd, buf, strlen((const char *)buf)+1, 0) < 0) {
            exception("Failed to send echo to server.\n");
        }
    }
    if (read_size < 0) {
        exception("Failed to read data from client.");
    }
    close(client_fd);
}

void build_client(const char *target, uint16_t port) {
    int32_t client_fd;
    struct sockaddr_in server;
    // Create socket.
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        exception("Failed to create socket.\n");
    }
    memset(&server, 0x00, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if (inet_pton(AF_INET, target, &server.sin_addr.s_addr) < 0) {
        exception("Failed to use invalid ip address %s\n", target);
    }

    // Connect to server.
    if (connect(client_fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        exception("Failed to establish connection to server %s:%d\n", target, port);
    }

    // Handle.
    handle(client_fd);

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

