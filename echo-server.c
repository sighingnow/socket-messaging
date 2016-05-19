/**
 * Copyright: sighingnow@gmail.com
 *
 * Echo server using TCP.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

const int BUFFER = 1024;

void usage(char const *name) {
    fprintf(stderr, "Usage: %s port\n", name);
}

#define exception(...) do { fprintf(stderr, __VA_ARGS__); exit(1); } while (0)
#define logging(...) do { fprintf(stdout, __VA_ARGS__); } while (0)

struct thread_args {
    int32_t client_fd;
    struct sockaddr_in client;
};

void *handle(void *args) {
    int32_t client_fd = ((struct thread_args *)args)->client_fd;
    struct sockaddr_in client = ((struct thread_args *)args)->client;
    free(args);
    int32_t read_size = -1;
    uint8_t buf[BUFFER];
    char client_ip[INET_ADDRSTRLEN];
    uint16_t client_port = ntohs(client.sin_port);
    inet_ntop(AF_INET, &(client.sin_addr), client_ip, INET_ADDRSTRLEN);
    logging("Establish connection with client %s:%d\n", client_ip, client_port);
    while ((read_size = recv(client_fd, buf, BUFFER, 0)) > 0) {
        // handle...
        logging("from %s:%d %s\n", client_ip, client_port, buf);
        if (send(client_fd, buf, read_size, 0) < 0) {
            exception("Failed to send echo response to client %s:%d.\n", client_ip, client_port);
        }
    }
    if (read_size < 0) {
        // abnormal close.
        logging("Close connection with client %s:%d abnormally.\n", client_ip, client_port);
        pthread_exit(NULL);
    }
    else {
        // normal close.
        logging("Close connection with client %s:%d.\n", client_ip, client_port);
    }
    close(client_fd);
    return NULL;
}

void build_server(uint16_t port) {
    pthread_t tid;
    int32_t server_fd, client_fd;
    struct sockaddr_in server, client;
    struct thread_args *args = NULL;

    // Create socket.
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        exception("Failed to create socket.\n");
    }
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind with given port and start listening.
    if (bind(server_fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        exception("Failed to bind socket at port %d.\n", port);
    }
    if (listen(server_fd, 1024) < 0) {
        exception("Failed to listen on port %d.\n", port);
    }
    logging("Listening start on port %d....\n", port);

    while (1) {
        socklen_t client_size = sizeof(client);
        if ((client_fd = accept(server_fd, (struct sockaddr*) &client, &client_size)) < 0) {
            exception("Failed to establish connection with client.\n");
        }
        args = (struct thread_args *)malloc(sizeof(struct thread_args));
        args->client_fd = client_fd;
        args->client = client;
        if(pthread_create(&tid, NULL, handle, args) < 0) {
            exception("Failed to create thread.\n");
        }
    }
    logging("Listening stop.\n");
}

int main(int argc, char **argv) {
    // Get port from command-line arguments.
    if (argc != 2) {
        usage(argv[0]);
        return 0;
    }

    // Build TCP socket listener.
    build_server(atoi(argv[1]));

    return 0;
}

