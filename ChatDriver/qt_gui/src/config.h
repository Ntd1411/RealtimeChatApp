#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

class Config {
public:
    // Backend server configuration
    // Change this single value to update server for entire app
    static constexpr const char* BACKEND_URL = "http://localhost:3000";
    
    // Derived URLs (auto-generated from BACKEND_URL)
    static QString getBackendUrl() {
        return QString(BACKEND_URL);
    }
    
    // WebSocket URL (auto-converts http to ws)
    static QString getWebSocketUrl() {
        QString url = QString(BACKEND_URL);
        if (url.startsWith("https://")) {
            url.replace(0, 8, "wss://");
        } else if (url.startsWith("http://")) {
            url.replace(0, 7, "ws://");
        }
        return url;
    }
};

#endif // CONFIG_H
