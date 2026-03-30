#include "cryptodriver_qt.h"
#include <cryptodriver.h>
#include <QDebug>

CryptoDriverQt::CryptoDriverQt()
{
    driver = crypto_driver_new();
    if (!driver) {
        qWarning() << "Failed to initialize crypto driver";
    }
}

CryptoDriverQt::~CryptoDriverQt()
{
    if (driver) {
        crypto_driver_free(driver);
        driver = nullptr;
    }
}

bool CryptoDriverQt::isOpen() const
{
    return crypto_driver_is_open(driver) != 0;
}

QByteArray CryptoDriverQt::sha1Hash(const QByteArray &data)
{
    if (!driver) {
        qWarning() << "Crypto driver not initialized";
        return QByteArray();
    }

    unsigned char digest[SHA1_DIGEST_SIZE];
    int ret = crypto_driver_sha1(driver, 
                                (const unsigned char *)data.constData(),
                                data.length(),
                                digest);

    if (ret != 0) {
        qWarning() << "SHA1 failed with error:" << ret;
        return QByteArray();
    }

    return QByteArray((const char *)digest, SHA1_DIGEST_SIZE);
}

QByteArray CryptoDriverQt::sha1Hash(const QString &str)
{
    return sha1Hash(str.toUtf8());
}

QByteArray CryptoDriverQt::desEncrypt(const QByteArray &key, const QByteArray &plaintext)
{
    if (!driver) {
        qWarning() << "Crypto driver not initialized";
        return QByteArray();
    }

    if (key.length() != 8) {
        qWarning() << "DES key must be 8 bytes, got" << key.length();
        return QByteArray();
    }

    unsigned char ciphertext[MAX_CRYPTO_DATA];
    size_t ciphertext_len;

    int ret = crypto_driver_des_encrypt(driver,
                                       (const unsigned char *)key.constData(),
                                       (const unsigned char *)plaintext.constData(),
                                       plaintext.length(),
                                       ciphertext,
                                       &ciphertext_len);

    if (ret != 0) {
        qWarning() << "DES encrypt failed with error:" << ret;
        return QByteArray();
    }

    return QByteArray((const char *)ciphertext, ciphertext_len);
}

QByteArray CryptoDriverQt::desDecrypt(const QByteArray &key, const QByteArray &ciphertext)
{
    if (!driver) {
        qWarning() << "Crypto driver not initialized";
        return QByteArray();
    }

    if (key.length() != 8) {
        qWarning() << "DES key must be 8 bytes, got" << key.length();
        return QByteArray();
    }

    unsigned char plaintext[MAX_CRYPTO_DATA];
    size_t plaintext_len;

    int ret = crypto_driver_des_decrypt(driver,
                                       (const unsigned char *)key.constData(),
                                       (const unsigned char *)ciphertext.constData(),
                                       ciphertext.length(),
                                       plaintext,
                                       &plaintext_len);

    if (ret != 0) {
        qWarning() << "DES decrypt failed with error:" << ret;
        return QByteArray();
    }

    return QByteArray((const char *)plaintext, plaintext_len);
}
