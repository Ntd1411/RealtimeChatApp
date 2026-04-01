#ifndef APICLIENT_H
#define APICLIENT_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include "../crypto/kernel_crypto_client.h"

class ApiClient : public QObject {
    Q_OBJECT

public:
    ApiClient(const QString &baseUrl = "http://localhost:3000", QObject *parent = 0);
    
    // Auth endpoints
    void login(const QString &username, const QString &password);
    void signup(const QString &username, const QString &password, const QString &fullName, const QString &email);
    void logout();
    void getMe();
    
    // User endpoints
    void searchUsers(const QString &query);
    void updateProfile(const QJsonObject &data);
    
    // Message endpoints
    void getMessageUsers();
    void getMessages(const QString &userId);
    
    // Getters
    QString getToken() const { return token; }
    QString getUserId() const { return current_user_id; }
    QString getUsername() const { return current_username; }
    void setToken(const QString &t) { token = t; }
    
signals:
    void loginSuccess(const QJsonObject &user);
    void loginFailed(const QString &error);
    
    void signupSuccess();
    void signupFailed(const QString &error);
    
    void searchResults(const QJsonArray &users);
    void searchFailed(const QString &error);
    
    void messageUsersReceived(const QJsonArray &users);
    void messagesReceived(const QJsonArray &messages);
    
    void error(const QString &message);

private slots:
    void onLoginFinished();
    void onSignupFinished();
    void onLogoutFinished();
    void onSearchFinished();
    void onMessageUsersFinished();
    void onMessagesFinished();
    void onUpdateProfileFinished();
    void onGetMeFinished();

private:
    QNetworkAccessManager *manager;
    KernelCryptoClient *cryptoClient;
    QString base_url;
    QString token;
    QString current_user_id;
    QString current_username;
    
    // Reply pointers for async handling
    QNetworkReply *loginReply;
    QNetworkReply *signupReply;
    QNetworkReply *logoutReply;
    QNetworkReply *searchReply;
    QNetworkReply *messageUsersReply;
    QNetworkReply *messagesReply;
    QNetworkReply *updateProfileReply;
    QNetworkReply *getMeReply;
    
    QNetworkRequest createRequest(const QString &endpoint);
    void handleJsonResponse(QNetworkReply *reply);
    QString normalizeVietnamese(const QString &input);
    QString hashPassword(const QString &password);
};

#endif // APICLIENT_H
