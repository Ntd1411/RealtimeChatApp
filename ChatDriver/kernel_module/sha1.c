/*
 * SHA1 Hash Implementation (RFC 3174)
 * Custom implementation without kernel crypto API
 */

#include <linux/string.h>
#include "sha1.h"

/* SHA1 Constants */
#define K0  0x5a827999
#define K1  0x6ed9eba1
#define K2  0x8f1bbcdc
#define K3  0xca62c1d6

/* Helper macros */
#define ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define BE32(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | \
                 (((x) & 0xff0000) >> 8) | (((x) >> 24) & 0xff))

/* Get 32-bit big-endian value from buffer */
static uint32_t get_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

/* Put 32-bit big-endian value into buffer */
static void put_be32(uint8_t *p, uint32_t v)
{
    p[0] = (v >> 24) & 0xff;
    p[1] = (v >> 16) & 0xff;
    p[2] = (v >> 8) & 0xff;
    p[3] = v & 0xff;
}

/* Put 64-bit big-endian value into buffer */
static void put_be64(uint8_t *p, uint64_t v)
{
    put_be32(p, (uint32_t)(v >> 32));
    put_be32(p + 4, (uint32_t)v);
}

/**
 * Process 512-bit (64-byte) block
 */
static void sha1_process_block(struct sha1_context *ctx, const uint8_t *block)
{
    uint32_t w[80];
    uint32_t a, b, c, d, e, temp;
    int i;

    /* Parse block into 16 32-bit words (big-endian) */
    for (i = 0; i < 16; i++) {
        w[i] = get_be32(block + i * 4);
    }

    /* Extend to 80 words */
    for (i = 16; i < 80; i++) {
        w[i] = ROTL(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    }

    /* Initialize working variables */
    a = ctx->h[0];
    b = ctx->h[1];
    c = ctx->h[2];
    d = ctx->h[3];
    e = ctx->h[4];

    /* Main loop - 80 rounds */
    for (i = 0; i < 80; i++) {
        uint32_t f, k;

        if (i < 20) {
            /* Round 1: f = (b AND c) OR (NOT b AND d) */
            f = (b & c) | ((~b) & d);
            k = K0;
        } else if (i < 40) {
            /* Round 2: f = b XOR c XOR d */
            f = b ^ c ^ d;
            k = K1;
        } else if (i < 60) {
            /* Round 3: f = (b AND c) OR (b AND d) OR (c AND d) */
            f = (b & c) | (b & d) | (c & d);
            k = K2;
        } else {
            /* Round 4: f = b XOR c XOR d */
            f = b ^ c ^ d;
            k = K3;
        }

        temp = ROTL(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = ROTL(b, 30);
        b = a;
        a = temp;
    }

    /* Add this chunk's hash to result so far */
    ctx->h[0] += a;
    ctx->h[1] += b;
    ctx->h[2] += c;
    ctx->h[3] += d;
    ctx->h[4] += e;
}

/**
 * Initialize SHA1 context
 */
void sha1_init(struct sha1_context *ctx)
{
    ctx->h[0] = 0x67452301;
    ctx->h[1] = 0xefcdab89;
    ctx->h[2] = 0x98badcfe;
    ctx->h[3] = 0x10325476;
    ctx->h[4] = 0xc3d2e1f0;
    ctx->bitcount = 0;
    ctx->buflen = 0;
}

/**
 * Update SHA1 with input data
 */
void sha1_update(struct sha1_context *ctx, 
                 const uint8_t *input, 
                 uint32_t len)
{
    uint32_t have, need;

    if (!input || len == 0) {
        return;
    }

    /* Update bit count */
    ctx->bitcount += (uint64_t)len << 3;

    /* Handle partial block */
    have = ctx->buflen;
    need = SHA1_BLOCK_SIZE - have;

    if (len >= need) {
        /* Complete the pending block */
        if (have) {
            memcpy(ctx->buffer + have, input, need);
            sha1_process_block(ctx, ctx->buffer);
            input += need;
            len -= need;
            ctx->buflen = 0;
        }

        /* Process full blocks */
        while (len >= SHA1_BLOCK_SIZE) {
            sha1_process_block(ctx, input);
            input += SHA1_BLOCK_SIZE;
            len -= SHA1_BLOCK_SIZE;
        }
    }

    /* Save remaining bytes for next call */
    if (len > 0) {
        memcpy(ctx->buffer + ctx->buflen, input, len);
        ctx->buflen += len;
    }
}

/**
 * Finalize SHA1 and get digest
 */
void sha1_final(struct sha1_context *ctx, uint8_t digest[SHA1_DIGEST_SIZE])
{
    uint8_t final_block[SHA1_BLOCK_SIZE];
    uint32_t have = ctx->buflen;
    uint32_t need = (have < 56) ? (56 - have) : (120 - have);
    uint32_t i;

    /* Append bit '1' to message */
    ctx->buffer[have] = 0x80;
    memset(ctx->buffer + have + 1, 0, need - 1);

    /* Append message length (in bits) as 64-bit big-endian */
    put_be64(ctx->buffer + SHA1_BLOCK_SIZE - 8, ctx->bitcount);

    /* Process final block(s) */
    if (have < 56) {
        sha1_process_block(ctx, ctx->buffer);
    } else {
        sha1_process_block(ctx, ctx->buffer);
        sha1_process_block(ctx, ctx->buffer + SHA1_BLOCK_SIZE);
    }

    /* Output hash as 5 big-endian 32-bit words */
    for (i = 0; i < 5; i++) {
        put_be32(digest + i * 4, ctx->h[i]);
    }
}

/**
 * One-shot SHA1 hash
 */
void sha1_hash(const uint8_t *input, uint32_t len, uint8_t digest[SHA1_DIGEST_SIZE])
{
    struct sha1_context ctx;

    sha1_init(&ctx);
    sha1_update(&ctx, input, len);
    sha1_final(&ctx, digest);
}
