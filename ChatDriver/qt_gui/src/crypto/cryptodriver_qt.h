#ifndef CRYPTODRIVER_QT_H
#define CRYPTODRIVER_QT_H

#include <QString>
#include <QByteArray>

typedef struct crypto_driver CryptoDriver; // Opaque C type

class CryptoDriverQt {
public:
    CryptoDriverQt();
    ~CryptoDriverQt();

    bool isOpen() const;
    
    // SHA1 hash using kernel module
    QByteArray sha1Hash(const QByteArray &data);
    QByteArray sha1Hash(const QString &str);
    
    // DES encrypt using kernel module
    QByteArray desEncrypt(const QByteArray &key, const QByteArray &plaintext);
    
    // DES decrypt using kernel module
    QByteArray desDecrypt(const QByteArray &key, const QByteArray &ciphertext);

private:
    CryptoDriver *driver;
};

#endif // CRYPTODRIVER_QT_H
