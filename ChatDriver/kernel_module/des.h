/*
 * DES (Data Encryption Standard) Implementation
 * FIPS 46-3 - Data Encryption Standard
 */

#ifndef _DES_H
#define _DES_H

#include <linux/types.h>

#define DES_KEY_SIZE        8       /* 64 bits, 8 bytes */
#define DES_BLOCK_SIZE      8       /* 64 bits, 8 bytes */
#define DES_MAX_INPUT       4096

/**
 * DES Context Structure
 */
struct des_context {
    uint32_t subkeys[32];       /* 16 rounds × 2 subkeys per round */
};

/**
 * Initialize DES context with key (expand key schedule)
 */
void des_init(struct des_context *ctx, const uint8_t key[DES_KEY_SIZE]);

/**
 * Encrypt one 8-byte block
 * Input and output can be the same buffer
 */
void des_encrypt_block(struct des_context *ctx, 
                       const uint8_t input[DES_BLOCK_SIZE],
                       uint8_t output[DES_BLOCK_SIZE]);

/**
 * Decrypt one 8-byte block
 * Input and output can be the same buffer
 */
void des_decrypt_block(struct des_context *ctx,
                       const uint8_t input[DES_BLOCK_SIZE],
                       uint8_t output[DES_BLOCK_SIZE]);

/**
 * Encrypt data in ECB mode (Electronic Codebook)
 * Input length must be multiple of 8 bytes
 */
void des_encrypt_ecb(struct des_context *ctx,
                     const uint8_t *input,
                     uint8_t *output,
                     uint32_t len);

/**
 * Decrypt data in ECB mode
 * Input length must be multiple of 8 bytes
 */
void des_decrypt_ecb(struct des_context *ctx,
                     const uint8_t *input,
                     uint8_t *output,
                     uint32_t len);

#endif /* _DES_H */
