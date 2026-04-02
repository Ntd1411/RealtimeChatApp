#include "chatwindow.h"
#include "../config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSplitter>
#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>

ChatWindow::ChatWindow(const QString &t, const QString &uid, const QString &u)
    : token(t), user_id(uid), username(u), current_chat_user_id("")
{
    setWindowTitle("Chat Application - " + username);
    setGeometry(100, 100, 1000, 600);
    
    // Initialize API and Socket clients
    apiClient = new ApiClient(Config::getBackendUrl(), this);
    apiClient->setToken(token);
    
    socketClient = new SocketClient(Config::getBackendUrl(), token, this);
    
    // Initialize crypto driver
    cryptoDriver = new KernelCryptoClient();
    if (!cryptoDriver->isOpen()) {
        QMessageBox::warning(this, "Warning", 
            "Crypto device not available. Messages won't be encrypted.");
    }
    
    setupUI();
    setupConnections();
    
    // Load existing conversations
    apiClient->getMessageUsers();
    
    // Connect to server
    socketClient->connect();
}

ChatWindow::~ChatWindow()
{
    socketClient->disconnect();
    delete cryptoDriver;
}

void ChatWindow::setupUI()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // ========== LEFT PANEL ==========
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(10, 10, 10, 10);
    leftLayout->setSpacing(5);
    
    // Search section
    QLabel *searchLabel = new QLabel("<b>Find Users</b>");
    leftLayout->addWidget(searchLabel);
    
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search users...");
    searchEdit->setMinimumHeight(30);
    searchEdit->setStyleSheet("border: 1px solid #ccc; border-radius: 3px; padding: 5px;");
    leftLayout->addWidget(searchEdit);
    
    searchButton = new QPushButton("Search");
    searchButton->setMinimumHeight(30);
    searchButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 3px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #1976D2; }"
    );
    leftLayout->addWidget(searchButton);
    
    // Chat list
    QLabel *chatsLabel = new QLabel("<b>Conversations</b>");
    leftLayout->addWidget(chatsLabel);
    
    chatListWidget = new QListWidget();
    chatListWidget->setStyleSheet(
        "QListWidget { border: 1px solid #ccc; border-radius: 3px; }"
        "QListWidget::item:selected { background-color: #2196F3; color: white; }"
    );
    chatListWidget->setMinimumWidth(250);
    leftLayout->addWidget(chatListWidget);
    
    leftLayout->addStretch();
    leftPanel->setStyleSheet("background-color: #f5f5f5; border-right: 1px solid #ddd;");
    
    // ========== RIGHT PANEL ==========
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(10, 10, 10, 10);
    rightLayout->setSpacing(5);
    
    // Header
    currentChatLabel = new QLabel("<b>Select a conversation</b>");
    currentChatLabel->setStyleSheet("font-size: 14px; color: #333;");
    rightLayout->addWidget(currentChatLabel);
    
    typingLabel = new QLabel("");
    typingLabel->setStyleSheet("font-size: 10px; color: #999; font-style: italic;");
    rightLayout->addWidget(typingLabel);
    
    QFrame *headerLine = new QFrame();
    headerLine->setFrameShape(QFrame::HLine);
    headerLine->setFrameShadow(QFrame::Sunken);
    rightLayout->addWidget(headerLine);
    
    // Message view
    messageView = new QTextEdit();
    messageView->setReadOnly(true);
    messageView->setStyleSheet(
        "QTextEdit { border: 1px solid #ccc; border-radius: 3px; background-color: #fafafa; }"
    );
    rightLayout->addWidget(messageView, 1);
    
    // Message input area
    QHBoxLayout *inputLayout = new QHBoxLayout();
    
    messageInput = new QLineEdit();
    messageInput->setPlaceholderText("Type a message...");
    messageInput->setMinimumHeight(35);
    messageInput->setStyleSheet("border: 1px solid #ccc; border-radius: 3px; padding: 5px;");
    messageInput->setEnabled(false);
    inputLayout->addWidget(messageInput);
    
    sendButton = new QPushButton("Send");
    sendButton->setMinimumHeight(35);
    sendButton->setMinimumWidth(80);
    sendButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 3px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:disabled { background-color: #cccccc; }"
    );
    sendButton->setEnabled(false);
    inputLayout->addWidget(sendButton);
    
    rightLayout->addLayout(inputLayout);
    
    // Status bar
    statusLabel = new QLabel("Connecting...");
    statusLabel->setStyleSheet("font-size: 10px; color: #666;");
    rightLayout->addWidget(statusLabel);
    
    rightPanel->setStyleSheet("background-color: white;");
    
    // ========== MAIN SPLITTER ==========
    mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 2);
    QList<int> sizes;
    sizes << 250 << 500;
    mainSplitter->setSizes(sizes);
    
    mainLayout->addWidget(mainSplitter);
}

void ChatWindow::setupConnections()
{
    // UI Signals
    connect(searchButton, &QPushButton::clicked, this, &ChatWindow::onSearchUserClicked);
    connect(searchEdit, &QLineEdit::returnPressed, this, &ChatWindow::onSearchUserClicked);
    connect(searchEdit, &QLineEdit::textChanged, this, &ChatWindow::onSearchTextChanged);
    connect(chatListWidget, &QListWidget::itemClicked, this, &ChatWindow::onUserSelected);
    connect(sendButton, &QPushButton::clicked, this, &ChatWindow::onSendMessageClicked);
    connect(messageInput, &QLineEdit::returnPressed, this, &ChatWindow::onSendMessageClicked);
    
    // API Signals
    connect(apiClient, &ApiClient::searchResults, this, &ChatWindow::onSearchResults);
    connect(apiClient, &ApiClient::searchFailed, this, &ChatWindow::onSearchFailed);
    connect(apiClient, &ApiClient::messageUsersReceived, this, &ChatWindow::onMessageUsersReceived);
    connect(apiClient, &ApiClient::messagesReceived, this, &ChatWindow::onMessagesReceived);
    
    // Socket Signals
    connect(socketClient, &SocketClient::connected, this, &ChatWindow::onSocketConnected);
    connect(socketClient, &SocketClient::disconnected, this, &ChatWindow::onSocketDisconnected);
    connect(socketClient, &SocketClient::messageReceived, this, &ChatWindow::onSocketMessageReceived);
}

void ChatWindow::onSendMessageClicked()
{
    QString content = messageInput->text().trimmed();
    
    if (content.isEmpty() || current_chat_user_id.isEmpty()) {
        return;
    }
    
    qDebug() << "[SEND-MESSAGE] Sending message to:" << current_chat_user_id;
    
    // Save message via API (for persistence)
    apiClient->saveMessage(current_chat_user_id, content);
    
    // Send real-time via socket if connected
    if (socketClient->isConnected()) {
        socketClient->sendMessage(current_chat_user_id, content);
        qDebug() << "[SEND-MESSAGE] Sent via socket";
    } else {
        qDebug() << "[SEND-MESSAGE] Socket not connected, message will be sent via API only";
    }
    
    // Display own message immediately
    displayMessage(username, content, true);
    messageInput->clear();
}

void ChatWindow::onSearchUserClicked()
{
    QString query = searchEdit->text().trimmed();
    if (!query.isEmpty()) {
        apiClient->searchUsers(query);
        statusLabel->setText("Searching...");
    }
}

void ChatWindow::onSearchTextChanged(const QString &text)
{
    if (text.isEmpty()) {
        // Load existing conversations when search is cleared
        apiClient->getMessageUsers();
    }
}

void ChatWindow::onUserSelected(QListWidgetItem *item)
{
    // Get user ID from item data
    QString selectedUserId = item->data(Qt::UserRole).toString();
    QString selectedUsername = item->text();
    
    current_chat_user_id = selectedUserId;
    current_chat_username = selectedUsername;
    
    currentChatLabel->setText("<b>Chat with " + selectedUsername + "</b>");
    messageInput->setEnabled(true);
    sendButton->setEnabled(true);
    messageView->clear();
    typingLabel->clear();
    
    // Load conversation history
    apiClient->getMessages(selectedUserId);
    
    statusLabel->setText("Loading conversation...");
}

void ChatWindow::onSearchResults(const QJsonArray &users)
{
    chatListWidget->clear();
    
    for (int i = 0; i < users.size(); i++) {
        QJsonObject user = users[i].toObject();
        QString userId = user["_id"].toString();
        QString userName = user["username"].toString();
        
        QListWidgetItem *item = new QListWidgetItem(userName);
        item->setData(Qt::UserRole, userId);
        chatListWidget->addItem(item);
    }
    
    statusLabel->setText("Found " + QString::number(users.size()) + " users");
}

void ChatWindow::onSearchFailed(const QString &error)
{
    statusLabel->setText("Search failed: " + error);
    QMessageBox::warning(this, "Search Error", error);
}

void ChatWindow::onMessageUsersReceived(const QJsonArray &users)
{
    chatListWidget->clear();
    
    for (int i = 0; i < users.size(); i++) {
        QJsonObject user = users[i].toObject();
        QString userId = user["_id"].toString();
        QString userName = user["username"].toString();
        
        QListWidgetItem *item = new QListWidgetItem(userName);
        item->setData(Qt::UserRole, userId);
        chatListWidget->addItem(item);
    }
    
    statusLabel->setText("Loaded " + QString::number(users.size()) + " conversations");
}

void ChatWindow::onMessagesReceived(const QJsonArray &messages)
{
    messageView->clear();
    
    for (int i = 0; i < messages.size(); i++) {
        QJsonObject msg = messages[i].toObject();
        QString senderId = msg["senderId"].toString();
        QString content = msg["content"].toString();
        QString createdAt = msg["createdAt"].toString();
        
        bool isOwn = (senderId == user_id);
        QString sender = isOwn ? "You" : current_chat_username;
        
        displayMessage(sender, content, isOwn, createdAt);
    }
    
    statusLabel->setText("Ready");
}

void ChatWindow::onSocketConnected()
{
    qDebug() << "[SOCKET] Connected successfully";
    statusLabel->setText("Connected");
    statusLabel->setStyleSheet("font-size: 10px; color: green; font-weight: bold;");
}

void ChatWindow::onSocketDisconnected()
{
    qDebug() << "[SOCKET] Disconnected from server";
    statusLabel->setText("Disconnected - Reconnecting...");
    statusLabel->setStyleSheet("font-size: 10px; color: orange; font-weight: bold;");
}

void ChatWindow::onSocketMessageReceived(const QJsonObject &message)
{
    QString senderId = message["senderId"].toString();
    QString content = message["content"].toString();
    
    qDebug() << "[SOCKET-MESSAGE] From:" << senderId << "Content:" << content;
    
    // Only display if it's from current chat
    if (senderId == current_chat_user_id) {
        displayMessage(current_chat_username, content, false);
        statusLabel->setText("New message received");
    }
}



void ChatWindow::displayMessage(const QString &sender, const QString &content, bool isOwn, const QString &timestamp)
{
    QString time = timestamp;
    if (time.isEmpty()) {
        time = QDateTime::currentDateTime().toString("hh:mm:ss");
    }
    
    QString formatted = QString("[%1] <b>%2:</b> %3\n").arg(time, sender, content);
    
    // Apply styling
    messageView->setTextColor(isOwn ? Qt::blue : Qt::darkGreen);
    if (isOwn) {
        messageView->setTextBackgroundColor(QColor(200, 220, 255));
    }
    messageView->append(formatted);
    messageView->setTextBackgroundColor(Qt::white);
}



void ChatWindow::closeEvent(QCloseEvent *event)
{
    socketClient->disconnect();
    apiClient->logout();
    QMainWindow::closeEvent(event);
}
