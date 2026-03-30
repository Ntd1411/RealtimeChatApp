/*
 * DES (Data Encryption Standard) Implementation (FIPS 46-3)
 * Custom implementation without kernel crypto API
 */

#include <linux/string.h>
#include "des.h"

/* DES S-boxes (Substitution boxes) */
static const uint8_t sbox[8][64] = {
    /* S1 */
    { 14,0,4,15,13,7,1,4, 2,14,15,2,11,13,8,1,
      3,10,10,6,6,12,12,11, 5,9,9,5,0,6,0,14,
      7,11,10,4,13,1,5,11, 6,6,11,5,6,4,7,16,
      4,13,11,7,2,6,1,4, 13,12,7,11,13,6,1,9 },
    /* S2 */
    { 15,1,8,14,6,11,3,4, 9,7,2,13,12,0,5,10,
      3,13,4,7,15,2,8,14, 12,0,1,10,6,9,11,5,
      0,14,7,11,10,4,13,1, 5,12,12,1,13,0,11,6,
      4,11,2,15,4,2,7,12, 9,5,10,15,6,8,0,13 },
    /* S3 */
    { 10,0,9,14,6,3,15,5, 1,13,12,0,11,4,2,15,
      13,8,10,1,3,15,4,2, 11,6,7,12,0,5,14,9,
      14,4,5,8,7,6,13,15, 0,15,8,4,7,4,5,14,
      12,16,1,14,8,7,6,11, 4,12,2,15,5,12,3,7 },
    /* S4 */
    { 2,12,4,1,7,10,11,6, 8,5,3,15,13,0,14,9,
      14,11,2,12,4,7,13,1, 5,0,15,10,3,9,8,6,
      4,2,1,11,10,13,7,8, 15,9,12,5,6,2,12,9,
      7,2,11,1,4,14,12,16, 0,13,10,12,9,15,5,11 },
    /* S5 */
    { 2,14,4,11,13,15,11,8, 3,10,14,4,9,12,15,5,
      9,8,5,12,1,15,14,10, 7,6,8,13,6,5,12,7,
      5,11,12,14,15,14,3,10, 8,7,0,4,11,13,8,1,
      4,13,1,6,11,16,12,7, 13,10,3,15,9,0,6,6 },
    /* S6 */
    { 12,1,10,15,9,2,6,8, 0,13,3,4,14,7,5,11,
      10,15,4,2,7,12,9,5, 6,1,13,14,0,11,3,8,
      9,14,15,5,2,8,12,3, 7,0,4,10,1,13,11,6,
      4,3,2,12,9,5,15,10, 11,14,1,7,6,0,8,13 },
    /* S7 */
    { 4,11,2,14,15,0,8,13, 3,12,9,7,5,10,6,1,
      13,0,11,5,12,1,9,15, 7,4,12,8,2,10,14,12,
      15,1,4,2,7,6,10,9, 10,28,6,11,13,14,5,0,
      13,3,11,1,1,10,14,9, 5,12,6,15,7,2,0,4 },
    /* S8 */
    { 13,2,8,4,6,15,11,1, 10,9,3,14,5,0,12,7,
      1,15,13,8,10,3,7,4, 12,5,6,11,0,14,9,2,
      7,11,4,1,9,12,14,2, 0,6,10,13,15,3,5,8,
      2,1,14,7,4,10,8,13, 15,12,9,0,3,5,6,11 }
};

/* Initial Permutation */
static const uint8_t ip[64] = {
    58,50,42,34,26,18,10,2, 60,52,44,36,28,20,12,4,
    62,54,46,38,30,22,14,6, 64,56,48,40,32,24,16,8,
    57,49,41,33,25,17,9,1, 59,51,43,35,27,19,11,3,
    61,53,45,37,29,21,13,5, 63,55,47,39,31,23,15,7
};

/* Final Permutation (inverse of IP) */
static const uint8_t fp[64] = {
    40,8,48,16,56,24,64,32, 39,7,47,15,55,23,63,31,
    38,6,46,14,54,22,62,30, 37,5,45,13,53,21,61,29,
    36,4,44,12,52,20,60,28, 35,3,43,11,51,19,59,27,
    34,2,42,10,50,18,58,26, 33,1,41,9,49,17,57,25
};

/* Expansion Permutation (32 bits → 48 bits) */
static const uint8_t exp[48] = {
    32,1,2,3,4,5, 4,5,6,7,8,9, 8,9,10,11,12,13,
    12,13,14,15,16,17, 16,17,18,19,20,21, 20,21,22,23,24,25,
    24,25,26,27,28,29, 28,29,30,31,32,1
};

/* Parity Drop Permutation (64 bits → 56 bits, removes parity bits) */
static const uint8_t pc1[56] = {
    57,49,41,33,25,17,9, 1,58,50,42,34,26,18,10,2,
    59,51,43,35,27,19,11, 3,60,52,44,36,28,20,12,4,
    62,54,46,38,30,22,14, 6,61,53,45,37,29,21,13,5,
    63,55,47,39,31,23,15, 7
};

/* PC-2 Permutation (56 bits → 48 bits subkey) */
static const uint8_t pc2[48] = {
    14,17,11,24,1,5, 3,28,15,6,21,10,
    23,19,12,4,26,8, 16,7,27,20,13,2,
    41,52,31,37,47,55, 30,40,51,45,33,48,
    44,49,39,56,34,53, 46,42,50,36,29,32
};

/* Permutation P (32 bits) */
static const uint8_t p[32] = {
    16,7,20,21, 29,12,28,17, 1,15,23,26, 5,18,31,10,
    2,8,24,14, 32,27,3,9, 19,13,30,6, 22,11,4,25
};

/* Left rotation counts for key schedule */
static const uint8_t left_rot[16] = {
    1,1,2,2, 2,2,2,2, 1,2,2,2, 2,2,2,1
};

/**
 * Bit extraction helper
 */
static uint8_t get_bit(const uint8_t *data, int index)
{
    return (data[index >> 3] >> (7 - (index & 7))) & 1;
}

/**
 * Bit setting helper
 */
static void set_bit(uint8_t *data, int index, uint8_t value)
{
    if (value) {
        data[index >> 3] |= (1 << (7 - (index & 7)));
    } else {
        data[index >> 3] &= ~(1 << (7 - (index & 7)));
    }
}

/**
 * Apply permutation table
 */
static void permute(const uint8_t *input, uint8_t *output, 
                   const uint8_t *table, int size)
{
    int i;
    memset(output, 0, (size + 7) >> 3);
    for (i = 0; i < size; i++) {
        uint8_t bit = get_bit(input, table[i] - 1);
        set_bit(output, i, bit);
    }
}

/**
 * Left rotate 28-bit value
 */
static void rotate_left(uint8_t *data, int count)
{
    int i, j;
    uint8_t temp[4];
    
    memcpy(temp, data, 4);
    for (i = 0; i < 28; i++) {
        uint8_t bit = get_bit(temp, i);
        set_bit(data, (i + count) % 28, bit);
    }
}

/**
 * Key schedule generation
 */
static void generate_subkeys(const uint8_t key[DES_KEY_SIZE], uint32_t subkeys[32])
{
    uint8_t key_56[7];
    uint8_t c[4], d[4];
    int round, i;

    /* Apply PC-1 to remove parity bits (64 → 56) */
    permute(key, key_56, pc1, 56);

    /* Split into left (C) and right (D) 28-bit halves */
    memcpy(c, key_56, 4);
    memcpy(d, key_56 + 3, 4);

    /* Generate 16 subkeys */
    for (round = 0; round < 16; round++) {
        uint8_t c_temp[4], d_temp[4];
        uint8_t cd[7];
        uint8_t subkey[6];

        /* Rotate left by specified amount */
        memcpy(c_temp, c, 4);
        memcpy(d_temp, d, 4);
        
        for (i = 0; i < left_rot[round]; i++) {
            rotate_left(c_temp, 1);
            rotate_left(d_temp, 1);
        }

        memcpy(c, c_temp, 4);
        memcpy(d, d_temp, 4);

        /* Combine C and D for PC-2 */
        memcpy(cd, c, 4);
        memcpy(cd + 3, d, 4);

        /* Apply PC-2 (56 → 48, generates subkey) */
        permute(cd, subkey, pc2, 48);

        /* Store as two 24-bit values (for simplicity) */
        subkeys[round * 2] = ((uint32_t)subkey[0] << 16) |
                            ((uint32_t)subkey[1] << 8) |
                            (uint32_t)subkey[2];
        subkeys[round * 2 + 1] = ((uint32_t)subkey[3] << 16) |
                                ((uint32_t)subkey[4] << 8) |
                                (uint32_t)subkey[5];
    }
}

/**
 * F-function for DES round
 */
static uint32_t f_function(uint32_t r, const uint32_t *subkey)
{
    uint8_t r_bytes[4];
    uint8_t expanded[6];
    uint8_t xored[6];
    uint8_t sbox_out[4];
    uint32_t result = 0;
    int i, sb;

    /* Convert R to bytes */
    r_bytes[0] = (r >> 24) & 0xff;
    r_bytes[1] = (r >> 16) & 0xff;
    r_bytes[2] = (r >> 8) & 0xff;
    r_bytes[3] = r & 0xff;

    /* Expansion: 32 bits → 48 bits */
    memset(expanded, 0, 6);
    for (i = 0; i < 48; i++) {
        uint8_t bit = get_bit(r_bytes, exp[i] - 1);
        set_bit(expanded, i, bit);
    }

    /* XOR with subkey */
    for (i = 0; i < 6; i++) {
        xored[i] = expanded[i] ^ ((uint8_t *)subkey)[i];
    }

    /* Apply S-boxes (48 bits → 32 bits) */
    memset(sbox_out, 0, 4);
    for (sb = 0; sb < 8; sb++) {
        uint8_t row, col, sbox_val;
        int bit_pos = sb * 6;

        /* Extract 6 bits for this S-box */
        row = (get_bit(xored, bit_pos) << 1) | 
              get_bit(xored, bit_pos + 5);
        col = (get_bit(xored, bit_pos + 1) << 3) |
              (get_bit(xored, bit_pos + 2) << 2) |
              (get_bit(xored, bit_pos + 3) << 1) |
              get_bit(xored, bit_pos + 4);

        sbox_val = sbox[sb][row * 16 + col];

        /* Store 4 output bits */
        int out_pos = sb * 4;
        set_bit(sbox_out, out_pos, (sbox_val >> 3) & 1);
        set_bit(sbox_out, out_pos + 1, (sbox_val >> 2) & 1);
        set_bit(sbox_out, out_pos + 2, (sbox_val >> 1) & 1);
        set_bit(sbox_out, out_pos + 3, sbox_val & 1);
    }

    /* Apply P-permutation */
    uint8_t p_out[4];
    permute(sbox_out, p_out, p, 32);

    result = ((uint32_t)p_out[0] << 24) |
            ((uint32_t)p_out[1] << 16) |
            ((uint32_t)p_out[2] << 8) |
            (uint32_t)p_out[3];

    return result;
}

/**
 * Initialize DES context with key
 */
void des_init(struct des_context *ctx, const uint8_t key[DES_KEY_SIZE])
{
    generate_subkeys(key, ctx->subkeys);
}

/**
 * Encrypt one 8-byte block
 */
void des_encrypt_block(struct des_context *ctx,
                       const uint8_t input[DES_BLOCK_SIZE],
                       uint8_t output[DES_BLOCK_SIZE])
{
    uint8_t perm[8];
    uint32_t l, r, tmp;
    int round, i;

    /* Initial Permutation */
    permute(input, perm, ip, 64);

    /* Extract L and R */
    l = ((uint32_t)perm[0] << 24) | ((uint32_t)perm[1] << 16) |
        ((uint32_t)perm[2] << 8) | (uint32_t)perm[3];
    r = ((uint32_t)perm[4] << 24) | ((uint32_t)perm[5] << 16) |
        ((uint32_t)perm[6] << 8) | (uint32_t)perm[7];

    /* 16 rounds */
    for (round = 0; round < 16; round++) {
        uint32_t f_out = f_function(r, &ctx->subkeys[round * 2]);
        tmp = r;
        r = l ^ f_out;
        l = tmp;
    }

    /* Reverse final swap */
    tmp = l;
    l = r;
    r = tmp;

    /* Combine L and R */
    perm[0] = (l >> 24) & 0xff;
    perm[1] = (l >> 16) & 0xff;
    perm[2] = (l >> 8) & 0xff;
    perm[3] = l & 0xff;
    perm[4] = (r >> 24) & 0xff;
    perm[5] = (r >> 16) & 0xff;
    perm[6] = (r >> 8) & 0xff;
    perm[7] = r & 0xff;

    /* Final Permutation */
    permute(perm, output, fp, 64);
}

/**
 * Decrypt one 8-byte block
 */
void des_decrypt_block(struct des_context *ctx,
                       const uint8_t input[DES_BLOCK_SIZE],
                       uint8_t output[DES_BLOCK_SIZE])
{
    uint8_t perm[8];
    uint32_t l, r, tmp;
    int round;

    /* Initial Permutation */
    permute(input, perm, ip, 64);

    /* Extract L and R */
    l = ((uint32_t)perm[0] << 24) | ((uint32_t)perm[1] << 16) |
        ((uint32_t)perm[2] << 8) | (uint32_t)perm[3];
    r = ((uint32_t)perm[4] << 24) | ((uint32_t)perm[5] << 16) |
        ((uint32_t)perm[6] << 8) | (uint32_t)perm[7];

    /* 16 rounds (in reverse order) */
    for (round = 15; round >= 0; round--) {
        uint32_t f_out = f_function(r, &ctx->subkeys[round * 2]);
        tmp = r;
        r = l ^ f_out;
        l = tmp;
    }

    /* Reverse final swap */
    tmp = l;
    l = r;
    r = tmp;

    /* Combine L and R */
    perm[0] = (l >> 24) & 0xff;
    perm[1] = (l >> 16) & 0xff;
    perm[2] = (l >> 8) & 0xff;
    perm[3] = l & 0xff;
    perm[4] = (r >> 24) & 0xff;
    perm[5] = (r >> 16) & 0xff;
    perm[6] = (r >> 8) & 0xff;
    perm[7] = r & 0xff;

    /* Final Permutation */
    permute(perm, output, fp, 64);
}

/**
 * Encrypt data in ECB mode
 */
void des_encrypt_ecb(struct des_context *ctx,
                     const uint8_t *input,
                     uint8_t *output,
                     uint32_t len)
{
    uint32_t blocks = len >> 3;
    uint32_t i;

    for (i = 0; i < blocks; i++) {
        des_encrypt_block(ctx, input + i * 8, output + i * 8);
    }
}

/**
 * Decrypt data in ECB mode
 */
void des_decrypt_ecb(struct des_context *ctx,
                     const uint8_t *input,
                     uint8_t *output,
                     uint32_t len)
{
    uint32_t blocks = len >> 3;
    uint32_t i;

    for (i = 0; i < blocks; i++) {
        des_decrypt_block(ctx, input + i * 8, output + i * 8);
    }
}
