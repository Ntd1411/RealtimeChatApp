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
    : QObject(parent), server_url(serverUrl), token(t), message_counter(0), shouldReconnect(true), authenticated(false)
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
    authenticated = false;
    
    // Convert http:// to ws:// and https:// to wss://
    QString wsUrl = server_url;
    if (wsUrl.startsWith("https://")) {
        wsUrl.replace(0, 8, "wss://");
    } else if (wsUrl.startsWith("http://")) {
        wsUrl.replace(0, 7, "ws://");
    }
    
    // Connect without auth token in query - will send auth via Socket.IO protocol
    if (!wsUrl.endsWith("/")) wsUrl += "/";
    wsUrl += "socket.io/?transport=websocket";
    
    logToFile("Socket URL: " + wsUrl);
    logToFile("Will send auth token after connection established");
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
    logToFile("========================================");
    logToFile("[CONNECTED] TCP socket connected successfully");
    logToFile("WebSocket state: " + QString::number(webSocket->isValid()));
    logToFile("[CONNECTED] Sending Socket.IO auth frame...");
    logToFile("========================================");
    
    // Send auth frame to server
    sendAuthMessage();
}

void SocketClient::sendAuthMessage()
{
    // Build Socket.IO CONNECT packet with auth
    // Socket.IO packet format: 0 (CONNECT) + namespace + JSON auth
    QJsonObject authObj;
    authObj["token"] = token;
    
    QJsonDocument doc(authObj);
    QString authPacket = "0" + doc.toJson(QJsonDocument::Compact);
    
    // Wrap in Engine.IO message frame (type 4)
    // Engine.IO protocol: 4 = message frame
    QString engineioFrame = "4" + authPacket;
    
    logToFile("[AUTH-FRAME] Socket.IO packet: " + authPacket.left(100));
    logToFile("[AUTH-FRAME] Engine.IO frame: " + engineioFrame.left(100));
    logToFile("[AUTH-FRAME] Sending auth message...");
    
    if (webSocket && webSocket->isValid()) {
        webSocket->sendTextMessage(engineioFrame);
        logToFile("[AUTH-FRAME] Auth frame sent successfully");
    } else {
        logToFile("[AUTH-FRAME] ERROR: WebSocket not valid");
        emit error("Socket không sẵn sàng");
    }
}

void SocketClient::onDisconnected()
{
    logToFile("========================================");
    logToFile("[DISCONNECTED] Socket disconnected from server");
    logToFile("WebSocket state: " + QString::number(webSocket->isValid()));
    logToFile("========================================");
    emit disconnected();
    
    // Auto-reconnect after 5 seconds
    if (shouldReconnect && reconnectTimer) {
        logToFile("[RECONNECT] Scheduling reconnect in 5 seconds...");
        reconnectTimer->start(5000);
    }
}

void SocketClient::onTextMessageReceived(const QString &message)
{
    logToFile("========================================");
    logToFile("[TEXT-MESSAGE-RECEIVED] Message arrived");
    logToFile("[TEXT-MESSAGE-RECEIVED] Length: " + QString::number(message.length()));
    logToFile("[TEXT-MESSAGE-RECEIVED] Full content: " + message);
    logToFile("[TEXT-MESSAGE-RECEIVED] First 10 chars hex:");
    
    // Log hex values of first 10 chars
    for (int i = 0; i < qMin(10, message.length()); i++) {
        logToFile("  [" + QString::number(i) + "] = " + 
                 QString::number((int)message[i].toLatin1()) + 
                 " (" + message[i] + ")");
    }
    logToFile("========================================");
    parseSocketMessage(message);
}

void SocketClient::onError(QAbstractSocket::SocketError error)
{
    QString errorMsg = webSocket->errorString();
    logToFile("========================================");
    logToFile("[SOCKET-ERROR] Error code: " + QString::number(error));
    logToFile("[SOCKET-ERROR] Error message: " + errorMsg);
    logToFile("[SOCKET-ERROR] WebSocket state: " + QString::number(webSocket->state()));
    logToFile("[SOCKET-ERROR] Authenticated: " + QString(authenticated ? "YES" : "NO"));
    logToFile("========================================");
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
            logToFile("[ENGINE-IO-OPEN] Server sent OPEN frame: " + message.left(100));
            // This is the Engine.IO OPEN packet, just log it and continue
            break;
        }
        case '1': {
            logToFile("[ENGINE-IO-CLOSE] Server sent CLOSE frame");
            break;
        }
        case '2': {
            logToFile("[ENGINE-IO-PING] Received ping, sending pong");
            // Respond with pong (3)
            if (webSocket && webSocket->isValid()) {
                webSocket->sendTextMessage("3");
            }
            break;
        }
        case '3': {
            logToFile("[ENGINE-IO-PONG] Received pong");
            break;
        }
        case '4': {
            logToFile("[ENGINE-IO-MESSAGE] Received Engine.IO message frame");
            
            // Engine.IO frame 4 contains Socket.IO packet
            // Extract Socket.IO packet (everything after the '4')
            if (message.length() < 2) {
                logToFile("[ENGINE-IO-MESSAGE] Message too short");
                break;
            }
            
            QString socketioPacket = message.mid(1);
            char socketioType = socketioPacket[0].toLatin1();
            
            logToFile("[SOCKETIO-PACKET] Type: " + QString::number(socketioType) + 
                     " Content: " + socketioPacket.left(200));
            
            switch (socketioType) {
                case '0': {
                    // Socket.IO CONNECT response
                    logToFile("[SOCKETIO-CONNECT] Server connect response received");
                    
                    // Check if there's JSON data (error response)
                    if (socketioPacket.length() > 1 && socketioPacket[1] == '{') {
                        QString jsonStr = socketioPacket.mid(1);
                        QJsonParseError jsonError;
                        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &jsonError);
                        
                        if (doc.isObject()) {
                            QJsonObject obj = doc.object();
                            if (obj.contains("message")) {
                                // Auth error
                                QString errorMsg = obj["message"].toString();
                                logToFile("[SOCKETIO-CONNECT] AUTH FAILED: " + errorMsg);
                                emit error("Lỗi xác thực: " + errorMsg);
                                if (webSocket && webSocket->isValid()) {
                                    webSocket->close();
                                }
                            } else {
                                logToFile("[SOCKETIO-CONNECT] AUTH SUCCESSFUL");
                                authenticated = true;
                                if (reconnectTimer) {
                                    reconnectTimer->stop();
                                }
                                emit connected();
                            }
                        } else {
                            logToFile("[SOCKETIO-CONNECT] JSON parse error: " + jsonError.errorString());
                        }
                    } else {
                        // No JSON data = successful connect
                        logToFile("[SOCKETIO-CONNECT] AUTH SUCCESSFUL (no data)");
                        authenticated = true;
                        if (reconnectTimer) {
                            reconnectTimer->stop();
                        }
                        emit connected();
                    }
                    break;
                }
                case '2': {
                    // Socket.IO EVENT - app message
                    logToFile("[SOCKETIO-EVENT] Received socket event");
                    
                    if (socketioPacket.length() > 1) {
                        QString jsonStr = socketioPacket.mid(1);
                        QJsonParseError jsonError;
                        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &jsonError);
                        
                        if (doc.isArray()) {
                            QJsonArray arr = doc.array();
                            if (arr.size() >= 2) {
                                QString eventName = arr[0].toString();
                                QJsonObject data = arr[1].toObject();
                                
                                logToFile("[SOCKETIO-EVENT] Event: " + eventName);
                                
                                if (eventName == "receive-message") {
                                    logToFile("[SOCKETIO-EVENT] Received message event");
                                    emit messageReceived(data);
                                } else if (eventName == "seen-message") {
                                    emit messagesSeen(data["viewerId"].toString());
                                } else if (eventName == "typing-start") {
                                    emit typingStarted(data["senderId"].toString(), data["senderName"].toString());
                                } else if (eventName == "typing-stop") {
                                    emit typingStopped(data["senderId"].toString());
                                } else if (eventName == "noti-online") {
                                    emit onlineStatusChanged(data["id"].toString(), true);
                                } else if (eventName == "noti-offline") {
                                    emit onlineStatusChanged(data["id"].toString(), false);
                                } else {
                                    logToFile("[SOCKETIO-EVENT] Unknown event: " + eventName);
                                }
                            }
                        } else {
                            logToFile("[SOCKETIO-EVENT] JSON not array");
                        }
                    }
                    break;
                }
                case '4': {
                    // Socket.IO ERROR
                    logToFile("[SOCKETIO-ERROR] Error packet received: " + socketioPacket);
                    if (socketioPacket.length() > 1) {
                        QString errorData = socketioPacket.mid(1);
                        logToFile("[SOCKETIO-ERROR] Error data: " + errorData);
                        emit error("Socket error: " + errorData);
                    }
                    break;
                }
                default: {
                    logToFile("[SOCKETIO-UNKNOWN] Type: " + QString::number(socketioType) + 
                             " Content: " + socketioPacket.left(100));
                }
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
