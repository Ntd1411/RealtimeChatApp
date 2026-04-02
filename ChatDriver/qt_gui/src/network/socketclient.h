#ifndef SOCKETCLIENT_H
#define SOCKETCLIENT_H

#include <QString>
#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

class SocketClient : public QObject {
    Q_OBJECT

public:
    SocketClient(const QString &serverUrl, const QString &token, QObject *parent = 0);
    ~SocketClient();
    
    void connect();
    void disconnect();
    bool isConnected() const;
    void sendMessage(const QString &receiverId, const QString &content);

signals:
    void connected();
    void disconnected();
    void messageReceived(const QJsonObject &message);
    void error(const QString &message);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onError(QAbstractSocket::SocketError error);
    void onReconnectTimerTimeout();
    void onAuthTimeoutTimerTimeout();

private:
    void parseSocketMessage(const QString &message);
    void logToFile(const QString &msg);
    void sendAuthMessage();
    
    QWebSocket *webSocket;
    QString server_url;
    QString token;
    long long message_counter;
    QTimer *reconnectTimer;
    QTimer *authTimeoutTimer;
    bool shouldReconnect;
    bool authenticated;
    bool engineioReady;
};

#endif // SOCKETCLIENT_H
