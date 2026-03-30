/*
 * SHA1 Hash Implementation
 * RFC 3174 - US Secure Hash Algorithm
 */

#ifndef _SHA1_H
#define _SHA1_H

#include <linux/types.h>

#define SHA1_DIGEST_SIZE    20
#define SHA1_BLOCK_SIZE     64
#define SHA1_MAX_INPUT      4096

/**
 * SHA1 Context Structure
 */
struct sha1_context {
    uint32_t h[5];              /* 5 hash values (A, B, C, D, E) */
    uint64_t bitcount;          /* Total bits processed */
    uint8_t buffer[SHA1_BLOCK_SIZE];  /* Current block buffer */
    uint32_t buflen;            /* Bytes in buffer */
};

/**
 * Initialize SHA1 context
 */
void sha1_init(struct sha1_context *ctx);

/**
 * Update SHA1 with input data
 */
void sha1_update(struct sha1_context *ctx, 
                 const uint8_t *input, 
                 uint32_t len);

/**
 * Finalize SHA1 and get digest
 */
void sha1_final(struct sha1_context *ctx, 
                uint8_t digest[SHA1_DIGEST_SIZE]);

/**
 * One-shot SHA1 hash
 */
void sha1_hash(const uint8_t *input, 
               uint32_t len, 
               uint8_t digest[SHA1_DIGEST_SIZE]);

#endif /* _SHA1_H */
