/*
 * ChatDriver Kernel Crypto Module
 * Uses custom SHA1 and DES implementations (no kernel crypto API)
 * For CentOS 6 (Linux kernel 2.6.32+)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include "../include/crypto_module.h"
#include "sha1.h"
#include "des.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChatDriver");
MODULE_DESCRIPTION("Kernel crypto module with custom SHA1 and DES");
MODULE_VERSION("1.0");

#define DEVICE_NAME "chat_crypto"
#define CLASS_NAME "chat_crypto_class"

/* Module state */
static int major_number;
static struct class* crypto_class = NULL;
static struct device* crypto_device = NULL;
static struct cdev crypto_cdev;

/**
 * Device IOCTL Handler
 */
static long chat_crypto_ioctl(struct file *filp,
                               unsigned int cmd,
                               unsigned long arg)
{
    struct sha1_request sha1_req;
    struct des_request des_req;
    void __user *argp = (void __user *)arg;
    unsigned char *kernel_input = NULL;
    unsigned char *kernel_output = NULL;
    int ret;

    switch (cmd) {
        case CRYPTO_IOCTL_SHA1_HASH:
            /* Copy SHA1 request from user space */
            if (copy_from_user(&sha1_req, argp, sizeof(sha1_req))) {
                return -EFAULT;
            }

            /* Validate input length */
            if (sha1_req.input_len > SHA1_MAX_INPUT) {
                return -EINVAL;
            }

            /* Perform SHA1 hash */
            sha1_hash(sha1_req.input, sha1_req.input_len, sha1_req.digest);

            /* Copy result back to user space */
            if (copy_to_user(argp, &sha1_req, sizeof(sha1_req))) {
                return -EFAULT;
            }

            return CRYPTO_OK;

        case CRYPTO_IOCTL_DES_ENCRYPT:
            /* Copy DES request from user space */
            if (copy_from_user(&des_req, argp, sizeof(des_req))) {
                return -EFAULT;
            }

            /* Validate input */
            if (des_req.input_len > DES_MAX_INPUT) {
                return -EINVAL;
            }

            if (des_req.input_len % DES_BLOCK_SIZE != 0) {
                return -EINVAL;
            }

            /* Perform DES encryption */
            {
                struct des_context ctx;
                des_init(&ctx, des_req.key);
                des_encrypt_ecb(&ctx, des_req.input, des_req.output, des_req.input_len);
            }

            des_req.output_len = des_req.input_len;

            /* Copy result back to user space */
            if (copy_to_user(argp, &des_req, sizeof(des_req))) {
                return -EFAULT;
            }

            return CRYPTO_OK;

        case CRYPTO_IOCTL_DES_DECRYPT:
            /* Copy DES request from user space */
            if (copy_from_user(&des_req, argp, sizeof(des_req))) {
                return -EFAULT;
            }

            /* Validate input */
            if (des_req.input_len > DES_MAX_INPUT) {
                return -EINVAL;
            }

            if (des_req.input_len % DES_BLOCK_SIZE != 0) {
                return -EINVAL;
            }

            /* Perform DES decryption */
            {
                struct des_context ctx;
                des_init(&ctx, des_req.key);
                des_decrypt_ecb(&ctx, des_req.input, des_req.output, des_req.input_len);
            }

            des_req.output_len = des_req.input_len;

            /* Copy result back to user space */
            if (copy_to_user(argp, &des_req, sizeof(des_req))) {
                return -EFAULT;
            }

            return CRYPTO_OK;

        default:
            return -EINVAL;
    }
}

/**
 * Device file operations structure
 */
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = chat_crypto_ioctl,
};

/**
 * Module initialization
 */
static int __init chat_crypto_init(void)
{
    dev_t dev_num;
    int ret;

    printk(KERN_INFO "ChatDriver: Loading kernel crypto module\n");

    /* Allocate character device number */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "ChatDriver: Failed to allocate device number\n");
        return ret;
    }

    major_number = MAJOR(dev_num);
    printk(KERN_INFO "ChatDriver: Allocated major number %d\n", major_number);

    /* Create device class */
    crypto_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(crypto_class)) {
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ERR "ChatDriver: Failed to create device class\n");
        return PTR_ERR(crypto_class);
    }

    /* Create device node */
    crypto_device = device_create(crypto_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(crypto_device)) {
        class_destroy(crypto_class);
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ERR "ChatDriver: Failed to create device\n");
        return PTR_ERR(crypto_device);
    }

    /* Initialize and add character device */
    cdev_init(&crypto_cdev, &fops);
    crypto_cdev.owner = THIS_MODULE;
    ret = cdev_add(&crypto_cdev, dev_num, 1);
    if (ret < 0) {
        device_destroy(crypto_class, dev_num);
        class_destroy(crypto_class);
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ERR "ChatDriver: Failed to add character device\n");
        return ret;
    }

    printk(KERN_INFO "ChatDriver: Kernel module loaded successfully\n");
    printk(KERN_INFO "ChatDriver: Device /dev/%s is ready\n", DEVICE_NAME);
    printk(KERN_INFO "ChatDriver: SHA1 and custom DES enabled\n");

    return 0;
}

/**
 * Module cleanup
 */
static void __exit chat_crypto_exit(void)
{
    dev_t dev_num = MKDEV(major_number, 0);

    printk(KERN_INFO "ChatDriver: Unloading kernel crypto module\n");

    /* Remove character device */
    cdev_del(&crypto_cdev);

    /* Destroy device node */
    device_destroy(crypto_class, dev_num);

    /* Destroy device class */
    class_destroy(crypto_class);

    /* Unregister character device region */
    unregister_chrdev_region(dev_num, 1);

    printk(KERN_INFO "ChatDriver: Module unloaded successfully\n");
}

module_init(chat_crypto_init);
module_exit(chat_crypto_exit);
