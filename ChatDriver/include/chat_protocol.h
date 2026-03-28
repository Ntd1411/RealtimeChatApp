/*
 * Chat Protocol Definitions
 * P2P communication protocol with authentication
 */

#ifndef CHAT_PROTOCOL_H
#define CHAT_PROTOCOL_H

#include <stdint.h>
#include <time.h>

#define CHAT_PORT 5555
#define MAX_USERNAME_LEN 32
#define MAX_PASSWORD_LEN 64
#define MAX_MESSAGE_LEN 2048

/* Protocol message types */
typedef enum {
    PKT_REGISTER = 0x01,
    PKT_LOGIN = 0x02,
    PKT_AUTH_RESPONSE = 0x03,
    PKT_MESSAGE = 0x04,
    PKT_MESSAGE_ACK = 0x05,
    PKT_DISCONNECT = 0x06,
    PKT_USER_LIST = 0x07,
    PKT_USER_ONLINE = 0x08,
    PKT_HEARTBEAT = 0x09,
    PKT_ERROR = 0xFF
} packet_type_t;

typedef struct {
    uint8_t type;
    uint32_t sequence;
    uint32_t sender_id;
    uint32_t receiver_id;
    uint16_t length;
    uint64_t timestamp;
} __attribute__((packed)) packet_header_t;

/* Register packet */
typedef struct {
    packet_header_t header;
    char username[MAX_USERNAME_LEN];
    char password_hash[20]; /* SHA1 hash */
} __attribute__((packed)) register_pkt_t;

/* Login packet */
typedef struct {
    packet_header_t header;
    char username[MAX_USERNAME_LEN];
    char password_hash[20]; /* SHA1 hash */
} __attribute__((packed)) login_pkt_t;

/* Auth response */
typedef struct {
    packet_header_t header;
    uint8_t status; /* 0=success, 1=fail */
    uint32_t user_id;
    char token[64];
} __attribute__((packed)) auth_resp_t;

/* Message packet - encrypted with DES */
typedef struct {
    packet_header_t header;
    uint8_t iv[8]; /* DES IV */
    char encrypted_body[MAX_MESSAGE_LEN + 8]; /* DES encrypted message */
} __attribute__((packed)) message_pkt_t;

/* Message acknowledge */
typedef struct {
    packet_header_t header;
    uint32_t acked_sequence;
    uint8_t status;
} __attribute__((packed)) msg_ack_t;

#endif /* CHAT_PROTOCOL_H */
