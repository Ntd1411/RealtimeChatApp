#ifndef SIGNUPDIALOG_H
#define SIGNUPDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../api/apiclient.h"

class SignUpDialog : public QDialog {
    Q_OBJECT

public:
    SignUpDialog(ApiClient *apiClient, QWidget *parent = 0);
    
    bool isSignupSuccessful() const { return signup_successful; }

private slots:
    void onSignupClicked();
    void onSignupSuccess();
    void onSignupFailed(const QString &error);

private:
    void setupUI();
    void setupConnections();
    
    // UI Elements
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QLineEdit *fullNameEdit;
    QLineEdit *emailEdit;
    QPushButton *signupButton;
    QPushButton *cancelButton;
    QLabel *statusLabel;
    
    // API Client
    ApiClient *apiClient;
    
    // State
    bool signup_successful;
};

#endif // SIGNUPDIALOG_H
