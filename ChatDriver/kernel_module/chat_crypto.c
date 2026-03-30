#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/string.h>

#define CRYPTO_IOCTL_SHA1    _IOWR('c', 1, struct crypto_req)
#define CRYPTO_IOCTL_DES_ENC _IOWR('c', 2, struct crypto_req)
#define CRYPTO_IOCTL_DES_DEC _IOWR('c', 3, struct crypto_req)

// Device configuration
#define DEVICE_NAME "kma_crypto"
#define CLASS_NAME  "kma_crypto_class"

// Device state (global)
static int dev_major = 0;
static struct class *crypto_class = NULL;
static struct device *crypto_device = NULL;
static struct cdev crypto_cdev;

// Cau truc goi tin giao tiep giua App va Kernel
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

// Device open handler
static int crypto_open(struct inode *inode, struct file *file) {
    printk(KERN_NOTICE "[KMA] Device opened\n");
    return 0;
}

// Device release handler
static int crypto_release(struct inode *inode, struct file *file) {
    printk(KERN_NOTICE "[KMA] Device closed\n");
    return 0;
}

static const struct file_operations crypto_fops = {
    .owner = THIS_MODULE,
    .open = crypto_open,
    .release = crypto_release,
    .unlocked_ioctl = crypto_ioctl,
};

static int __init crypto_init(void) {
    dev_t dev;
    int ret;

    printk(KERN_NOTICE "[KMA] Initializing crypto driver...\n");

    // Cấp phát device number động
    ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "[KMA] Failed to allocate device number: %d\n", ret);
        return ret;
    }

    dev_major = MAJOR(dev);
    printk(KERN_NOTICE "[KMA] Device number allocated: MAJOR=%d, MINOR=%d\n", dev_major, MINOR(dev));

    // Khởi tạo character device
    cdev_init(&crypto_cdev, &crypto_fops);
    crypto_cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&crypto_cdev, dev, 1);
    if (ret < 0) {
        printk(KERN_ERR "[KMA] Failed to add character device: %d\n", ret);
        unregister_chrdev_region(dev, 1);
        return ret;
    }
    printk(KERN_NOTICE "[KMA] Character device registered\n");

    // Tạo device class
    crypto_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(crypto_class)) {
        printk(KERN_ERR "[KMA] Failed to create device class\n");
        cdev_del(&crypto_cdev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(crypto_class);
    }
    printk(KERN_NOTICE "[KMA] Device class created: %s\n", CLASS_NAME);

    // Tạo device node
    crypto_device = device_create(crypto_class, NULL, dev, NULL, DEVICE_NAME);
    if (IS_ERR(crypto_device)) {
        printk(KERN_ERR "[KMA] Failed to create device node\n");
        class_destroy(crypto_class);
        cdev_del(&crypto_cdev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(crypto_device);
    }
    printk(KERN_NOTICE "[KMA] Device node created: /dev/%s\n", DEVICE_NAME);
    printk(KERN_NOTICE "[KMA] ===== Crypto driver initialized successfully! =====\n");
    printk(KERN_NOTICE "[KMA] Device: /dev/%s (Major: %d)\n", DEVICE_NAME, dev_major);
    
    return 0;
}

static void __exit crypto_exit(void) {
    dev_t dev = MKDEV(dev_major, 0);

    printk(KERN_NOTICE "[KMA] Cleaning up crypto driver...\n");
    
    // Xóa device node
    device_destroy(crypto_class, dev);
    printk(KERN_NOTICE "[KMA] Device node destroyed\n");
    
    // Xóa device class
    class_destroy(crypto_class);
    printk(KERN_NOTICE "[KMA] Device class destroyed\n");
    
    // Xóa character device
    cdev_del(&crypto_cdev);
    printk(KERN_NOTICE "[KMA] Character device removed\n");
    
    // Unregister device number
    unregister_chrdev_region(dev, 1);
    printk(KERN_NOTICE "[KMA] Device number unregistered: MAJOR=%d\n", dev_major);
    printk(KERN_NOTICE "[KMA] Crypto driver unloaded\n");
}

module_init(crypto_init);
module_exit(crypto_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("KMA Chatty Team");
MODULE_DESCRIPTION("KMA Crypto Driver - SHA1 and DES via kernel crypto API");
MODULE_VERSION("2.0");