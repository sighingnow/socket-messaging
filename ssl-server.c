/**
 * Copyright: sighingnow@gmail.com
 *
 * Echo server using SSL.
 *
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <pthread.h>
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

struct thread_args {
    int32_t client_fd;
    struct sockaddr_in client;
    SSL_CTX *ctx;
};

void *handle(void *args) {
    int32_t client_fd = ((struct thread_args *)args)->client_fd;
    struct sockaddr_in client = ((struct thread_args *)args)->client;
    SSL_CTX *ctx = ((struct thread_args *)args)->ctx;
    free(args);

    SSL *ssl;
    X509 *cert = NULL;

    int32_t read_size = -1;
    uint8_t buf[BUFFER];
    char client_ip[INET_ADDRSTRLEN];
    uint16_t client_port = ntohs(client.sin_port);
    inet_ntop(AF_INET, &(client.sin_addr), client_ip, INET_ADDRSTRLEN);
    logging("Establish connection with client %s:%d.\n", client_ip, client_port);

    // Create SSL connection.
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_fd);
    if (SSL_accept(ssl) < 0) {
        exception("Failed to establish SSL connection.\n");
    }
    logging("Establish SSL connection with %s encryption.\n", SSL_get_cipher(ssl));

    // Display certificate information.
    if ((cert = SSL_get_peer_certificate(ssl)) == NULL) {
        // exception("Failed to get peer certificate.\n");
        logging("No certificate from client.\n");
    } else {
        logging("Server certificate subject: %s.\n",
                X509_NAME_oneline(X509_get_subject_name(cert), 0, 0));
        logging("Server certificate issuer: %s.\n",
                X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0));
        X509_free(cert);
    }

    while ((read_size = SSL_read(ssl, buf, BUFFER)) > 0) {
        // handle...
        logging("from %s:%d %s\n", client_ip, client_port, buf);
        if (SSL_write(ssl, buf, read_size) < 0) {
            exception("Failed to send echo response to client %s:%d.\n", client_ip, client_port);
        }
    }
    if (read_size < 0) {
        // abnormal close.
        logging("Close connection with client %s:%d abnormally, read_size: "
                "%d.\n",
                client_ip, client_port, read_size);
    } else {
        // normal close.
        logging("Close connection with client %s:%d.\n", client_ip, client_port);
    }
    close(client_fd);
    // Shutdown SSL connection and release resource.
    SSL_shutdown(ssl);
    SSL_free(ssl);
    pthread_exit(NULL);
}

void build_server(uint16_t port, const char *cert, const char *private_key) {
    pthread_t tid;
    int32_t server_fd, client_fd;
    struct sockaddr_in server, client;
    struct thread_args *args = NULL;

    SSL_CTX *ctx;

    // Create socket.
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        exception("Failed to create socket.\n");
    }
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind with given port and start listening.
    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        exception("Failed to bind socket at port %d.\n", port);
    }
    if (listen(server_fd, 1024) < 0) {
        exception("Failed to listen on port %d.\n", port);
    }
    logging("Listening start on port %d....\n", port);

    // Create SSL context.
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    if ((ctx = SSL_CTX_new(SSLv23_server_method())) == NULL) {
        exception("Failed to craete SSL CTX using SSLv23_client_method.\n");
    }

    // Load certificate and private key.
    if (SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) < 0) {
        logging("Failed to load certificate file.\n");
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, private_key, SSL_FILETYPE_PEM) < 0) {
        logging("Failed to load private key file.\n");
    }
    if (!SSL_CTX_check_private_key(ctx)) {
        exception("Private key does not match the public certificate.\n");
    }

    // Echo SSL listen loop.
    while (1) {
        socklen_t client_size = sizeof(client);
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client, &client_size)) < 0) {
            exception("Failed to establish connection with client.\n");
        }
        args = (struct thread_args *)malloc(sizeof(struct thread_args));
        args->client_fd = client_fd;
        args->client = client;
        args->ctx = ctx;
        if (pthread_create(&tid, NULL, handle, args) < 0) {
            exception("Failed to create thread.\n");
        }
        pthread_detach(tid);
    }
    SSL_CTX_free(ctx);
    close(server_fd);
    logging("Listening stop.\n");
}

int main(int argc, char **argv) {
    // Get port from command-line arguments.
    if (argc != 2) {
        usage(argv[0]);
        return 0;
    }

    // Check if the root permission is given.
    if (getuid() != 0) {
        exception("The program must be run with root/sudo permission.\n");
    }

    // Build TCP socket listener.
    build_server(atoi(argv[1]), "./cacert.pem", "./private-key.pem");
    // load_certificate(ctx, "./cacert.pem", "./private-key.pem");

    return 0;
}
