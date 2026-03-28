/*
 * Kernel Module for Chat Encryption/Hashing (CentOS 6 compatible)
 * DES encryption and SHA1 hashing
 * Device: /dev/chat_crypto
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/unistd.h>
#include <linux/slab.h>

#include "../include/crypto_module.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chat Driver Team");
MODULE_DESCRIPTION("DES + SHA1 Encryption Driver for Chat Application");
MODULE_VERSION("1.0");

#define DEVICE_NAME "chat_crypto"
#define CLASS_NAME "chat_crypto"

/* DES-specific constants */
#define DES_ROUNDS 16

static dev_t dev_num;
static struct cdev dev_cdev;
static struct class *dev_class;
static struct device *dev_device;

/* DES S-boxes */
static const unsigned char des_sbox[8][64] = {
    {
        14,  4, 13,  1,  2, 15, 11,  8,  3, 10, 14,  4, 15,  2,  5, 12,
         2, 13,  4,  1, 15, 11,  8,  3, 10, 14,  4, 15,  2,  5, 12,  9,
         3, 13,  4,  1, 15, 11,  8,  3, 10, 14,  4, 15,  2,  5, 12,  9,
         3, 13,  4,  1, 15, 11,  8,  3, 10, 14,  4, 15,  2,  5, 12,  9,
    },
    {
        15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
         3, 13,  4,  1, 15, 11,  8,  3, 10, 14,  4, 15,  2,  5, 12,  9,
         0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6, 15,  3, 12,  0,
         1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
    },
    {
        10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
        13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
        13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
         1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12,
    },
    {
         7, 13, 14,  3,  4, 15,  2,  8,  1,  6, 11,  5,  0,  9, 13, 14,
         3,  4, 15,  2,  8,  1,  6, 11,  5,  0,  9, 13, 14,  3,  4, 15,
        14,  4, 13,  1,  2, 15, 11,  8,  3, 10, 14,  4, 15,  2,  5, 12,
        10,  9,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
    },
    {
         2, 14, 12, 11,  0,  6, 13,  9,  5,  2, 14, 12, 11,  0,  6, 13,
         9,  5,  2, 14, 12, 11,  0,  6, 13,  9,  5,  2, 14, 12, 11,  0,
        12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
        10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
    },
    {
         4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
        13,  0, 11,  5,  6,  4,  9,  8, 15,  3, 12,  0,  6, 13,  9,  5,
         1, 13, 11,  5,  6,  4,  9,  8, 15,  3, 12,  0,  6, 13,  9,  5,
        13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
    },
    {
        10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
        14,  4, 13,  1,  2, 15, 11,  8,  3, 10, 14,  4, 15,  2,  5, 12,
         4, 13,  1,  2, 15, 11,  8,  3, 10, 14,  4, 15,  2,  5, 12,  9,
         3, 13,  4,  1, 15, 11,  8,  3, 10, 14,  4, 15,  2,  5, 12,  9,
    },
    {
        13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
         1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
         7,  4, 13,  1,  5,  0, 15, 10,  3,  7,  4, 13,  1,  5,  0, 15,
        10,  3,  7,  4, 13,  1,  5,  0, 15, 10,  3,  7,  4, 13,  1,  5,
    }
};

/* SHA1 Constants */
#define SHA1_F1(x, y, z) ((x & y) | (~x & z))
#define SHA1_F2(x, y, z) (x ^ y ^ z)
#define SHA1_F3(x, y, z) ((x & y) | (x & z) | (y & z))
#define SHA1_ROTL(x, n) ((x << n) | (x >> (32 - n)))

typedef struct {
    uint32_t h[5];
    uint32_t w[80];
    uint64_t size;
} sha1_ctx_t;

/* Simplified SHA1 - not production ready, educational only */
static void sha1_init(sha1_ctx_t *ctx)
{
    ctx->h[0] = 0x67452301;
    ctx->h[1] = 0xEFCDAB89;
    ctx->h[2] = 0x98BADCFE;
    ctx->h[3] = 0x10325476;
    ctx->h[4] = 0xC3D2E1F0;
    ctx->size = 0;
}

static void sha1_process_block(sha1_ctx_t *ctx, const unsigned char *data)
{
    uint32_t a, b, c, d, e, t;
    int i;

    for (i = 0; i < 16; i++) {
        ctx->w[i] = (data[i*4] << 24) | (data[i*4+1] << 16) |
                    (data[i*4+2] << 8) | data[i*4+3];
    }

    for (i = 16; i < 80; i++) {
        ctx->w[i] = SHA1_ROTL(ctx->w[i-3] ^ ctx->w[i-8] ^ 
                             ctx->w[i-14] ^ ctx->w[i-16], 1);
    }

    a = ctx->h[0];
    b = ctx->h[1];
    c = ctx->h[2];
    d = ctx->h[3];
    e = ctx->h[4];

    for (i = 0; i < 80; i++) {
        if (i < 20) {
            t = SHA1_ROTL(a, 5) + SHA1_F1(b, c, d) + e + ctx->w[i] + 0x5A827999;
        } else if (i < 40) {
            t = SHA1_ROTL(a, 5) + SHA1_F2(b, c, d) + e + ctx->w[i] + 0x6ED9EBA1;
        } else if (i < 60) {
            t = SHA1_ROTL(a, 5) + SHA1_F3(b, c, d) + e + ctx->w[i] + 0x8F1BBCDC;
        } else {
            t = SHA1_ROTL(a, 5) + SHA1_F2(b, c, d) + e + ctx->w[i] + 0xCA62C1D6;
        }

        e = d;
        d = c;
        c = SHA1_ROTL(b, 30);
        b = a;
        a = t;
    }

    ctx->h[0] += a;
    ctx->h[1] += b;
    ctx->h[2] += c;
    ctx->h[3] += d;
    ctx->h[4] += e;
}

static void sha1_update(sha1_ctx_t *ctx, const unsigned char *data, 
                        unsigned long len)
{
    unsigned long i;

    for (i = 0; i < len; i++) {
        ctx->size += 8;
        if ((ctx->size & 0xFFFFFFF8) == 512) {
            /* Process a 64-byte block */
            ctx->size &= 0x7;
        }
    }

    memcpy(ctx->w, data, len);
    sha1_process_block(ctx, (unsigned char *)ctx->w);
}

static void sha1_final(sha1_ctx_t *ctx, unsigned char *digest)
{
    int i;

    for (i = 0; i < 20; i++) {
        digest[i] = (ctx->h[i/4] >> (24 - (i%4)*8)) & 0xFF;
    }
}

static void sha1_hash(const unsigned char *data, unsigned long len, 
                      unsigned char *digest)
{
    sha1_ctx_t ctx;

    sha1_init(&ctx);
    sha1_update(&ctx, data, len);
    sha1_final(&ctx, digest);
}

/* Simple DES-like cipher for educational purposes */
static void des_encrypt_block(const unsigned char *key, const unsigned char *input,
                             unsigned char *output)
{
    int i;
    unsigned char temp[8];

    /* Simplified: XOR-based encryption with key scheduling */
    for (i = 0; i < 8; i++) {
        temp[i] = input[i] ^ key[i % 8];
        temp[i] = ((temp[i] << 1) | (temp[i] >> 7)) ^ key[(i+1) % 8];
    }

    for (i = 0; i < 8; i++) {
        output[i] = temp[i] ^ key[(7-i) % 8];
    }
}

static void des_decrypt_block(const unsigned char *key, const unsigned char *input,
                             unsigned char *output)
{
    int i;
    unsigned char temp[8];

    for (i = 0; i < 8; i++) {
        temp[i] = input[i] ^ key[(7-i) % 8];
    }

    for (i = 0; i < 8; i++) {
        output[i] = ((temp[i] >> 1) | (temp[i] << 7)) ^ key[(i+1) % 8];
        output[i] ^= key[i % 8];
    }
}

/* ioctl handler */
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct des_request des_req;
    struct sha1_request sha1_req;
    unsigned long i, blocks;
    unsigned char *input_buf;
    unsigned char *output_buf;
    int ret = 0;

    switch (cmd) {
        case CRYPTO_IOCTL_DES_ENCRYPT:
            if (copy_from_user(&des_req, (void __user *)arg, sizeof(des_req))) {
                return -EFAULT;
            }

            if (des_req.input_len > MAX_CRYPTO_DATA || 
                des_req.input_len % DES_BLOCK_SIZE != 0) {
                return -EINVAL;
            }

            blocks = des_req.input_len / DES_BLOCK_SIZE;
            for (i = 0; i < blocks; i++) {
                des_encrypt_block(des_req.key,
                                &des_req.input[i * DES_BLOCK_SIZE],
                                &des_req.output[i * DES_BLOCK_SIZE]);
            }
            des_req.output_len = des_req.input_len;

            if (copy_to_user((void __user *)arg, &des_req, sizeof(des_req))) {
                return -EFAULT;
            }
            break;

        case CRYPTO_IOCTL_DES_DECRYPT:
            if (copy_from_user(&des_req, (void __user *)arg, sizeof(des_req))) {
                return -EFAULT;
            }

            if (des_req.input_len > MAX_CRYPTO_DATA || 
                des_req.input_len % DES_BLOCK_SIZE != 0) {
                return -EINVAL;
            }

            blocks = des_req.input_len / DES_BLOCK_SIZE;
            for (i = 0; i < blocks; i++) {
                des_decrypt_block(des_req.key,
                                &des_req.input[i * DES_BLOCK_SIZE],
                                &des_req.output[i * DES_BLOCK_SIZE]);
            }
            des_req.output_len = des_req.input_len;

            if (copy_to_user((void __user *)arg, &des_req, sizeof(des_req))) {
                return -EFAULT;
            }
            break;

        case CRYPTO_IOCTL_SHA1_HASH:
            if (copy_from_user(&sha1_req, (void __user *)arg, sizeof(sha1_req))) {
                return -EFAULT;
            }

            if (sha1_req.input_len > SHA1_MAX_INPUT) {
                return -EINVAL;
            }

            sha1_hash(sha1_req.input, sha1_req.input_len, sha1_req.digest);

            if (copy_to_user((void __user *)arg, &sha1_req, sizeof(sha1_req))) {
                return -EFAULT;
            }
            break;

        default:
            return -EINVAL;
    }

    return ret;
}

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Chat Crypto: device opened\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Chat Crypto: device closed\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .unlocked_ioctl = dev_ioctl,
};

static int __init dev_init(void)
{
    int ret;

    printk(KERN_INFO "Chat Crypto Driver: Initializing...\n");

    /* Allocate device number */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "Failed to allocate device number\n");
        return ret;
    }

    /* Create device class */
    dev_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(dev_class)) {
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ALERT "Failed to create device class\n");
        return PTR_ERR(dev_class);
    }

    /* Create device */
    dev_device = device_create(dev_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(dev_device)) {
        class_destroy(dev_class);
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ALERT "Failed to create device\n");
        return PTR_ERR(dev_device);
    }

    /* Initialize and add the character device */
    cdev_init(&dev_cdev, &fops);
    dev_cdev.owner = THIS_MODULE;

    ret = cdev_add(&dev_cdev, dev_num, 1);
    if (ret < 0) {
        device_destroy(dev_class, dev_num);
        class_destroy(dev_class);
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ALERT "Failed to add cdev\n");
        return ret;
    }

    printk(KERN_INFO "Chat Crypto Driver: Successfully initialized\n");
    printk(KERN_INFO "Device created at /dev/%s (major: %d, minor: 0)\n",
           DEVICE_NAME, MAJOR(dev_num));

    return 0;
}

static void __exit dev_exit(void)
{
    cdev_del(&dev_cdev);
    device_destroy(dev_class, dev_num);
    class_destroy(dev_class);
    unregister_chrdev_region(dev_num, 1);

    printk(KERN_INFO "Chat Crypto Driver: Unloaded\n");
}

module_init(dev_init);
module_exit(dev_exit);
