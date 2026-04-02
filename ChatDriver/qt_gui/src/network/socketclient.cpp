#include "socketclient.h"
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QTextStream>

void SocketClient::logToFile(const QString &msg)
{
    QFile file("chatclient.log");
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << "[SOCKET] " << msg << "\n";
        file.close();
    }
}

SocketClient::SocketClient(const QString &serverUrl, const QString &t, QObject *parent)
    : QObject(parent), server_url(serverUrl), token(t), message_counter(0), shouldReconnect(true)
{
    webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    
    QObject::connect(webSocket, SIGNAL(connected()), this, SLOT(onConnected()));
    QObject::connect(webSocket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    QObject::connect(webSocket, SIGNAL(textMessageReceived(QString)), 
                     this, SLOT(onTextMessageReceived(QString)));
    QObject::connect(webSocket, SIGNAL(error(QAbstractSocket::SocketError)), 
                     this, SLOT(onError(QAbstractSocket::SocketError)));
    
    // Setup reconnect timer
    reconnectTimer = new QTimer(this);
    QObject::connect(reconnectTimer, SIGNAL(timeout()), this, SLOT(onReconnectTimerTimeout()));
    
    logToFile("SocketClient initialized with server: " + serverUrl);
}

SocketClient::~SocketClient()
{
    shouldReconnect = false;
    if (reconnectTimer) {
        reconnectTimer->stop();
    }
    if (webSocket && webSocket->isValid()) {
        webSocket->close();
    }
    logToFile("SocketClient destroyed");
}

void SocketClient::connect()
{
    logToFile("Attempting to connect...");
    
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
    
    logToFile("Socket URL: " + wsUrl);
    webSocket->open(QUrl(wsUrl));
}

void SocketClient::disconnect()
{
    logToFile("Disconnecting...");
    shouldReconnect = false;
    if (reconnectTimer) {
        reconnectTimer->stop();
    }
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
    logToFile("Sending message to " + receiverId + ": " + content.left(100));
    
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
        logToFile("Message sent successfully");
    } else {
        logToFile("ERROR: Socket not connected, cannot send message");
        emit error("Socket không kết nối");
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
    logToFile("Socket connected successfully");
    if (reconnectTimer) {
        reconnectTimer->stop();
    }
    emit connected();
}

void SocketClient::onDisconnected()
{
    logToFile("Socket disconnected");
    emit disconnected();
    
    // Auto-reconnect after 5 seconds
    if (shouldReconnect && reconnectTimer) {
        logToFile("Scheduling reconnect in 5 seconds...");
        reconnectTimer->start(5000);
    }
}

void SocketClient::onTextMessageReceived(const QString &message)
{
    logToFile("Received message: " + message.left(200));
    parseSocketMessage(message);
}

void SocketClient::onError(QAbstractSocket::SocketError error)
{
    QString errorMsg = webSocket->errorString();
    logToFile("Socket error: " + errorMsg);
    emit this->error(errorMsg);
}

void SocketClient::onReconnectTimerTimeout()
{
    logToFile("Attempting to reconnect...");
    connect();
}

void SocketClient::parseSocketMessage(const QString &message)
{
    if (message.isEmpty()) {
        logToFile("Empty message received");
        return;
    }
    
    logToFile("===== SOCKET MESSAGE START =====");
    logToFile("Message length: " + QString::number(message.length()));
    logToFile("First 200 chars: " + message.left(200));
    logToFile("First char code: " + QString::number((int)message[0].toLatin1()));
    
    // Socket.IO protocol: first char is frame type
    // 0 = disconnect, 1 = connect, 2 = disconnect, 3 = ping, 4 = pong, 5 = message, 6 = upgrade, 7 = noop
    // For Socket.IO namespace: 0 = disconnect, 1 = connect, 2 = disconnect, 3 = error, 4 = ack, 5 = error
    
    char frameType = message[0].toLatin1();
    
    switch (frameType) {
        case '0': {
            logToFile("[FRAME-0] Disconnect: " + message.mid(1, 50));
            break;
        }
        case '1': {
            logToFile("[FRAME-1] Engine.IO Connect: " + message.mid(1, 50));
            break;
        }
        case '2': {
            logToFile("[FRAME-2] Ping");
            // Respond with pong (3)
            if (webSocket && webSocket->isValid()) {
                webSocket->sendTextMessage("3");
                logToFile("Sent pong response");
            }
            break;
        }
        case '3': {
            logToFile("[FRAME-3] Pong");
            break;
        }
        case '4': {
            logToFile("[FRAME-4] Emit message (Socket.IO event)");
            // Emit message - parse JSON after "4"
            QString jsonStr = message.mid(1);
            logToFile("JSON to parse: " + jsonStr.left(200));
            
            QJsonParseError jsonError;
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &jsonError);
            
            if (doc.isNull()) {
                logToFile("ERROR: JSON parse failed: " + jsonError.errorString());
                break;
            }
            
            if (!doc.isArray()) {
                logToFile("ERROR: JSON is not array");
                break;
            }
            
            QJsonArray arr = doc.array();
            if (arr.size() < 2) {
                logToFile("WARNING: Array size < 2: " + QString::number(arr.size()));
                break;
            }
            
            QString eventName = arr[0].toString();
            QJsonObject data = arr[1].toObject();
            
            logToFile("Event: " + eventName);
            
            if (eventName == "receive-message") {
                QString senderId = data["senderId"].toString();
                QString content = data["content"].toString();
                logToFile("Message from " + senderId + ": " + content.left(100));
                emit messageReceived(data);
            } else if (eventName == "seen-message") {
                QString viewerId = data["viewerId"].toString();
                logToFile("Message seen by " + viewerId);
                emit messagesSeen(viewerId);
            } else if (eventName == "typing-start") {
                QString senderId = data["senderId"].toString();
                logToFile("Typing started by " + senderId);
                emit typingStarted(senderId, data["senderName"].toString());
            } else if (eventName == "typing-stop") {
                QString senderId = data["senderId"].toString();
                logToFile("Typing stopped by " + senderId);
                emit typingStopped(senderId);
            } else if (eventName == "noti-online") {
                QString userId = data["id"].toString();
                logToFile("User online: " + userId);
                emit onlineStatusChanged(userId, true);
            } else if (eventName == "noti-offline") {
                QString userId = data["id"].toString();
                logToFile("User offline: " + userId);
                emit onlineStatusChanged(userId, false);
            } else {
                logToFile("Unknown event: " + eventName);
            }
            break;
        }
        case '5': {
            logToFile("[FRAME-5] Ack");
            break;
        }
        case '6': {
            logToFile("[FRAME-6] Error: " + message.mid(1, 100));
            break;
        }
        default: {
            logToFile("[UNKNOWN FRAME] Type: " + QString::number((int)frameType) + " Content: " + message.left(100));
        }
    }
    
    logToFile("===== SOCKET MESSAGE END =====");
}
