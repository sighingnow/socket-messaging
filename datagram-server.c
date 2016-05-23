/**
 * Copyright: sighingnow@gmail.com
 *
 * Echo server using UDP.
 *
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

const int BUFFER = 1024;

void usage(char const *name) { fprintf(stderr, "Usage: %s port\n", name); }

#define exception(...)                \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        exit(1);                      \
    } while (0)
#define logging(...)                  \
    do {                              \
        fprintf(stdout, __VA_ARGS__); \
    } while (0)

void build_server(uint16_t port) {
    int32_t server_fd;
    struct sockaddr_in server, client;
    socklen_t client_size = -1;
    int32_t read_size = -1;
    uint8_t buf[BUFFER];
    char client_ip[INET_ADDRSTRLEN];
    uint16_t client_port;

    // Create socket.
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        exception("Failed to create UDP socket.\n");
    }
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind with given port and start listening.
    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        exception("Failed to bind socket at port %d.\n", port);
    }
    logging("Bind UDP listener on port %d....\n", port);

    while (1) {
        client_size = sizeof(client);
        if ((read_size = recvfrom(server_fd, buf, BUFFER, 0, (struct sockaddr *)&client,
                                  &client_size)) < 0) {
            exception("Failed to receive data from client.\n");
        }
        buf[read_size] = '\0';
        client_port = ntohs(client.sin_port);
        inet_ntop(AF_INET, &(client.sin_addr), client_ip, INET_ADDRSTRLEN);
        logging("from %s:%d %s\n", client_ip, client_port, buf);
        if (sendto(server_fd, buf, read_size, 0, (struct sockaddr *)&client, client_size) < 0) {
            exception("Failed to send response to client %s:%d.\n", client_ip, client_port);
        }
    }
    close(server_fd);
    logging("UDP Listening stop.\n");
}

int main(int argc, char **argv) {
    // Get port from command-line arguments.
    if (argc != 2) {
        usage(argv[0]);
        return 0;
    }

    // Build UDP socket listener.
    build_server(atoi(argv[1]));

    return 0;
}
