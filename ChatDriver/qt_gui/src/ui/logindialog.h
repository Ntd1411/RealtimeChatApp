#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../api/apiclient.h"

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    LoginDialog(QWidget *parent = 0);
    
    QString getToken() const { return token; }
    QString getUserId() const { return user_id; }
    QString getUsername() const { return username; }
    bool isLoggedIn() const { return logged_in; }

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onLoginSuccess(const QJsonObject &user);
    void onLoginFailed(const QString &error);
    void onSignupSuccess();
    void onSignupFailed(const QString &error);

private:
    void setupUI();
    void setupConnections();
    
    // UI Elements
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    QPushButton *registerButton;
    QLabel *statusLabel;
    
    // API Client
    ApiClient *apiClient;
    
    // State
    QString token;
    QString user_id;
    QString username;
    bool logged_in;
};

#endif // LOGINDIALOG_H
