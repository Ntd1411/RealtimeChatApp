#include "signupdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>

SignUpDialog::SignUpDialog(ApiClient *apiClient, QWidget *parent)
    : QDialog(parent), apiClient(apiClient), signup_successful(false)
{
    setWindowTitle("Sign Up");
    setGeometry(300, 300, 450, 450);
    setModal(true);
    setStyleSheet(
        "QDialog { background-color: #f5f5f5; }"
        "QLineEdit { border: 1px solid #ccc; border-radius: 3px; padding: 8px; }"
        "QPushButton { border: none; border-radius: 3px; padding: 8px; font-weight: bold; }"
    );
    
    setupUI();
    setupConnections();
}

void SignUpDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Title
    QLabel *titleLabel = new QLabel("<h2>Create New Account</h2>");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // Username
    QLabel *userLabel = new QLabel("Username:");
    userLabel->setStyleSheet("font-weight: bold;");
    mainLayout->addWidget(userLabel);
    usernameEdit = new QLineEdit();
    usernameEdit->setPlaceholderText("Choose a unique username");
    usernameEdit->setMinimumHeight(35);
    mainLayout->addWidget(usernameEdit);
    
    // Full Name
    QLabel *nameLabel = new QLabel("Full Name:");
    nameLabel->setStyleSheet("font-weight: bold;");
    mainLayout->addWidget(nameLabel);
    fullNameEdit = new QLineEdit();
    fullNameEdit->setPlaceholderText("Enter your full name");
    fullNameEdit->setMinimumHeight(35);
    mainLayout->addWidget(fullNameEdit);
    
    // Email
    QLabel *emailLabel = new QLabel("Email:");
    emailLabel->setStyleSheet("font-weight: bold;");
    mainLayout->addWidget(emailLabel);
    emailEdit = new QLineEdit();
    emailEdit->setPlaceholderText("Enter your email address");
    emailEdit->setMinimumHeight(35);
    mainLayout->addWidget(emailEdit);
    
    // Password
    QLabel *passLabel = new QLabel("Password:");
    passLabel->setStyleSheet("font-weight: bold;");
    mainLayout->addWidget(passLabel);
    passwordEdit = new QLineEdit();
    passwordEdit->setPlaceholderText("Choose a strong password");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setMinimumHeight(35);
    mainLayout->addWidget(passwordEdit);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    signupButton = new QPushButton("Sign Up");
    signupButton->setMinimumHeight(35);
    signupButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "}"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #3d8b40; }"
    );
    buttonLayout->addWidget(signupButton);
    
    cancelButton = new QPushButton("Cancel");
    cancelButton->setMinimumHeight(35);
    cancelButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #f44336;"
        "  color: white;"
        "}"
        "QPushButton:hover { background-color: #da190b; }"
        "QPushButton:pressed { background-color: #ba0a0a; }"
    );
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Status
    statusLabel = new QLabel("Ready");
    statusLabel->setStyleSheet("color: gray; font-size: 10px;");
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);
    
    mainLayout->addStretch();
}

void SignUpDialog::setupConnections()
{
    connect(signupButton, &QPushButton::clicked, this, &SignUpDialog::onSignupClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    connect(apiClient, &ApiClient::signupSuccess, this, &SignUpDialog::onSignupSuccess);
    connect(apiClient, &ApiClient::signupFailed, this, &SignUpDialog::onSignupFailed);
}

void SignUpDialog::onSignupClicked()
{
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();
    QString fullName = fullNameEdit->text().trimmed();
    QString email = emailEdit->text().trimmed();
    
    // Validate input
    if (username.isEmpty()) {
        statusLabel->setText("Username is required");
        statusLabel->setStyleSheet("color: red; font-size: 10px;");
        return;
    }
    
    if (password.isEmpty()) {
        statusLabel->setText("Password is required");
        statusLabel->setStyleSheet("color: red; font-size: 10px;");
        return;
    }
    
    if (fullName.isEmpty()) {
        statusLabel->setText("Full name is required");
        statusLabel->setStyleSheet("color: red; font-size: 10px;");
        return;
    }
    
    if (email.isEmpty()) {
        statusLabel->setText("Email is required");
        statusLabel->setStyleSheet("color: red; font-size: 10px;");
        return;
    }
    
    // Simple email validation
    if (!email.contains("@") || !email.contains(".")) {
        statusLabel->setText("Invalid email format");
        statusLabel->setStyleSheet("color: red; font-size: 10px;");
        return;
    }
    
    statusLabel->setText("Creating account...");
    statusLabel->setStyleSheet("color: blue; font-size: 10px;");
    signupButton->setEnabled(false);
    cancelButton->setEnabled(false);
    
    // Call signup API
    apiClient->signup(username, password, fullName, email);
}

void SignUpDialog::onSignupSuccess()
{
    statusLabel->setText("Sign up successful!");
    statusLabel->setStyleSheet("color: green; font-size: 10px;");
    
    signup_successful = true;
    signupButton->setEnabled(true);
    cancelButton->setEnabled(true);
    
    QMessageBox::information(this, "Success", 
        "Account created successfully!\nYou can now log in with your credentials.");
    
    accept();
}

void SignUpDialog::onSignupFailed(const QString &error)
{
    statusLabel->setText("Sign up failed: " + error);
    statusLabel->setStyleSheet("color: red; font-size: 10px;");
    
    signupButton->setEnabled(true);
    cancelButton->setEnabled(true);
    
    QMessageBox::warning(this, "Sign Up Failed", 
        "Failed to create account:\n" + error);
}
