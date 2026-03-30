#include "cryptodriver.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include "../include/crypto_module.h"

struct crypto_driver {
    int device_fd;
};

CryptoDriver* crypto_driver_new(void)
{
    CryptoDriver *driver = (CryptoDriver *)malloc(sizeof(CryptoDriver));
    if (!driver) {
        return NULL;
    }

    driver->device_fd = open("/dev/chat_crypto", O_RDWR);
    if (driver->device_fd < 0) {
        fprintf(stderr, "Error: Failed to open /dev/chat_crypto\n");
        fprintf(stderr, "Make sure kernel module is loaded: lsmod | grep chat_crypto\n");
        free(driver);
        return NULL;
    }

    return driver;
}

void crypto_driver_free(CryptoDriver *driver)
{
    if (driver) {
        if (driver->device_fd >= 0) {
            close(driver->device_fd);
        }
        free(driver);
    }
}

int crypto_driver_is_open(const CryptoDriver *driver)
{
    return (driver && driver->device_fd >= 0);
}

int crypto_driver_sha1(CryptoDriver *driver, 
                      const unsigned char *input, 
                      size_t input_len,
                      unsigned char *output)
{
    struct sha1_request req;

    if (!driver || driver->device_fd < 0) {
        return CRYPTO_ERR_NO_DEVICE;
    }

    if (!input || !output) {
        return CRYPTO_ERR_INPUT;
    }

    if (input_len > SHA1_MAX_INPUT) {
        return CRYPTO_ERR_INPUT;
    }

    memcpy(req.input, input, input_len);
    req.input_len = input_len;

    if (ioctl(driver->device_fd, CRYPTO_IOCTL_SHA1_HASH, &req) < 0) {
        perror("SHA1 ioctl");
        return CRYPTO_ERR_IOCTL;
    }

    memcpy(output, req.digest, SHA1_DIGEST_SIZE);
    return CRYPTO_OK;
}

int crypto_driver_des_encrypt(CryptoDriver *driver,
                             const unsigned char *key,
                             const unsigned char *plaintext,
                             size_t plaintext_len,
                             unsigned char *ciphertext,
                             size_t *ciphertext_len)
{
    struct des_request req;
    unsigned long padded_len;
    int pad_len, i;

    if (!driver || driver->device_fd < 0) {
        return CRYPTO_ERR_NO_DEVICE;
    }

    if (!key || !plaintext || !ciphertext || !ciphertext_len) {
        return CRYPTO_ERR_INPUT;
    }

    /* Pad input to 8-byte boundary */
    padded_len = (plaintext_len + 7) & ~7;
    pad_len = padded_len - plaintext_len;

    if (padded_len > MAX_CRYPTO_DATA) {
        return CRYPTO_ERR_INPUT;
    }

    /* Copy plaintext and apply padding */
    memcpy(req.input, plaintext, plaintext_len);
    for (i = plaintext_len; i < (int)padded_len; i++) {
        req.input[i] = pad_len;
    }

    memcpy(req.key, key, DES_KEY_SIZE);
    req.input_len = padded_len;
    req.mode = 0; /* Encrypt */

    if (ioctl(driver->device_fd, CRYPTO_IOCTL_DES_ENCRYPT, &req) < 0) {
        perror("DES encrypt ioctl");
        return CRYPTO_ERR_IOCTL;
    }

    *ciphertext_len = req.output_len;
    memcpy(ciphertext, req.output, req.output_len);

    return CRYPTO_OK;
}

int crypto_driver_des_decrypt(CryptoDriver *driver,
                             const unsigned char *key,
                             const unsigned char *ciphertext,
                             size_t ciphertext_len,
                             unsigned char *plaintext,
                             size_t *plaintext_len)
{
    struct des_request req;
    int pad_len, i;

    if (!driver || driver->device_fd < 0) {
        return CRYPTO_ERR_NO_DEVICE;
    }

    if (!key || !ciphertext || !plaintext || !plaintext_len) {
        return CRYPTO_ERR_INPUT;
    }

    if (ciphertext_len > MAX_CRYPTO_DATA || ciphertext_len % 8 != 0) {
        return CRYPTO_ERR_INPUT;
    }

    memcpy(req.input, ciphertext, ciphertext_len);
    memcpy(req.key, key, DES_KEY_SIZE);
    req.input_len = ciphertext_len;
    req.mode = 1; /* Decrypt */

    if (ioctl(driver->device_fd, CRYPTO_IOCTL_DES_DECRYPT, &req) < 0) {
        perror("DES decrypt ioctl");
        return CRYPTO_ERR_IOCTL;
    }

    /* Remove padding */
    unsigned char *output = req.output;
    int output_len = req.output_len;

    if (output_len > 0) {
        pad_len = (int)(unsigned char)output[output_len - 1];
        if (pad_len > 0 && pad_len <= 8) {
            output_len -= pad_len;
        }
    }

    *plaintext_len = output_len;
    memcpy(plaintext, output, output_len);

    return CRYPTO_OK;
}
