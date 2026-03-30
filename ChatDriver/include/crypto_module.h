#ifndef CRYPTO_MODULE_H
#define CRYPTO_MODULE_H

#include <linux/ioctl.h>

/* Device file */
#define CRYPTO_DEVICE_FILE "/dev/chat_crypto"

/* Constants */
#define SHA1_DIGEST_SIZE 20
#define SHA1_MAX_INPUT 4096
#define DES_KEY_SIZE 8
#define MAX_CRYPTO_DATA 4096

/* IOCTL command codes */
#define CRYPTO_IOCTL_MAGIC 0xC0
#define CRYPTO_IOCTL_SHA1_HASH _IOWR(CRYPTO_IOCTL_MAGIC, 1, struct crypto_sha1_req)
#define CRYPTO_IOCTL_DES_ENCRYPT _IOWR(CRYPTO_IOCTL_MAGIC, 2, struct crypto_des_req)
#define CRYPTO_IOCTL_DES_DECRYPT _IOWR(CRYPTO_IOCTL_MAGIC, 3, struct crypto_des_req)

/* Request structures for IOCTL */
struct crypto_sha1_req {
    unsigned char *input;
    size_t input_len;
    unsigned char output[SHA1_DIGEST_SIZE];
};

struct crypto_des_req {
    unsigned char key[DES_KEY_SIZE];
    unsigned char *input;
    size_t input_len;
    unsigned char *output;
    size_t *output_len;
};

#endif /* CRYPTO_MODULE_H */
