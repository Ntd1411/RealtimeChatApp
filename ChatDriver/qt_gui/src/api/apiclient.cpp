#include "apiclient.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QDebug>

ApiClient::ApiClient(const QString &baseUrl, QObject *parent)
    : QObject(parent), base_url(baseUrl), loginReply(0), signupReply(0), logoutReply(0),
      searchReply(0), messageUsersReply(0), messagesReply(0), updateProfileReply(0), getMeReply(0)
{
    manager = new QNetworkAccessManager(this);
}

QNetworkRequest ApiClient::createRequest(const QString &endpoint)
{
    QUrl url(base_url + endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "ChatApp/1.0");
    
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
        qDebug() << "[createRequest] Added auth token";
    }
    
    qDebug() << "[createRequest] URL:" << url.toString();
    return request;
}

void ApiClient::login(const QString &username, const QString &password)
{
    qDebug() << "================================";
    qDebug() << "[LOGIN] Starting login process";
    qDebug() << "[LOGIN] Username:" << username;
    qDebug() << "[LOGIN] Base URL:" << base_url;
    
    QNetworkRequest request = createRequest("/auth/login");
    
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;
    
    QJsonDocument doc(json);
    qDebug() << "[LOGIN] Request body:" << doc.toJson(QJsonDocument::Compact);
    
    loginReply = manager->post(request, doc.toJson());
    
    if (!loginReply) {
        qDebug() << "[LOGIN] ERROR: Failed to create network reply!";
        emit loginFailed("Network error");
        return;
    }
    
    qDebug() << "[LOGIN] Sending request...";
    connect(loginReply, SIGNAL(finished()), this, SLOT(onLoginFinished()));
    connect(loginReply, SIGNAL(error(QNetworkReply::NetworkError)), 
            this, [this]() {
        if (loginReply) {
            qDebug() << "[LOGIN] Network error:" << loginReply->errorString();
        }
    });
}

void ApiClient::signup(const QString &username, const QString &password)
{
    QNetworkRequest request = createRequest("/auth/signup");
    
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;
    
    QJsonDocument doc(json);
    signupReply = manager->post(request, doc.toJson());
    connect(signupReply, SIGNAL(finished()), this, SLOT(onSignupFinished()));
}

void ApiClient::logout()
{
    if (token.isEmpty()) return;
    
    QNetworkRequest request = createRequest("/auth/logout");
    logoutReply = manager->post(request, QByteArray());
    connect(logoutReply, SIGNAL(finished()), this, SLOT(onLogoutFinished()));
}

void ApiClient::searchUsers(const QString &query)
{
    QUrl url(base_url + "/api/user/search");
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", query);
    url.setQuery(urlQuery);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    }
    
    searchReply = manager->get(request);
    connect(searchReply, SIGNAL(finished()), this, SLOT(onSearchFinished()));
}

void ApiClient::getMessageUsers()
{
    QNetworkRequest request = createRequest("/api/message/users");
    messageUsersReply = manager->get(request);
    connect(messageUsersReply, SIGNAL(finished()), this, SLOT(onMessageUsersFinished()));
}

void ApiClient::getMessages(const QString &userId)
{
    QNetworkRequest request = createRequest("/api/message/" + userId);
    messagesReply = manager->get(request);
    connect(messagesReply, SIGNAL(finished()), this, SLOT(onMessagesFinished()));
}

void ApiClient::updateProfile(const QJsonObject &data)
{
    QNetworkRequest request = createRequest("/api/user/update");
    QJsonDocument doc(data);
    QByteArray jsonData = doc.toJson();
    updateProfileReply = manager->post(request, jsonData);
    connect(updateProfileReply, SIGNAL(finished()), this, SLOT(onUpdateProfileFinished()));
}

void ApiClient::getMe()
{
    qDebug() << "[GET-ME] Fetching user info from /auth/me";
    QNetworkRequest request = createRequest("/auth/me");
    getMeReply = manager->get(request);
    
    if (!getMeReply) {
        qDebug() << "[GET-ME] Failed to create request";
        return;
    }
    
    qDebug() << "[GET-ME] Sending request...";
    connect(getMeReply, SIGNAL(finished()), this, SLOT(onGetMeFinished()));
}

void ApiClient::onLoginFinished()
{
    qDebug() << "[LOGIN-RESPONSE] Received response";
    
    if (!loginReply) {
        qDebug() << "[LOGIN-RESPONSE] ERROR: loginReply is NULL!";
        emit loginFailed("No response from server");
        return;
    }
    
    int httpStatus = loginReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "[LOGIN-RESPONSE] HTTP Status:" << httpStatus;
    qDebug() << "[LOGIN-RESPONSE] Error code:" << loginReply->error();
    
    if (loginReply->error() != QNetworkReply::NoError) {
        QString errorStr = loginReply->errorString();
        qDebug() << "[LOGIN-RESPONSE] Network Error:" << errorStr;
        emit loginFailed("Network error: " + errorStr);
        loginReply->deleteLater();
        loginReply = 0;
        return;
    }
    
    QByteArray responseData = loginReply->readAll();
    qDebug() << "[LOGIN-RESPONSE] Raw response:" << responseData;
    
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    
    if (!doc.isObject()) {
        qDebug() << "[LOGIN-RESPONSE] Response is NOT valid JSON!";
        emit loginFailed("Invalid JSON response from server");
        loginReply->deleteLater();
        loginReply = 0;
        return;
    }
    
    QJsonObject obj = doc.object();
    qDebug() << "[LOGIN-RESPONSE] JSON keys:" << obj.keys();
    
    // Support new format: {"message": "...", "token": "..."}
    if (obj.contains("token")) {
        token = obj["token"].toString();
        qDebug() << "[LOGIN-RESPONSE] New format detected (token at root)";
        qDebug() << "[LOGIN-RESPONSE] Token length:" << token.length();
        qDebug() << "[LOGIN-RESPONSE] Emitting loginSuccess";
        emit loginSuccess(obj);
        
        // Fetch user info from /auth/me
        qDebug() << "[LOGIN-RESPONSE] Fetching user info from /auth/me...";
        getMe();
    } 
    // Support old format: {"data": {"token": "...", "user": {...}}}
    else if (obj.contains("data")) {
        qDebug() << "[LOGIN-RESPONSE] Old format detected (token in data)";
        QJsonObject data = obj["data"].toObject();
        token = data["token"].toString();
        QJsonObject user = data["user"].toObject();
        current_user_id = user["_id"].toString();
        current_username = user["username"].toString();
        
        qDebug() << "[LOGIN-RESPONSE] User ID:" << current_user_id;
        qDebug() << "[LOGIN-RESPONSE] Username:" << current_username;
        qDebug() << "[LOGIN-RESPONSE] Emitting loginSuccess";
        
        emit loginSuccess(data);
    } 
    else {
        qDebug() << "[LOGIN-RESPONSE] No 'token' or 'data' field in response!";
        QString errorMsg = obj["message"].toString("Unknown error");
        qDebug() << "[LOGIN-RESPONSE] Error message:" << errorMsg;
        emit loginFailed(errorMsg);
    }
    
    loginReply->deleteLater();
    loginReply = 0;
    qDebug() << "================================\n";
}

void ApiClient::onSignupFinished()
{
    if (!signupReply) return;
    
    if (signupReply->error() == QNetworkReply::NoError) {
        emit signupSuccess();
    } else {
        emit signupFailed(signupReply->errorString());
    }
    signupReply->deleteLater();
    signupReply = 0;
}

void ApiClient::onLogoutFinished()
{
    if (!logoutReply) return;
    
    token.clear();
    current_user_id.clear();
    current_username.clear();
    logoutReply->deleteLater();
    logoutReply = 0;
}

void ApiClient::onSearchFinished()
{
    if (!searchReply) return;
    
    if (searchReply->error() == QNetworkReply::NoError) {
        QByteArray responseData = searchReply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("data")) {
                QJsonArray users = obj["data"].toArray();
                emit searchResults(users);
            }
        }
    } else {
        emit searchFailed(searchReply->errorString());
    }
    searchReply->deleteLater();
    searchReply = 0;
}

void ApiClient::onMessageUsersFinished()
{
    if (!messageUsersReply) return;
    
    if (messageUsersReply->error() == QNetworkReply::NoError) {
        QByteArray responseData = messageUsersReply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("data")) {
                QJsonArray users = obj["data"].toArray();
                emit messageUsersReceived(users);
            }
        }
    }
    messageUsersReply->deleteLater();
    messageUsersReply = 0;
}

void ApiClient::onMessagesFinished()
{
    if (!messagesReply) return;
    
    if (messagesReply->error() == QNetworkReply::NoError) {
        QByteArray responseData = messagesReply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("data")) {
                QJsonArray messages = obj["data"].toArray();
                emit messagesReceived(messages);
            }
        }
    }
    messagesReply->deleteLater();
    messagesReply = 0;
}

void ApiClient::onUpdateProfileFinished()
{
    if (updateProfileReply) {
        updateProfileReply->deleteLater();
        updateProfileReply = 0;
    }
}

void ApiClient::onGetMeFinished()
{
    qDebug() << "[GET-ME-RESPONSE] Received response";
    
    if (!getMeReply) {
        qDebug() << "[GET-ME-RESPONSE] getMeReply is NULL";
        return;
    }
    
    qDebug() << "[GET-ME-RESPONSE] Error code:" << getMeReply->error();
    
    if (getMeReply->error() == QNetworkReply::NoError) {
        QByteArray responseData = getMeReply->readAll();
        qDebug() << "[GET-ME-RESPONSE] Raw response:" << responseData;
        
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            qDebug() << "[GET-ME-RESPONSE] JSON keys:" << obj.keys();
            
            QJsonObject user;
            
            // Try different response formats
            if (obj.contains("user")) {
                user = obj["user"].toObject();
                qDebug() << "[GET-ME-RESPONSE] Found 'user' field";
            } else if (obj.contains("data")) {
                user = obj["data"].toObject();
                qDebug() << "[GET-ME-RESPONSE] Found 'data' field";
            } else {
                // Assume root is the user object
                user = obj;
                qDebug() << "[GET-ME-RESPONSE] Using root object as user";
            }
            
            // Extract user info
            current_user_id = user["_id"].toString();
            current_username = user["username"].toString();
            
            qDebug() << "[GET-ME-RESPONSE] User ID:" << current_user_id;
            qDebug() << "[GET-ME-RESPONSE] Username:" << current_username;
        } else {
            qDebug() << "[GET-ME-RESPONSE] Response is not valid JSON";
        }
    } else {
        qDebug() << "[GET-ME-RESPONSE] Error:" << getMeReply->errorString();
    }
    
    getMeReply->deleteLater();
    getMeReply = 0;
}
