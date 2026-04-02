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
    
    // Emit events
    void sendMessage(const QString &receiverId, const QString &content);
    void markMessageSeen(const QString &senderId);
    void notifyTypingStart(const QString &receiverId);
    void notifyTypingStop(const QString &receiverId);

signals:
    void connected();
    void disconnected();
    void messageReceived(const QJsonObject &message);
    void messagesSeen(const QString &viewerId);
    void typingStarted(const QString &senderId, const QString &senderName);
    void typingStopped(const QString &senderId);
    void error(const QString &message);
    void onlineStatusChanged(const QString &userId, bool isOnline);
    void onlineListReceived(const QJsonArray &userIds);

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
