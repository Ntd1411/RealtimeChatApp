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
    : QObject(parent), base_url(baseUrl)
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
    QNetworkReply *reply = manager->post(request, doc.toJson());
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
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
            emit loginFailed(reply->errorString());
        }
        reply->deleteLater();
    });
}

void ApiClient::signup(const QString &username, const QString &password)
{
    QNetworkRequest request = createRequest("/api/auth/signup");
    
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;
    
    QJsonDocument doc(json);
    QNetworkReply *reply = manager->post(request, doc.toJson());
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            emit signupSuccess();
        } else {
            emit signupFailed(reply->errorString());
        }
        reply->deleteLater();
    });
}

void ApiClient::logout()
{
    if (token.isEmpty()) return;
    
    QNetworkRequest request = createRequest("/api/auth/logout");
    QNetworkReply *reply = manager->post(request, QByteArray());
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        token.clear();
        current_user_id.clear();
        current_username.clear();
        reply->deleteLater();
    });
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
    
    QNetworkReply *reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("data")) {
                    QJsonArray users = obj["data"].toArray();
                    emit searchResults(users);
                }
            }
        } else {
            emit searchFailed(reply->errorString());
        }
        reply->deleteLater();
    });
}

void ApiClient::getMessageUsers()
{
    QNetworkRequest request = createRequest("/api/message/users");
    QNetworkReply *reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("data")) {
                    QJsonArray users = obj["data"].toArray();
                    emit messageUsersReceived(users);
                }
            }
        }
        reply->deleteLater();
    });
}

void ApiClient::getMessages(const QString &userId)
{
    QNetworkRequest request = createRequest("/api/message/" + userId);
    QNetworkReply *reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("data")) {
                    QJsonArray messages = obj["data"].toArray();
                    emit messagesReceived(messages);
                }
            }
        }
        reply->deleteLater();
    });
}

void ApiClient::updateProfile(const QJsonObject &data)
{
    QNetworkRequest request = createRequest("/api/user/update");
    QJsonDocument doc(data);
    QNetworkReply *reply = manager->sendCustomRequest(request, "PATCH", doc.toJson());
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
    });
}

void ApiClient::getMe()
{
    QNetworkRequest request = createRequest("/api/auth/me");
    QNetworkReply *reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                qDebug() << "User info:" << obj;
            }
        }
        reply->deleteLater();
    });
}
