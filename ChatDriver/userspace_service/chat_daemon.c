/*
 * User-space Chat Service Daemon
 * Handles socket communication and crypto operations via kernel module
 * Compile on CentOS 6: gcc -o chat_daemon chat_daemon.c -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include "../include/crypto_module.h"
#include "../include/chat_protocol.h"

#define MAX_CLIENTS 100
#define CRYPTO_DEVICE "/dev/chat_crypto"
#define LOG_FILE "/var/log/chat_daemon.log"

typedef struct {
    uint32_t user_id;
    char username[MAX_USERNAME_LEN];
    int socket_fd;
    struct sockaddr_in addr;
    time_t login_time;
    int is_online;
    pthread_t client_thread;
} client_info_t;

typedef struct {
    uint32_t user_id;
    char username[MAX_USERNAME_LEN];
    char password_hash[20]; /* SHA1 */
} user_db_t;

/* Global state */
static int server_socket = -1;
static int crypto_device = -1;
static client_info_t *clients[MAX_CLIENTS];
static int num_clients = 0;
static pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;
static int running = 1;

/* User database (in-memory for demo) */
static user_db_t users[MAX_CLIENTS];
static int num_users = 0;

/* Logging */
static void log_message(const char *fmt, ...)
{
    va_list args;
    FILE *f;
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);

    va_start(args, fmt);
    
    f = fopen(LOG_FILE, "a");
    if (f) {
        fprintf(f, "[%04i-%02i-%02i %02i:%02i:%02i] ", 
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                tm->tm_hour, tm->tm_min, tm->tm_sec);
        vfprintf(f, fmt, args);
        fprintf(f, "\n");
        fclose(f);
    }

    printf("[%02i:%02i:%02i] ", tm->tm_hour, tm->tm_min, tm->tm_sec);
    vprintf(fmt, args);
    printf("\n");

    va_end(args);
}

/* SHA1 simple implementation (matching kernel) */
static void compute_sha1(const unsigned char *input, unsigned long len,
                        unsigned char *output)
{
    struct sha1_request req;

    if (crypto_device < 0) {
        log_message("ERROR: Crypto device not open");
        return;
    }

    if (len > SHA1_MAX_INPUT) {
        log_message("ERROR: Input too long for SHA1");
        return;
    }

    memcpy(req.input, input, len);
    req.input_len = len;

    if (ioctl(crypto_device, CRYPTO_IOCTL_SHA1_HASH, &req) < 0) {
        log_message("ERROR: SHA1 ioctl failed - %s", strerror(errno));
        return;
    }

    memcpy(output, req.digest, SHA1_DIGEST_SIZE);
}

/* User registration */
static int register_user(const char *username, const char *password)
{
    unsigned char hash[SHA1_DIGEST_SIZE];
    int i;

    if (num_users >= MAX_CLIENTS) {
        return -1; /* Database full */
    }

    /* Check if user already exists */
    for (i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return -1; /* User exists */
        }
    }

    /* Hash password with SHA1 */
    compute_sha1((unsigned char *)password, strlen(password), hash);

    users[num_users].user_id = num_users + 1;
    strncpy(users[num_users].username, username, MAX_USERNAME_LEN - 1);
    memcpy(users[num_users].password_hash, hash, SHA1_DIGEST_SIZE);

    num_users++;

    log_message("User registered: %s (ID: %u)", username, users[num_users-1].user_id);

    return users[num_users - 1].user_id;
}

/* User authentication */
static int authenticate_user(const char *username, const char *password,
                           uint32_t *user_id)
{
    unsigned char hash[SHA1_DIGEST_SIZE];
    int i;

    compute_sha1((unsigned char *)password, strlen(password), hash);

    for (i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            if (memcmp(users[i].password_hash, hash, SHA1_DIGEST_SIZE) == 0) {
                *user_id = users[i].user_id;
                return 0; /* Success */
            }
            return -1; /* Password mismatch */
        }
    }

    return -1; /* User not found */
}

/* Handle DES encryption via kernel module */
static int des_encrypt(const unsigned char *key, const unsigned char *input,
                      unsigned long input_len, unsigned char *output,
                      unsigned long *output_len)
{
    struct des_request req;
    unsigned long padded_len;
    int i, pad_len;

    if (crypto_device < 0) {
        log_message("ERROR: Crypto device not open");
        return -1;
    }

    /* Pad input to 8-byte boundary */
    padded_len = (input_len + 7) & ~7;
    pad_len = padded_len - input_len;

    if (padded_len > MAX_CRYPTO_DATA) {
        return -1;
    }

    memcpy(req.input, input, input_len);
    for (i = input_len; i < padded_len; i++) {
        req.input[i] = pad_len;
    }

    memcpy(req.key, key, DES_KEY_SIZE);
    req.input_len = padded_len;
    req.mode = 0; /* Encrypt */

    if (ioctl(crypto_device, CRYPTO_IOCTL_DES_ENCRYPT, &req) < 0) {
        log_message("ERROR: DES encrypt ioctl failed - %s", strerror(errno));
        return -1;
    }

    *output_len = req.output_len;
    memcpy(output, req.output, req.output_len);

    return 0;
}

static int des_decrypt(const unsigned char *key, const unsigned char *input,
                      unsigned long input_len, unsigned char *output,
                      unsigned long *output_len)
{
    struct des_request req;
    int i, pad_len;

    if (crypto_device < 0) {
        log_message("ERROR: Crypto device not open");
        return -1;
    }

    if (input_len > MAX_CRYPTO_DATA || input_len % 8 != 0) {
        return -1;
    }

    memcpy(req.key, key, DES_KEY_SIZE);
    memcpy(req.input, input, input_len);
    req.input_len = input_len;
    req.mode = 1; /* Decrypt */

    if (ioctl(crypto_device, CRYPTO_IOCTL_DES_DECRYPT, &req) < 0) {
        log_message("ERROR: DES decrypt ioctl failed - %s", strerror(errno));
        return -1;
    }

    /* Remove padding */
    pad_len = req.output[req.output_len - 1];
    if (pad_len > 8 || pad_len == 0) {
        pad_len = 0;
    }

    *output_len = req.output_len - pad_len;
    memcpy(output, req.output, *output_len);

    return 0;
}

/* Send packet to client */
static int send_packet(int socket_fd, packet_header_t *header, void *body,
                      uint16_t body_len)
{
    unsigned char buffer[4096];
    int total_len = sizeof(packet_header_t) + body_len;

    if (total_len > sizeof(buffer)) {
        return -1;
    }

    header->length = body_len;
    memcpy(buffer, header, sizeof(packet_header_t));
    if (body_len > 0) {
        memcpy(buffer + sizeof(packet_header_t), body, body_len);
    }

    if (send(socket_fd, buffer, total_len, 0) < 0) {
        log_message("ERROR: Failed to send packet - %s", strerror(errno));
        return -1;
    }

    return 0;
}

/* Handle client connection thread */
static void* client_thread_func(void *arg)
{
    client_info_t *client = (client_info_t *)arg;
    unsigned char buffer[4096];
    int bytes_read;
    packet_header_t *header;
    int i;

    log_message("Client thread started for %s (ID: %u)",
               client->username, client->user_id);

    while (running && client->socket_fd > 0) {
        bytes_read = recv(client->socket_fd, buffer, sizeof(buffer), 0);

        if (bytes_read <= 0) {
            log_message("Client disconnected: %s", client->username);
            break;
        }

        if (bytes_read < sizeof(packet_header_t)) {
            continue;
        }

        header = (packet_header_t *)buffer;

        /* Process packet based on type */
        switch (header->type) {
            case PKT_MESSAGE: {
                message_pkt_t *msg = (message_pkt_t *)buffer;
                unsigned char decrypted[MAX_MESSAGE_LEN];
                unsigned long decrypted_len;
                
                /* Find receiver */
                pthread_mutex_lock(&client_lock);
                for (i = 0; i < num_clients; i++) {
                    if (clients[i] && clients[i]->user_id == header->receiver_id) {
                       if (des_decrypt((unsigned char *)"\x01\x23\x45\x67\x89\xAB\xCD\xEF",
                                      (unsigned char *)msg->encrypted_body,
                                      header->length,
                                      decrypted,
                                      &decrypted_len) == 0) {

                            /* Send to receiver */
                            msg->header.sender_id = client->user_id;
                            memcpy(msg->encrypted_body, decrypted, decrypted_len);
                            msg->header.length = decrypted_len;
                            send_packet(clients[i]->socket_fd, &msg->header,
                                      msg->encrypted_body, decrypted_len);

                            log_message("Message relayed: %s -> %s",
                                       client->username,
                                      clients[i]->username);
                        }
                        break;
                    }
                }
                pthread_mutex_unlock(&client_lock);
                break;
            }

            case PKT_HEARTBEAT:
                /* Respond to heartbeat */
                send_packet(client->socket_fd, header, NULL, 0);
                break;

            case PKT_DISCONNECT:
                log_message("Client disconnection request: %s", client->username);
                goto exit_thread;
                break;

            default:
                log_message("Unknown packet type: %d from %s",
                           header->type, client->username);
                break;
        }
    }

exit_thread:
    close(client->socket_fd);
    
    pthread_mutex_lock(&client_lock);
    for (i = 0; i < num_clients; i++) {
        if (clients[i] == client) {
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&client_lock);

    log_message("Client thread exiting: %s", client->username);
    free(client);

    return NULL;
}

/* Accept incoming connections */
static void* accept_thread_func(void *arg)
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int client_socket;
    client_info_t *client;
    unsigned char buffer[256];
    int bytes_read;
    register_pkt_t *reg_pkt;
    login_pkt_t *login_pkt;
    packet_header_t *header;
    auth_resp_t auth_resp;
    uint32_t user_id;

    log_message("Accept thread started for port %d", CHAT_PORT);

    while (running) {
        client_addr_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr,
                             &client_addr_len);

        if (client_socket < 0) {
            if (errno != EINTR) {
                log_message("ERROR: accept() failed - %s", strerror(errno));
            }
            continue;
        }

        log_message("New connection from %s:%d",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        /* Receive authentication packet */
        bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_read < sizeof(packet_header_t)) {
            close(client_socket);
            continue;
        }

        header = (packet_header_t *)buffer;

        if (header->type == PKT_REGISTER) {
            reg_pkt = (register_pkt_t *)buffer;
            user_id = register_user(reg_pkt->username, (char *)reg_pkt->password_hash);

            if (user_id > 0) {
                auth_resp.header.type = PKT_AUTH_RESPONSE;
                auth_resp.header.sender_id = user_id;
                auth_resp.status = 0; /* Success */
                auth_resp.user_id = user_id;
                strncpy(auth_resp.token, "token", sizeof(auth_resp.token) - 1);

                send_packet(client_socket, &auth_resp.header, NULL, 0);

                log_message("Registration successful for user ID %u", user_id);
            } else {
                auth_resp.header.type = PKT_ERROR;
                auth_resp.status = 1; /* Failure */
                send_packet(client_socket, &auth_resp.header, NULL, 0);

                close(client_socket);
                continue;
            }
        } else if (header->type == PKT_LOGIN) {
            login_pkt = (login_pkt_t *)buffer;

            if (authenticate_user(login_pkt->username,
                                (char *)login_pkt->password_hash,
                                &user_id) == 0) {
                auth_resp.header.type = PKT_AUTH_RESPONSE;
                auth_resp.header.sender_id = user_id;
                auth_resp.status = 0; /* Success */
                auth_resp.user_id = user_id;
                strncpy(auth_resp.token, "token", sizeof(auth_resp.token) - 1);

                send_packet(client_socket, &auth_resp.header, NULL, 0);

                log_message("Login successful for %s (ID: %u)", login_pkt->username, user_id);
            } else {
                auth_resp.header.type = PKT_ERROR;
                auth_resp.status = 1; /* Failure */
                send_packet(client_socket, &auth_resp.header, NULL, 0);

                close(client_socket);
                continue;
            }
        } else {
            close(client_socket);
            continue;
        }

        /* Create client structure */
        client = malloc(sizeof(client_info_t));
        if (!client) {
            close(client_socket);
            continue;
        }

        memset(client, 0, sizeof(*client));
        client->user_id = user_id;
        client->socket_fd = client_socket;
        client->addr = client_addr;
        client->login_time = time(NULL);
        client->is_online = 1;

        if (header->type == PKT_REGISTER) {
            strncpy(client->username, reg_pkt->username, MAX_USERNAME_LEN - 1);
        } else {
            strncpy(client->username, login_pkt->username, MAX_USERNAME_LEN - 1);
        }

        /* Add to client list */
        pthread_mutex_lock(&client_lock);
        if (num_clients < MAX_CLIENTS) {
            int i;
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == NULL) {
                    clients[i] = client;
                    num_clients++;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&client_lock);

        /* Create thread for client */
        if (pthread_create(&client->client_thread, NULL, client_thread_func, client) != 0) {
            log_message("ERROR: Failed to create client thread");
            free(client);
            close(client_socket);
        }
    }

    log_message("Accept thread exiting");
    return NULL;
}

/* Signal handler */
static void signal_handler(int sig)
{
    log_message("Received signal %d, shutting down...", sig);
    running = 0;
}

/* Main */
int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr;
    pthread_t accept_thread;
    int opt = 1;

    log_message("Chat Daemon starting...");

    /* Open crypto device */
    crypto_device = open(CRYPTO_DEVICE, O_RDWR);
    if (crypto_device < 0) {
        log_message("ERROR: Failed to open %s - %s", CRYPTO_DEVICE, strerror(errno));
        log_message("Make sure kernel module is loaded: insmod chat_crypto.ko");
        exit(1);
    }

    log_message("Crypto device opened: %s", CRYPTO_DEVICE);

    /* Create server socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        log_message("ERROR: Failed to create socket - %s", strerror(errno));
        exit(1);
    }

    /* Set socket options */
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Bind socket */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(CHAT_PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        log_message("ERROR: Failed to bind socket - %s", strerror(errno));
        exit(1);
    }

    /* Listen for connections */
    if (listen(server_socket, 5) < 0) {
        log_message("ERROR: Failed to listen - %s", strerror(errno));
        exit(1);
    }

    log_message("Server listening on port %d", CHAT_PORT);

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Create accept thread */
    if (pthread_create(&accept_thread, NULL, accept_thread_func, NULL) != 0) {
        log_message("ERROR: Failed to create accept thread");
        exit(1);
    }

    /* Main loop */
    while (running) {
        sleep(1);
    }

    pthread_join(accept_thread, NULL);

    close(server_socket);
    close(crypto_device);

    log_message("Chat Daemon stopped");

    return 0;
}
