#include "logindialog.h"
#include "../config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent), logged_in(false), token(""), user_id(""), username("")
{
    setWindowTitle("Chat Application - Login");
    setGeometry(300, 300, 400, 300);
    setModal(true);
    setStyleSheet(
        "QDialog { background-color: #f5f5f5; }"
        "QLineEdit { border: 1px solid #ccc; border-radius: 3px; padding: 5px; }"
        "QPushButton { border: none; border-radius: 3px; padding: 8px; font-weight: bold; }"
    );
    
    apiClient = new ApiClient(Config::getBackendUrl(), this);
    
    setupUI();
    setupConnections();
}

void LoginDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Title
    QLabel *titleLabel = new QLabel("<h2>Chat Application</h2>");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // Subtitle
    QLabel *subtitleLabel = new QLabel("Real-time Messaging with Encryption");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet("color: gray; font-size: 11px;");
    mainLayout->addWidget(subtitleLabel);
    
    // Username
    QLabel *userLabel = new QLabel("Username:");
    userLabel->setStyleSheet("font-weight: bold;");
    mainLayout->addWidget(userLabel);
    usernameEdit = new QLineEdit();
    usernameEdit->setPlaceholderText("Enter your username");
    usernameEdit->setMinimumHeight(35);
    mainLayout->addWidget(usernameEdit);
    
    // Password
    QLabel *passLabel = new QLabel("Password:");
    passLabel->setStyleSheet("font-weight: bold;");
    mainLayout->addWidget(passLabel);
    passwordEdit = new QLineEdit();
    passwordEdit->setPlaceholderText("Enter your password");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setMinimumHeight(35);
    mainLayout->addWidget(passwordEdit);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    loginButton = new QPushButton("Login");
    loginButton->setMinimumHeight(35);
    loginButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "}"
        "QPushButton:hover { background-color: #1976D2; }"
        "QPushButton:pressed { background-color: #1565C0; }"
    );
    buttonLayout->addWidget(loginButton);
    
    registerButton = new QPushButton("Register");
    registerButton->setMinimumHeight(35);
    registerButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "}"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #3d8b40; }"
    );
    buttonLayout->addWidget(registerButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Status
    statusLabel = new QLabel("Ready");
    statusLabel->setStyleSheet("color: gray; font-size: 10px;");
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);
    
    mainLayout->addStretch();
}

void LoginDialog::setupConnections()
{
    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(registerButton, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    
    connect(apiClient, &ApiClient::loginSuccess, this, &LoginDialog::onLoginSuccess);
    connect(apiClient, &ApiClient::loginFailed, this, &LoginDialog::onLoginFailed);
    connect(apiClient, &ApiClient::signupSuccess, this, &LoginDialog::onSignupSuccess);
    connect(apiClient, &ApiClient::signupFailed, this, &LoginDialog::onSignupFailed);
}

void LoginDialog::onLoginClicked()
{
    QString u = usernameEdit->text().trimmed();
    QString p = passwordEdit->text();
    
    if (u.isEmpty() || p.isEmpty()) {
        statusLabel->setText("Vui lòng nhập tên đăng nhập và mật khẩu");
        statusLabel->setStyleSheet("color: red; font-size: 10px;");
        return;
    }
    
    statusLabel->setText("Đang đăng nhập...");
    statusLabel->setStyleSheet("color: blue; font-size: 10px;");
    loginButton->setEnabled(false);
    registerButton->setEnabled(false);
    
    apiClient->login(u, p);
}

void LoginDialog::onRegisterClicked()
{
    SignUpDialog *signupDialog = new SignUpDialog(apiClient, this);
    signupDialog->exec();
    signupDialog->deleteLater();
}

void LoginDialog::onLoginSuccess(const QJsonObject &user)
{
    token = apiClient->getToken();
    user_id = apiClient->getUserId();
    username = apiClient->getUsername();
    logged_in = true;
    
    statusLabel->setText("Đăng nhập thành công!");
    statusLabel->setStyleSheet("color: green; font-size: 10px;");
    
    // Close dialog after a short delay
    QTimer::singleShot(500, this, &LoginDialog::accept);
}

void LoginDialog::onLoginFailed(const QString &error)
{
    statusLabel->setText("Đăng nhập thất bại: " + error);
    statusLabel->setStyleSheet("color: red; font-size: 10px;");
    loginButton->setEnabled(true);
    registerButton->setEnabled(true);
}

void LoginDialog::onSignupSuccess()
{
    QMessageBox::information(this, "Thành công", "Tài khoản đã được tạo! Bạn có thể đăng nhập ngay bây giờ.");
    usernameEdit->clear();
    passwordEdit->clear();
    statusLabel->setText("Sẵn sàng");
    statusLabel->setStyleSheet("color: gray; font-size: 10px;");
    loginButton->setEnabled(true);
    registerButton->setEnabled(true);
}

void LoginDialog::onSignupFailed(const QString &error)
{
    statusLabel->setText("Đăng ký thất bại: " + error);
    statusLabel->setStyleSheet("color: red; font-size: 10px;");
    loginButton->setEnabled(true);
    registerButton->setEnabled(true);
}
