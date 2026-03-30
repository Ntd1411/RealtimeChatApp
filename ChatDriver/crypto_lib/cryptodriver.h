#ifndef CRYPTODRIVER_H
#define CRYPTODRIVER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle to crypto device */
typedef struct crypto_driver CryptoDriver;

/* Create/destroy crypto driver handle */
CryptoDriver* crypto_driver_new(void);
void crypto_driver_free(CryptoDriver *driver);

/* Check if device is open */
int crypto_driver_is_open(const CryptoDriver *driver);

/* SHA1 hashing */
int crypto_driver_sha1(CryptoDriver *driver, 
                      const unsigned char *input, 
                      size_t input_len,
                      unsigned char *output);

/* DES encryption */
int crypto_driver_des_encrypt(CryptoDriver *driver,
                             const unsigned char *key,
                             const unsigned char *plaintext,
                             size_t plaintext_len,
                             unsigned char *ciphertext,
                             size_t *ciphertext_len);

/* DES decryption */
int crypto_driver_des_decrypt(CryptoDriver *driver,
                             const unsigned char *key,
                             const unsigned char *ciphertext,
                             size_t ciphertext_len,
                             unsigned char *plaintext,
                             size_t *plaintext_len);

/* Error codes */
#define CRYPTO_OK           0
#define CRYPTO_ERR_NO_DEVICE (-1)
#define CRYPTO_ERR_IOCTL    (-2)
#define CRYPTO_ERR_INPUT    (-3)
#define CRYPTO_ERR_OUTPUT   (-4)

#ifdef __cplusplus
}
#endif

#endif /* CRYPTODRIVER_H */
