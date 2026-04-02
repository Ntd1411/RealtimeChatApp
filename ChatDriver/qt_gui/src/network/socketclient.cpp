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
    : QObject(parent), server_url(serverUrl), token(t), message_counter(0), shouldReconnect(true), authenticated(false), engineioReady(false)
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
    
    // Setup auth timeout timer - wait max 5 seconds for server auth response
    authTimeoutTimer = new QTimer(this);
    authTimeoutTimer->setSingleShot(true);
    QObject::connect(authTimeoutTimer, SIGNAL(timeout()), this, SLOT(onAuthTimeoutTimerTimeout()));
    
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
    authenticated = false;
    
    QString wsUrl = server_url;
    if (wsUrl.startsWith("https://")) {
        wsUrl.replace(0, 8, "wss://");
    } else if (wsUrl.startsWith("http://")) {
        wsUrl.replace(0, 7, "ws://");
    }
    
    if (!wsUrl.endsWith("/")) wsUrl += "/";
    wsUrl += "socket.io/?EIO=4&transport=websocket";
    
    if (token.isEmpty()) {
        emit error("Token not found");
    }
    
    logToFile("[CONNECT] Connecting...");
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
    
    if (!authenticated) {
        logToFile("ERROR: Not authenticated, cannot send message");
        emit error("Chưa kết nối được server");
        return;
    }
    
    QJsonObject payload;
    payload["content"] = content;  // Sẽ được encode thành UTF-8 tự động
    payload["receiverId"] = receiverId;
    
    QJsonArray arr;
    arr.append("send-message");
    arr.append(payload);
    
    QJsonDocument doc(arr);
    QString message = "4" + doc.toJson(QJsonDocument::Compact); // "4" = Engine.IO MESSAGE frame
    
    if (webSocket && webSocket->isValid()) {
        webSocket->sendTextMessage(message);
        logToFile("Message sent successfully");
    } else {
        logToFile("ERROR: Socket not connected, cannot send message");
        emit error("Socket không kết nối");
    }
}

void SocketClient::onConnected()
{
    logToFile("[TCP] Connected, waiting for Engine.IO OPEN...");
    engineioReady = false;
    authenticated = false;
}

void SocketClient::sendAuthMessage()
{
    if (!engineioReady) {
        return;
    }
    
    QJsonObject authObj;
    authObj["token"] = token;
    
    QJsonDocument doc(authObj);
    QString authPacket = "0" + doc.toJson(QJsonDocument::Compact);
    QString engineioFrame = "4" + authPacket;
    
    if (webSocket && webSocket->isValid()) {
        webSocket->sendTextMessage(engineioFrame);
        logToFile("[AUTH] Sending auth...");
        
        if (authTimeoutTimer) {
            authTimeoutTimer->start(5000);
        }
    }
}

void SocketClient::onAuthTimeoutTimerTimeout()
{
    logToFile("[AUTH-TIMEOUT] No response, reconnecting...");
    authenticated = false;
    engineioReady = false;
    if (webSocket && webSocket->isValid()) {
        webSocket->close();
    }
}

void SocketClient::onDisconnected()
{
    logToFile("[DISCONNECT] Closed code: " + QString::number(webSocket->closeCode()));
    
    engineioReady = false;
    authenticated = false;
    
    if (authTimeoutTimer) {
        authTimeoutTimer->stop();
    }
    
    emit disconnected();
    
    if (shouldReconnect && reconnectTimer) {
        logToFile("[RECONNECT] Reconnecting in 5 seconds...");
        reconnectTimer->start(5000);
    }
}

void SocketClient::onTextMessageReceived(const QString &message)
{
    if (message.isEmpty()) {
        return;
    }
    logToFile("[TEXT-MESSAGE] Received " + QString::number(message.length()) + " bytes");
    parseSocketMessage(message);
}

void SocketClient::onError(QAbstractSocket::SocketError error)
{
    QString errorMsg = webSocket->errorString();
    logToFile("[SOCKET-ERROR] Error: " + errorMsg);
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
        return;
    }
    
    char frameType = message[0].toLatin1();
    
    switch (frameType) {
        case '0': {
            // Engine.IO OPEN
            if (message.length() > 1) {
                QString jsonStr = message.mid(1);
                QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
                
                if (doc.isObject()) {
                    engineioReady = true;
                    logToFile("[OPEN] Engine.IO ready, sending auth...");
                    sendAuthMessage();
                }
            }
            break;
        }
        case '2': {
            // Engine.IO PING
            if (webSocket && webSocket->isValid()) {
                webSocket->sendTextMessage("3");
                logToFile("[PING] Pong sent");
            }
            break;
        }
        case '4': {
            // Engine.IO MESSAGE (contains Socket.IO packet)
            if (message.length() < 2) break;
            
            QString socketioPacket = message.mid(1);
            char socketioType = socketioPacket[0].toLatin1();
            
            switch (socketioType) {
                case '0': {
                    // Socket.IO CONNECT response
                    logToFile("[CONNECT] Auth successful");
                    if (authTimeoutTimer) {
                        authTimeoutTimer->stop();
                    }
                    authenticated = true;
                    if (reconnectTimer) {
                        reconnectTimer->stop();
                    }
                    emit connected();
                    break;
                }
                case '2': {
                    // Socket.IO EVENT
                    if (socketioPacket.length() > 1) {
                        QString jsonStr = socketioPacket.mid(1);
                        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
                        
                        if (doc.isArray()) {
                            QJsonArray arr = doc.array();
                            if (arr.size() >= 2) {
                                QString eventName = arr[0].toString();
                                
                                if (eventName == "receive-message") {
                                    QJsonObject data = arr[1].toObject();
                                    logToFile("[EVENT] Message received");
                                    emit messageReceived(data);
                                }
                            }
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
}
