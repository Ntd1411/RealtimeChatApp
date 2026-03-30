#include "kernel_crypto_client.h"
#include <crypto_module.h>
#include <QDebug>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <cstring>

KernelCryptoClient::KernelCryptoClient() : device_fd(-1)
{
    openDevice();
}

KernelCryptoClient::~KernelCryptoClient()
{
    closeDevice();
}

bool KernelCryptoClient::openDevice()
{
    if (device_fd >= 0) {
        return true;  // Already open
    }

    device_fd = open(CRYPTO_DEVICE_FILE, O_RDWR);
    if (device_fd < 0) {
        int err = errno;
        qWarning() << "Failed to open crypto device" << CRYPTO_DEVICE_FILE 
                   << "- Error:" << strerror(err);
        return false;
    }

    qDebug() << "Crypto device opened successfully";
    return true;
}

void KernelCryptoClient::closeDevice()
{
    if (device_fd >= 0) {
        close(device_fd);
        device_fd = -1;
    }
}

bool KernelCryptoClient::isOpen() const
{
    return device_fd >= 0;
}

QByteArray KernelCryptoClient::sha1Hash(const QByteArray &data)
{
    if (device_fd < 0) {
        qWarning() << "Crypto device not open";
        return QByteArray();
    }

    if (data.length() > SHA1_MAX_INPUT) {
        qWarning() << "Input data too large for SHA1:" << data.length() << ">" << SHA1_MAX_INPUT;
        return QByteArray();
    }

    // Prepare request structure
    struct sha1_request req;
    memset(&req, 0, sizeof(req));
    memcpy(req.input, data.constData(), data.length());
    req.input_len = data.length();

    // Call kernel via ioctl
    int ret = ioctl(device_fd, CRYPTO_IOCTL_SHA1_HASH, &req);
    if (ret < 0) {
        int err = errno;
        qWarning() << "SHA1 ioctl failed - Error:" << strerror(err);
        return QByteArray();
    }

    // Return digest
    return QByteArray((const char *)req.digest, SHA1_DIGEST_SIZE);
}

QByteArray KernelCryptoClient::sha1Hash(const QString &str)
{
    return sha1Hash(str.toUtf8());
}

QByteArray KernelCryptoClient::desEncrypt(const QByteArray &key, const QByteArray &plaintext)
{
    if (device_fd < 0) {
        qWarning() << "Crypto device not open";
        return QByteArray();
    }

    if (key.length() != DES_KEY_SIZE) {
        qWarning() << "DES key must be 8 bytes, got" << key.length();
        return QByteArray();
    }

    if (plaintext.length() > MAX_CRYPTO_DATA) {
        qWarning() << "Plaintext too large for DES:" << plaintext.length() << ">" << MAX_CRYPTO_DATA;
        return QByteArray();
    }

    if (plaintext.length() % 8 != 0) {
        qWarning() << "Plaintext length must be multiple of 8, got" << plaintext.length();
        return QByteArray();
    }

    // Prepare request structure
    struct des_request req;
    memset(&req, 0, sizeof(req));
    memcpy(req.key, key.constData(), DES_KEY_SIZE);
    memcpy(req.input, plaintext.constData(), plaintext.length());
    req.input_len = plaintext.length();
    req.output_len = 0;
    req.mode = 0;  // 0 = encrypt

    // Call kernel via ioctl
    int ret = ioctl(device_fd, CRYPTO_IOCTL_DES_ENCRYPT, &req);
    if (ret < 0) {
        int err = errno;
        qWarning() << "DES encrypt ioctl failed - Error:" << strerror(err);
        return QByteArray();
    }

    // Return ciphertext
    return QByteArray((const char *)req.output, req.output_len);
}

QByteArray KernelCryptoClient::desDecrypt(const QByteArray &key, const QByteArray &ciphertext)
{
    if (device_fd < 0) {
        qWarning() << "Crypto device not open";
        return QByteArray();
    }

    if (key.length() != DES_KEY_SIZE) {
        qWarning() << "DES key must be 8 bytes, got" << key.length();
        return QByteArray();
    }

    if (ciphertext.length() > MAX_CRYPTO_DATA) {
        qWarning() << "Ciphertext too large for DES:" << ciphertext.length() << ">" << MAX_CRYPTO_DATA;
        return QByteArray();
    }

    if (ciphertext.length() % 8 != 0) {
        qWarning() << "Ciphertext length must be multiple of 8, got" << ciphertext.length();
        return QByteArray();
    }

    // Prepare request structure
    struct des_request req;
    memset(&req, 0, sizeof(req));
    memcpy(req.key, key.constData(), DES_KEY_SIZE);
    memcpy(req.input, ciphertext.constData(), ciphertext.length());
    req.input_len = ciphertext.length();
    req.output_len = 0;
    req.mode = 1;  // 1 = decrypt

    // Call kernel via ioctl
    int ret = ioctl(device_fd, CRYPTO_IOCTL_DES_DECRYPT, &req);
    if (ret < 0) {
        int err = errno;
        qWarning() << "DES decrypt ioctl failed - Error:" << strerror(err);
        return QByteArray();
    }

    // Return plaintext
    return QByteArray((const char *)req.output, req.output_len);
}
