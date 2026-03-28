/*
 * Shared header for crypto operations in kernel module
 * DES encryption and SHA1 hashing
 */

#ifndef CRYPTO_MODULE_H
#define CRYPTO_MODULE_H

#include <linux/types.h>

#define CRYPTO_IOC_MAGIC 'C'
#define CRYPTO_IOCTL_DES_ENCRYPT _IOW(CRYPTO_IOC_MAGIC, 1, struct des_request)
#define CRYPTO_IOCTL_DES_DECRYPT _IOW(CRYPTO_IOC_MAGIC, 2, struct des_request)
#define CRYPTO_IOCTL_SHA1_HASH   _IOW(CRYPTO_IOC_MAGIC, 3, struct sha1_request)

/* DES Block size */
#define DES_BLOCK_SIZE 8
#define DES_KEY_SIZE 8
#define MAX_CRYPTO_DATA 4096

/* SHA1 output size */
#define SHA1_DIGEST_SIZE 20
#define SHA1_MAX_INPUT 4096

/* DES Request Structure */
struct des_request {
    unsigned char key[DES_KEY_SIZE];
    unsigned char input[MAX_CRYPTO_DATA];
    unsigned char output[MAX_CRYPTO_DATA];
    unsigned long input_len;
    unsigned long output_len;
    int mode; /* 0 = encrypt, 1 = decrypt */
};

/* SHA1 Request Structure */
struct sha1_request {
    unsigned char input[SHA1_MAX_INPUT];
    unsigned char digest[SHA1_DIGEST_SIZE];
    unsigned long input_len;
};

/* Network packet format for chat */
struct chat_packet {
    unsigned int sender_id;
    unsigned int receiver_id;
    unsigned long timestamp;
    unsigned long msg_len;
    unsigned char msg_hash[SHA1_DIGEST_SIZE]; /* SHA1 of message */
    unsigned char iv[DES_BLOCK_SIZE];          /* IV for DES */
    unsigned char encrypted_msg[MAX_CRYPTO_DATA]; /* DES encrypted */
};

#endif /* CRYPTO_MODULE_H */
