#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSplitter>
#include <QTimer>
#include "../api/apiclient.h"
#include "../network/socketclient.h"
#include "../crypto/kernel_crypto_client.h"

class ChatWindow : public QMainWindow {
    Q_OBJECT

public:
    ChatWindow(const QString &token, const QString &userId, const QString &username);
    ~ChatWindow();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    // UI Events
    void onSendMessageClicked();
    void onSearchUserClicked();
    void onUserSelected(QListWidgetItem *item);
    void onSearchTextChanged(const QString &text);
    
    // API Events
    void onSearchResults(const QJsonArray &users);
    void onSearchFailed(const QString &error);
    void onMessageUsersReceived(const QJsonArray &users);
    void onMessagesReceived(const QJsonArray &messages);
    
    // Socket Events
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketMessageReceived(const QJsonObject &message);
    void onSocketTypingStart(const QString &senderId, const QString &senderName);
    void onSocketTypingStopped(const QString &senderId);
    void onSocketOnlineStatusChanged(const QString &userId, bool isOnline);

private:
    void setupUI();
    void setupConnections();
    void loadConversationHistory();
    void displayMessage(const QString &sender, const QString &content, bool isOwn, const QString &timestamp = "");
    void enableCrypto();
    
    // UI Components
    QWidget *centralWidget;
    
    // Left panel - Chat list and search
    QLineEdit *searchEdit;
    QPushButton *searchButton;
    QListWidget *chatListWidget;
    
    // Right panel - Chat window
    QTextEdit *messageView;
    QLineEdit *messageInput;
    QPushButton *sendButton;
    QLabel *currentChatLabel;
    QLabel *statusLabel;
    QLabel *typingLabel;
    
    // Splitter
    QSplitter *mainSplitter;
    
    // Data
    ApiClient *apiClient;
    SocketClient *socketClient;
    KernelCryptoClient *cryptoDriver;
    
    QString token;
    QString user_id;
    QString username;
    QString current_chat_user_id;
    QString current_chat_username;
    
    QTimer *typingTimer;
    bool isTyping;
};

#endif // CHATWINDOW_H
