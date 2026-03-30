#include "socketclient.h"
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QTimer>

SocketClient::SocketClient(const QString &serverUrl, const QString &t, QObject *parent)
    : QObject(parent), server_url(serverUrl), token(t), message_counter(0)
{
    webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    
    QObject::connect(webSocket, SIGNAL(connected()), this, SLOT(onConnected()));
    QObject::connect(webSocket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    QObject::connect(webSocket, SIGNAL(textMessageReceived(QString)), 
                     this, SLOT(onTextMessageReceived(QString)));
    QObject::connect(webSocket, SIGNAL(error(QAbstractSocket::SocketError)), 
                     this, SLOT(onError(QAbstractSocket::SocketError)));
}

SocketClient::~SocketClient()
{
    if (webSocket && webSocket->isValid()) {
        webSocket->close();
    }
}

void SocketClient::connect()
{
    // Convert http:// to ws:// and https:// to wss://
    QString wsUrl = server_url;
    if (wsUrl.startsWith("https://")) {
        wsUrl.replace(0, 8, "wss://");
    } else if (wsUrl.startsWith("http://")) {
        wsUrl.replace(0, 7, "ws://");
    }
    
    // Add auth token to query
    if (!wsUrl.endsWith("/")) wsUrl += "/";
    wsUrl += "socket.io/?transport=websocket&token=" + token;
    
    qDebug() << "Connecting to:" << wsUrl;
    webSocket->open(QUrl(wsUrl));
}

void SocketClient::disconnect()
{
    if (webSocket && webSocket->isValid()) {
        webSocket->close();
    }
}

bool SocketClient::isConnected() const
{
    return webSocket && webSocket->isValid();
}

void SocketClient::sendMessage(const QString &receiverId, const QString &content)
{
    QJsonObject payload;
    payload["content"] = content;
    payload["receiverId"] = receiverId;
    
    QJsonArray arr;
    arr.append("send-message");
    arr.append(payload);
    arr.append(QJsonObject()); // acknowledgement callback
    
    QJsonDocument doc(arr);
    QString message = "4" + doc.toJson(QJsonDocument::Compact); // "4" = emit frame type
    
    if (webSocket && webSocket->isValid()) {
        webSocket->sendTextMessage(message);
        qDebug() << "Message sent to:" << receiverId;
    }
}

void SocketClient::markMessageSeen(const QString &senderId)
{
    QJsonObject payload;
    payload["senderId"] = senderId;
    
    QJsonArray arr;
    arr.append("seen-message");
    arr.append(payload);
    
    QJsonDocument doc(arr);
    QString message = "4" + doc.toJson(QJsonDocument::Compact);
    
    if (webSocket && webSocket->isValid()) {
        webSocket->sendTextMessage(message);
    }
}

void SocketClient::notifyTypingStart(const QString &receiverId)
{
    QJsonObject payload;
    payload["receiverId"] = receiverId;
    
    QJsonArray arr;
    arr.append("typing-start");
    arr.append(payload);
    
    QJsonDocument doc(arr);
    QString message = "4" + doc.toJson(QJsonDocument::Compact);
    
    if (webSocket && webSocket->isValid()) {
        webSocket->sendTextMessage(message);
    }
}

void SocketClient::notifyTypingStop(const QString &receiverId)
{
    QJsonObject payload;
    payload["receiverId"] = receiverId;
    
    QJsonArray arr;
    arr.append("typing-stop");
    arr.append(payload);
    
    QJsonDocument doc(arr);
    QString message = "4" + doc.toJson(QJsonDocument::Compact);
    
    if (webSocket && webSocket->isValid()) {
        webSocket->sendTextMessage(message);
    }
}

void SocketClient::onConnected()
{
    qDebug() << "Socket connected";
    emit connected();
}

void SocketClient::onDisconnected()
{
    qDebug() << "Socket disconnected";
    emit disconnected();
}

void SocketClient::onTextMessageReceived(const QString &message)
{
    qDebug() << "Received:" << message;
    parseSocketMessage(message);
}

void SocketClient::onError(QAbstractSocket::SocketError error)
{
    qDebug() << "Socket error:" << error;
    emit this->error(webSocket->errorString());
}

void SocketClient::parseSocketMessage(const QString &message)
{
    if (message.isEmpty()) return;
    
    // Socket.IO protocol: first char is frame type
    // 4 = emit, 2 = connect, 0 = disconnect, etc.
    if (message[0] == '4') {
        // Emit message - parse JSON after "4"
        QString jsonStr = message.mid(1);
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        
        if (doc.isArray()) {
            QJsonArray arr = doc.array();
            if (arr.size() >= 2) {
                QString eventName = arr[0].toString();
                QJsonObject data = arr[1].toObject();
                
                if (eventName == "receive-message") {
                    emit messageReceived(data);
                } else if (eventName == "seen-message") {
                    emit messagesSeen(data["viewerId"].toString());
                } else if (eventName == "typing-start") {
                    emit typingStarted(
                        data["senderId"].toString(),
                        data["senderName"].toString()
                    );
                } else if (eventName == "typing-stop") {
                    emit typingStopped(data["senderId"].toString());
                } else if (eventName == "noti-online") {
                    emit onlineStatusChanged(data["id"].toString(), true);
                } else if (eventName == "noti-offline") {
                    emit onlineStatusChanged(data["id"].toString(), false);
                }
            }
        }
    } else if (message[0] == '2') {
        // Connect event
        qDebug() << "Socket connected - ready to use";
    }
}
