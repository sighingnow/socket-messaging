/**
 * Copyright: sighingnow@gmail.com
 *
 * Echo client using SSL.
 *
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
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

    SSL_CTX *ctx;
    SSL *ssl;
    X509 *cert = NULL;

    char server_ip[INET_ADDRSTRLEN];
    uint16_t server_port = ntohs(server.sin_port);

    inet_ntop(AF_INET, &(server.sin_addr), server_ip, INET_ADDRSTRLEN);
    logging("Establish connection with server %s:%d.\n", server_ip, server_port);

    // Create SSL context and connection.
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    if ((ctx = SSL_CTX_new(SSLv23_client_method())) == NULL) {
        exception("Failed to craete SSL CTX using SSLv23_client_method.\n");
    }
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, server_fd);
    if (SSL_connect(ssl) < 0) {
        exception("Failed to establish SSL connection.\n");
    }
    logging("Establish SSL connection with %s encryption.\n", SSL_get_cipher(ssl));

    // Display certificate information.
    if ((cert = SSL_get_peer_certificate(ssl)) == NULL) {
        exception("Failed to get peer certificate.\n");
    }
    logging("Server certificate subject: %s.\n",
            X509_NAME_oneline(X509_get_subject_name(cert), 0, 0));
    logging("Server certificate issuer: %s.\n",
            X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0));
    X509_free(cert);

    // SSL echo communication.
    while (fgets((char *)buf, BUFFER, stdin) != NULL) {
        if (SSL_write(ssl, buf, strlen((char const *)buf) + 1) < 0) {
            exception("Failed to send echo to SSL server.\n");
        }
        if ((read_size = SSL_read(ssl, buf, BUFFER)) < 0) {
            exception("Failed to receive from SSL server %s:%d.\n", server_ip, server_port);
        }
        if (read_size == 0) {
            break;
        }
        buf[read_size] = '\0';
        logging("from SSL server %s:%d %s\n", server_ip, server_port, buf);
    }
    if (read_size < 0) {
        exception("Failed to read data from server.");
    }
    close(server_fd);

    // Shutdown SSL and release resource.
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
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
        exception("Failed to use invalid ip address %s.\n", target);
    }

    // Connect to server.
    if (connect(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        exception("Failed to establish connection to server %s:%d.\n", target, port);
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
