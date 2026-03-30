#include <QApplication>
#include <QDesktopWidget>
#include "ui/logindialog.h"
#include "ui/chatwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application style and palette
    app.setStyle("Fusion");
    
    // Show login dialog
    LoginDialog loginWindow;
    
    if (loginWindow.exec() == QDialog::Accepted && loginWindow.isLoggedIn()) {
        // Open chat window
        ChatWindow *chatWindow = new ChatWindow(
            loginWindow.getToken(),
            loginWindow.getUserId(),
            loginWindow.getUsername()
        );
        
        chatWindow->show();
        return app.exec();
    }
    
    return 0;
}
