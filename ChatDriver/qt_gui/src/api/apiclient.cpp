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
    
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    }
    
    return request;
}

void ApiClient::login(const QString &username, const QString &password)
{
    QNetworkRequest request = createRequest("/api/auth/login");
    
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;
    
    QJsonDocument doc(json);
    loginReply = manager->post(request, doc.toJson());
    connect(loginReply, SIGNAL(finished()), this, SLOT(onLoginFinished()));
}

void ApiClient::signup(const QString &username, const QString &password)
{
    QNetworkRequest request = createRequest("/api/auth/signup");
    
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
    
    QNetworkRequest request = createRequest("/api/auth/logout");
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
    updateProfileReply = manager->sendCustomRequest(request, "PATCH", doc.toJson());
    connect(updateProfileReply, SIGNAL(finished()), this, SLOT(onUpdateProfileFinished()));
}

void ApiClient::getMe()
{
    QNetworkRequest request = createRequest("/api/auth/me");
    getMeReply = manager->get(request);
    connect(getMeReply, SIGNAL(finished()), this, SLOT(onGetMeFinished()));
}

void ApiClient::onLoginFinished()
{
    if (!loginReply) return;
    
    if (loginReply->error() == QNetworkReply::NoError) {
        QByteArray responseData = loginReply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("data")) {
                QJsonObject data = obj["data"].toObject();
                token = data["token"].toString();
                current_user_id = data["user"]["_id"].toString();
                current_username = data["user"]["username"].toString();
                emit loginSuccess(data);
            } else {
                emit loginFailed(obj["message"].toString("Unknown error"));
            }
        }
    } else {
        emit loginFailed(loginReply->errorString());
    }
    loginReply->deleteLater();
    loginReply = 0;
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
    if (!getMeReply) return;
    
    if (getMeReply->error() == QNetworkReply::NoError) {
        QByteArray responseData = getMeReply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            qDebug() << "User info:" << obj;
        }
    }
    getMeReply->deleteLater();
    getMeReply = 0;
}
