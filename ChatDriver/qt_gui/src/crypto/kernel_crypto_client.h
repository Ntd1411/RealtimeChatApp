#ifndef KERNEL_CRYPTO_CLIENT_H
#define KERNEL_CRYPTO_CLIENT_H

#include <QString>
#include <QByteArray>

/**
 * Direct kernel driver client for crypto operations
 * Communicates with /dev/chat_crypto using ioctl syscalls
 */
class KernelCryptoClient {
public:
    KernelCryptoClient();
    ~KernelCryptoClient();

    bool isOpen() const;
    
    // SHA1 hash using kernel module
    QByteArray sha1Hash(const QByteArray &data);
    QByteArray sha1Hash(const QString &str);
    
    // DES encrypt using kernel module
    QByteArray desEncrypt(const QByteArray &key, const QByteArray &plaintext);
    
    // DES decrypt using kernel module
    QByteArray desDecrypt(const QByteArray &key, const QByteArray &ciphertext);

private:
    int device_fd;  // File descriptor for /dev/chat_crypto
    bool openDevice();
    void closeDevice();
};

#endif // KERNEL_CRYPTO_CLIENT_H
