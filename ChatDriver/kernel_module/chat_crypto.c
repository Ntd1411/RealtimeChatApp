#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/string.h>

#define CRYPTO_IOCTL_SHA1    _IOWR('c', 1, struct crypto_req)
#define CRYPTO_IOCTL_DES_ENC _IOWR('c', 2, struct crypto_req)
#define CRYPTO_IOCTL_DES_DEC _IOWR('c', 3, struct crypto_req)

// Cau truc goi tin giao tiep giua Node.js (App) va Kernel
struct crypto_req {
    char data[256];
    int data_len;
    char key[8];    // Key 8 byte cho DES
    char result[256];
    int result_len;
};

// Ham bam SHA1
static int do_sha1(struct crypto_req *req) {
    struct crypto_hash *tfm;
    struct hash_desc desc;
    struct scatterlist sg;

    tfm = crypto_alloc_hash("sha1", 0, CRYPTO_ALG_ASYNC);
    if (IS_ERR(tfm)) return PTR_ERR(tfm);

    desc.tfm = tfm;
    desc.flags = 0;

    sg_init_one(&sg, req->data, req->data_len);
    crypto_hash_init(&desc);
    crypto_hash_update(&desc, &sg, req->data_len);
    crypto_hash_final(&desc, req->result);

    req->result_len = crypto_hash_digestsize(tfm);
    crypto_free_hash(tfm);
    return 0;
}

// Ham ma hoa / giai ma DES
static int do_des(struct crypto_req *req, int encrypt) {
    struct crypto_blkcipher *tfm;
    struct blkcipher_desc desc;
    struct scatterlist sg_in, sg_out;
    int ret, crypt_len = req->data_len;

    // Thuat toan DES khoi (ECB) yeu cau du lieu phai là boi so cua 8 byte.
    if (crypt_len % 8 != 0) {
        crypt_len = ((crypt_len / 8) + 1) * 8;
        memset(req->data + req->data_len, 0, crypt_len - req->data_len); // Dem so 0 vao cuoi
    }

    tfm = crypto_alloc_blkcipher("ecb(des)", 0, CRYPTO_ALG_ASYNC);
    if (IS_ERR(tfm)) return PTR_ERR(tfm);

    ret = crypto_blkcipher_setkey(tfm, req->key, 8);
    if (ret) {
        crypto_free_blkcipher(tfm);
        return ret;
    }

    desc.tfm = tfm;
    desc.flags = 0;

    sg_init_one(&sg_in, req->data, crypt_len);
    sg_init_one(&sg_out, req->result, crypt_len);

    if (encrypt) ret = crypto_blkcipher_encrypt(&desc, &sg_out, &sg_in, crypt_len);
    else         ret = crypto_blkcipher_decrypt(&desc, &sg_out, &sg_in, crypt_len);

    req->result_len = crypt_len;
    crypto_free_blkcipher(tfm);
    return ret;
}

// Ham lang nghe su kien tu User-space
static long crypto_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct crypto_req req;

    if (copy_from_user(&req, (struct crypto_req *)arg, sizeof(req)))
        return -EFAULT;

    switch (cmd) {
        case CRYPTO_IOCTL_SHA1:     do_sha1(&req); break;
        case CRYPTO_IOCTL_DES_ENC:  do_des(&req, 1); break;
        case CRYPTO_IOCTL_DES_DEC:  do_des(&req, 0); break;
        default: return -EINVAL;
    }

    if (copy_to_user((struct crypto_req *)arg, &req, sizeof(req)))
        return -EFAULT;

    return 0;
}

static const struct file_operations crypto_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = crypto_ioctl,
};
// Dung miscdevice de tu dong tao file /dev/kma_crypto, do phai dung lenh mknod
static struct miscdevice crypto_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "kma_crypto",
    .fops = &crypto_fops,
};

static int __init crypto_init(void) {
    misc_register(&crypto_misc);
    printk(KERN_INFO "KMA Crypto Driver Loaded: /dev/kma_crypto\n");
    return 0;
}

static void __exit crypto_exit(void) {
    misc_deregister(&crypto_misc);
    printk(KERN_INFO "KMA Crypto Driver Unloaded\n");
}

module_init(crypto_init);
module_exit(crypto_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("KMA Chatty Team");