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

#include <QFile>
#include <QTextStream>

// Hàm ghi log ra file
void logToFile(const QString &msg) {
    QFile file("chatclient.log");
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << msg << "\n";
        file.close();
    }
}

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
        logToFile("[createRequest] Added auth token");
    }
    
    logToFile("[createRequest] URL: " + url.toString());
    return request;
}

void ApiClient::login(const QString &username, const QString &password)
{
    logToFile("================================");
    logToFile("[LOGIN] Starting login process");
    logToFile("[LOGIN] Username: " + username);
    logToFile("[LOGIN] Base URL: " + base_url);
    
    QNetworkRequest request = createRequest("/auth/login");
    
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;
    
    QJsonDocument doc(json);
    logToFile("[LOGIN] Request body: " + QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    
    loginReply = manager->post(request, doc.toJson());
    
    if (!loginReply) {
        logToFile("[LOGIN] ERROR: Failed to create network reply!");
        emit loginFailed("Network error");
        return;
    }
    
    logToFile("[LOGIN] Sending request...");
    connect(loginReply, SIGNAL(finished()), this, SLOT(onLoginFinished()));
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
    logToFile("[GET-ME] Fetching user info from /auth/me");
    QNetworkRequest request = createRequest("/auth/me");
    getMeReply = manager->get(request);
    
    if (!getMeReply) {
        logToFile("[GET-ME] Failed to create request");
        return;
    }
    
    logToFile("[GET-ME] Sending request...");
    connect(getMeReply, SIGNAL(finished()), this, SLOT(onGetMeFinished()));
}

void ApiClient::onLoginFinished()
{
    logToFile("[LOGIN-RESPONSE] Received response");

    if (!loginReply) {
        logToFile("[LOGIN-RESPONSE] ERROR: loginReply is NULL!");
        emit loginFailed("No response from server");
        return;
    }

    int httpStatus = loginReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    logToFile("[LOGIN-RESPONSE] HTTP Status: " + QString::number(httpStatus));
    logToFile("[LOGIN-RESPONSE] Error code: " + QString::number(loginReply->error()));

    if (loginReply->error() != QNetworkReply::NoError) {
        QString errorStr = loginReply->errorString();
        logToFile("[LOGIN-RESPONSE] Network Error: " + errorStr);
        emit loginFailed("Network error: " + errorStr);
        loginReply->deleteLater();
        loginReply = nullptr;
        return;
    }

    QByteArray responseData = loginReply->readAll();
    logToFile("[LOGIN-RESPONSE] Response size: " + QString::number(responseData.size()) + " bytes");

    // Log raw response (có thể chứa ký tự lạ)
    logToFile("[LOGIN-RESPONSE] Raw response: " + QString::fromLatin1(responseData));

    // Hex dump để debug
    QString hexDump;
    for (int i = 0; i < responseData.size(); ++i) {
        hexDump += QString::number((unsigned char)responseData[i], 16).rightJustified(2, '0').toUpper();
        if ((i + 1) % 16 == 0) hexDump += "\n";
        else if ((i + 1) % 2 == 0) hexDump += " ";
    }
    logToFile("[LOGIN-RESPONSE] Hex dump:\n" + hexDump);

    // Log Content-Type từ server
    QString contentType = loginReply->header(QNetworkRequest::ContentTypeHeader).toString();
    logToFile("[LOGIN-RESPONSE] Content-Type: " + contentType);

    // ================== XỬ LÝ ENCODING - PHẦN QUAN TRỌNG NHẤT ==================
    responseData = responseData.trimmed();  // trim whitespace + \r\n

    // Loại BOM UTF-8 nếu có
    if (responseData.size() >= 3 &&
        (unsigned char)responseData[0] == 0xEF &&
        (unsigned char)responseData[1] == 0xBB &&
        (unsigned char)responseData[2] == 0xBF) {
        logToFile("[LOGIN-RESPONSE] Detected and removed UTF-8 BOM");
        responseData = responseData.mid(3);
    }

    if (responseData.isEmpty()) {
        logToFile("[LOGIN-RESPONSE] ERROR: Response is EMPTY after trim!");
        emit loginFailed("Server returned empty response");
        loginReply->deleteLater();
        loginReply = nullptr;
        return;
    }

    // === Normalize UTF-8 cho CentOS 6 (fix invalid UTF8 string) ===
    QString decodedStr;
    QByteArray normalizedData;

    // Cách 1: fromUtf8 với replacement cho ký tự lỗi
    decodedStr = QString::fromUtf8(responseData.constData(), responseData.size(), 
                                   QString::ReplacementForInvalidUtf8);

    // Cách 2: Nếu vẫn có ký tự thay thế → thử fromLatin1 (thường hiệu quả với tiếng Việt)
    if (decodedStr.contains(QChar::ReplacementCharacter)) {
        logToFile("[LOGIN-RESPONSE] Detected invalid UTF-8, trying fromLatin1...");
        decodedStr = QString::fromLatin1(responseData.constData(), responseData.size());
    }

    // Chuẩn hóa lại thành UTF-8 sạch
    normalizedData = decodedStr.toUtf8();

    logToFile("[LOGIN-RESPONSE] Normalized response: " + decodedStr.left(300));

    // Parse JSON từ dữ liệu đã normalize
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(normalizedData, &jsonError);

    if (doc.isNull()) {
        logToFile("[LOGIN-RESPONSE] ERROR: Still cannot parse JSON: " + jsonError.errorString() 
                  + " at offset " + QString::number(jsonError.offset));
        logToFile("[LOGIN-RESPONSE] First 200 chars: " + decodedStr.left(200));

        emit loginFailed("Invalid JSON response from server");
        loginReply->deleteLater();
        loginReply = nullptr;
        return;
    }

    logToFile("[LOGIN-RESPONSE] JSON parsed successfully!");

    if (!doc.isObject()) {
        logToFile("[LOGIN-RESPONSE] ERROR: Response is not a JSON object!");
        emit loginFailed("Invalid response format from server");
        loginReply->deleteLater();
        loginReply = nullptr;
        return;
    }

    QJsonObject obj = doc.object();
    logToFile("[LOGIN-RESPONSE] JSON keys: " + obj.keys().join(", "));

    // === XỬ LÝ 2 FORMAT JSON ===
    if (obj.contains("token")) {
        // New format: {"message": "...", "token": "..."}
        token = obj["token"].toString();
        logToFile("[LOGIN-RESPONSE] New format detected - Token received");
        logToFile("[LOGIN-RESPONSE] Token length: " + QString::number(token.length()));
        emit loginSuccess(obj);

        logToFile("[LOGIN-RESPONSE] Fetching user info from /auth/me...");
        getMe();
    }
    else if (obj.contains("data")) {
        // Old format: {"data": {"token": "...", "user": {...}}}
        logToFile("[LOGIN-RESPONSE] Old format detected");
        QJsonObject dataObj = obj["data"].toObject();
        token = dataObj["token"].toString();

        QJsonObject user = dataObj["user"].toObject();
        current_user_id = user["_id"].toString();
        current_username = user["username"].toString();

        logToFile("[LOGIN-RESPONSE] User ID: " + current_user_id);
        logToFile("[LOGIN-RESPONSE] Username: " + current_username);
        emit loginSuccess(dataObj);
    }
    else {
        // Error case
        QString errorMsg = obj["message"].toString("Unknown error from server");
        logToFile("[LOGIN-RESPONSE] No token or data field. Error: " + errorMsg);
        emit loginFailed(errorMsg);
    }

    loginReply->deleteLater();
    loginReply = nullptr;
    logToFile("[LOGIN-RESPONSE] Login processing finished.\n================================");
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
    logToFile("[GET-ME-RESPONSE] Received response");
    
    if (!getMeReply) {
        logToFile("[GET-ME-RESPONSE] getMeReply is NULL");
        return;
    }
    
    logToFile("[GET-ME-RESPONSE] Error code: " + QString::number(getMeReply->error()));
    
    if (getMeReply->error() == QNetworkReply::NoError) {
        QByteArray responseData = getMeReply->readAll();
        logToFile("[GET-ME-RESPONSE] Raw response: " + QString::fromUtf8(responseData));
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            logToFile("[GET-ME-RESPONSE] JSON keys: " + obj.keys().join(", "));
            QJsonObject user;
            // Try different response formats
            if (obj.contains("user")) {
                user = obj["user"].toObject();
                logToFile("[GET-ME-RESPONSE] Found 'user' field");
            } else if (obj.contains("data")) {
                user = obj["data"].toObject();
                logToFile("[GET-ME-RESPONSE] Found 'data' field");
            } else {
                // Assume root is the user object
                user = obj;
                logToFile("[GET-ME-RESPONSE] Using root object as user");
            }
            // Extract user info
            current_user_id = user["_id"].toString();
            current_username = user["username"].toString();
            logToFile("[GET-ME-RESPONSE] User ID: " + current_user_id);
            logToFile("[GET-ME-RESPONSE] Username: " + current_username);
        } else {
            logToFile("[GET-ME-RESPONSE] Response is not valid JSON");
        }
    } else {
        logToFile("[GET-ME-RESPONSE] Error: " + getMeReply->errorString());
    }
    
    getMeReply->deleteLater();
    getMeReply = 0;
}
