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

// Hàm chuẩn hóa tiếng Việt - bỏ dấu
QString ApiClient::normalizeVietnamese(const QString &input)
{
    QString result = input;
    
    // Map các ký tự có dấu tiếng Việt thành ký tự không dấu
    QMap<QString, QString> vietnameseMap;
    
    // a, ă, â
    vietnameseMap["á"] = "a"; vietnameseMap["à"] = "a"; vietnameseMap["ả"] = "a"; 
    vietnameseMap["ã"] = "a"; vietnameseMap["ạ"] = "a";
    vietnameseMap["ă"] = "a"; vietnameseMap["ắ"] = "a"; vietnameseMap["ằ"] = "a";
    vietnameseMap["ẳ"] = "a"; vietnameseMap["ẵ"] = "a"; vietnameseMap["ặ"] = "a";
    vietnameseMap["â"] = "a"; vietnameseMap["ấ"] = "a"; vietnameseMap["ầ"] = "a";
    vietnameseMap["ẩ"] = "a"; vietnameseMap["ẫ"] = "a"; vietnameseMap["ậ"] = "a";
    
    // e, ê
    vietnameseMap["é"] = "e"; vietnameseMap["è"] = "e"; vietnameseMap["ẻ"] = "e";
    vietnameseMap["ẽ"] = "e"; vietnameseMap["ẹ"] = "e";
    vietnameseMap["ê"] = "e"; vietnameseMap["ế"] = "e"; vietnameseMap["ề"] = "e";
    vietnameseMap["ể"] = "e"; vietnameseMap["ễ"] = "e"; vietnameseMap["ệ"] = "e";
    
    // i
    vietnameseMap["í"] = "i"; vietnameseMap["ì"] = "i"; vietnameseMap["ỉ"] = "i";
    vietnameseMap["ĩ"] = "i"; vietnameseMap["ị"] = "i";
    
    // o, ô, ơ
    vietnameseMap["ó"] = "o"; vietnameseMap["ò"] = "o"; vietnameseMap["ỏ"] = "o";
    vietnameseMap["õ"] = "o"; vietnameseMap["ọ"] = "o";
    vietnameseMap["ô"] = "o"; vietnameseMap["ố"] = "o"; vietnameseMap["ồ"] = "o";
    vietnameseMap["ổ"] = "o"; vietnameseMap["ỗ"] = "o"; vietnameseMap["ộ"] = "o";
    vietnameseMap["ơ"] = "o"; vietnameseMap["ớ"] = "o"; vietnameseMap["ờ"] = "o";
    vietnameseMap["ở"] = "o"; vietnameseMap["ỡ"] = "o"; vietnameseMap["ợ"] = "o";
    
    // u, ư
    vietnameseMap["ú"] = "u"; vietnameseMap["ù"] = "u"; vietnameseMap["ủ"] = "u";
    vietnameseMap["ũ"] = "u"; vietnameseMap["ụ"] = "u";
    vietnameseMap["ư"] = "u"; vietnameseMap["ứ"] = "u"; vietnameseMap["ừ"] = "u";
    vietnameseMap["ử"] = "u"; vietnameseMap["ữ"] = "u"; vietnameseMap["ự"] = "u";
    
    // y
    vietnameseMap["ý"] = "y"; vietnameseMap["ỳ"] = "y"; vietnameseMap["ỷ"] = "y";
    vietnameseMap["ỹ"] = "y"; vietnameseMap["ỵ"] = "y";
    
    // d
    vietnameseMap["đ"] = "d";
    
    // Uppercase
    vietnameseMap["Á"] = "A"; vietnameseMap["À"] = "A"; vietnameseMap["Ả"] = "A";
    vietnameseMap["Ã"] = "A"; vietnameseMap["Ạ"] = "A";
    vietnameseMap["Ă"] = "A"; vietnameseMap["Ắ"] = "A"; vietnameseMap["Ằ"] = "A";
    vietnameseMap["Ẳ"] = "A"; vietnameseMap["Ẵ"] = "A"; vietnameseMap["Ặ"] = "A";
    vietnameseMap["Â"] = "A"; vietnameseMap["Ấ"] = "A"; vietnameseMap["Ầ"] = "A";
    vietnameseMap["Ẩ"] = "A"; vietnameseMap["Ẫ"] = "A"; vietnameseMap["Ậ"] = "A";
    
    vietnameseMap["É"] = "E"; vietnameseMap["È"] = "E"; vietnameseMap["Ẻ"] = "E";
    vietnameseMap["Ẽ"] = "E"; vietnameseMap["Ẹ"] = "E";
    vietnameseMap["Ê"] = "E"; vietnameseMap["Ế"] = "E"; vietnameseMap["Ề"] = "E";
    vietnameseMap["Ể"] = "E"; vietnameseMap["Ễ"] = "E"; vietnameseMap["Ệ"] = "E";
    
    vietnameseMap["Í"] = "I"; vietnameseMap["Ì"] = "I"; vietnameseMap["Ỉ"] = "I";
    vietnameseMap["Ĩ"] = "I"; vietnameseMap["Ị"] = "I";
    
    vietnameseMap["Ó"] = "O"; vietnameseMap["Ò"] = "O"; vietnameseMap["Ỏ"] = "O";
    vietnameseMap["Õ"] = "O"; vietnameseMap["Ọ"] = "O";
    vietnameseMap["Ô"] = "O"; vietnameseMap["Ố"] = "O"; vietnameseMap["Ồ"] = "O";
    vietnameseMap["Ổ"] = "O"; vietnameseMap["Ỗ"] = "O"; vietnameseMap["Ộ"] = "O";
    vietnameseMap["Ơ"] = "O"; vietnameseMap["Ớ"] = "O"; vietnameseMap["Ờ"] = "O";
    vietnameseMap["Ở"] = "O"; vietnameseMap["Ỡ"] = "O"; vietnameseMap["Ợ"] = "O";
    
    vietnameseMap["Ú"] = "U"; vietnameseMap["Ù"] = "U"; vietnameseMap["Ủ"] = "U";
    vietnameseMap["Ũ"] = "U"; vietnameseMap["Ụ"] = "U";
    vietnameseMap["Ư"] = "U"; vietnameseMap["Ứ"] = "U"; vietnameseMap["Ừ"] = "U";
    vietnameseMap["Ử"] = "U"; vietnameseMap["Ữ"] = "U"; vietnameseMap["Ự"] = "U";
    
    vietnameseMap["Ý"] = "Y"; vietnameseMap["Ỳ"] = "Y"; vietnameseMap["Ỷ"] = "Y";
    vietnameseMap["Ỹ"] = "Y"; vietnameseMap["Ỵ"] = "Y";
    
    vietnameseMap["Đ"] = "D";
    
    // Replace tất cả ký tự có dấu
    for (QMap<QString, QString>::iterator it = vietnameseMap.begin(); it != vietnameseMap.end(); ++it) {
        result.replace(it.key(), it.value());
    }
    
    return result;
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

void ApiClient::signup(const QString &username, const QString &password, const QString &fullName, const QString &email)
{
    logToFile("================================");
    logToFile("[SIGNUP] Starting signup process");
    logToFile("[SIGNUP] Username: " + username);
    logToFile("[SIGNUP] Full Name: " + fullName);
    logToFile("[SIGNUP] Email: " + email);
    
    // Normalize tên và email bỏ dấu tiếng Việt
    QString normalizedFullName = this->normalizeVietnamese(fullName);
    QString normalizedUsername = this->normalizeVietnamese(username);
    
    QNetworkRequest request = createRequest("/auth/signup");
    
    QJsonObject json;
    json["username"] = normalizedUsername;
    json["password"] = password;
    json["fullName"] = normalizedFullName;
    json["email"] = email.toLower();
    
    QJsonDocument doc(json);
    logToFile("[SIGNUP] Request body: " + QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    
    signupReply = manager->post(request, doc.toJson());
    
    if (!signupReply) {
        logToFile("[SIGNUP] ERROR: Failed to create network reply!");
        emit signupFailed("Network error");
        return;
    }
    
    logToFile("[SIGNUP] Sending request...");
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
        loginReply = 0;
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
        loginReply = 0;
        return;
    }

    // === Normalize UTF-8 cho CentOS 6 (fix invalid UTF8 string) ===
    QString decodedStr = QString::fromUtf8(responseData.constData(), responseData.size());
    // Nếu vẫn có ký tự thay thế (ký tự lỗi), thử fromLatin1
    bool hasReplacement = false;
    for (int i = 0; i < decodedStr.length(); ++i) {
        if (decodedStr.at(i).unicode() == 0xFFFD) { hasReplacement = true; break; }
    }
    if (hasReplacement) {
        logToFile("[LOGIN-RESPONSE] Detected invalid UTF-8, trying fromLatin1...");
        decodedStr = QString::fromLatin1(responseData.constData(), responseData.size());
    }
    QByteArray normalizedData = decodedStr.toUtf8();
    logToFile("[LOGIN-RESPONSE] Normalized response: " + decodedStr.left(300));
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(normalizedData, &jsonError);
    if (doc.isNull()) {
        logToFile("[LOGIN-RESPONSE] ERROR: Still cannot parse JSON: " + jsonError.errorString() 
                  + " at offset " + QString::number(jsonError.offset));
        logToFile("[LOGIN-RESPONSE] First 200 chars: " + decodedStr.left(200));

        // Thử loại bỏ trường message (và ký tự không phải ASCII)
        QString asciiOnly = decodedStr;
        for (int i = 0; i < asciiOnly.length(); ++i) {
            if (asciiOnly.at(i).unicode() > 127) asciiOnly[i] = ' ';
        }
        // Loại bỏ trường "message": ... (dùng regex đơn giản)
        int msgIdx = asciiOnly.indexOf("\"message\"");
        if (msgIdx != -1) {
            int commaIdx = asciiOnly.indexOf(',', msgIdx);
            int braceIdx = asciiOnly.indexOf('}', msgIdx);
            int endIdx = (commaIdx != -1 && commaIdx < braceIdx) ? commaIdx+1 : braceIdx;
            if (endIdx > msgIdx) {
                asciiOnly.remove(msgIdx, endIdx - msgIdx);
            }
        }
        QByteArray asciiData = asciiOnly.toUtf8();
        logToFile("[LOGIN-RESPONSE] Try parse after removing message/ascii: " + asciiOnly.left(200));
        QJsonParseError jsonError2;
        QJsonDocument doc2 = QJsonDocument::fromJson(asciiData, &jsonError2);
        if (!doc2.isNull()) {
            logToFile("[LOGIN-RESPONSE] Parse OK after removing message/ascii!");
            doc = doc2;
        } else {
            logToFile("[LOGIN-RESPONSE] Still cannot parse after removing message/ascii: " + jsonError2.errorString());
            emit loginFailed("Invalid JSON response from server");
            loginReply->deleteLater();
            loginReply = 0;
            return;
        }
    }

    logToFile("[LOGIN-RESPONSE] JSON parsed successfully!");

    if (!doc.isObject()) {
        logToFile("[LOGIN-RESPONSE] ERROR: Response is not a JSON object!");
        emit loginFailed("Đăng nhập thất bại hoặc server lỗi");
        loginReply->deleteLater();
        loginReply = 0;
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
        // Error case: lấy message nếu có, nếu không thì trả về lỗi mặc định
        QString errorMsg = obj.contains("message") ? obj["message"].toString() : "Đăng nhập thất bại hoặc server lỗi";
        logToFile("[LOGIN-RESPONSE] No token or data field. Error: " + errorMsg);
        emit loginFailed(errorMsg);
    }

    loginReply->deleteLater();
    loginReply = 0;
    logToFile("[LOGIN-RESPONSE] Login processing finished.\n================================");
}

void ApiClient::onSignupFinished()
{
    logToFile("================================");
    logToFile("[SIGNUP-RESPONSE] Received response");
    
    if (!signupReply) {
        logToFile("[SIGNUP-RESPONSE] ERROR: signupReply is NULL!");
        emit signupFailed("No response from server");
        return;
    }
    
    int httpStatus = signupReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    logToFile("[SIGNUP-RESPONSE] HTTP Status: " + QString::number(httpStatus));
    logToFile("[SIGNUP-RESPONSE] Error code: " + QString::number(signupReply->error()));
    
    QByteArray responseData = signupReply->readAll();
    logToFile("[SIGNUP-RESPONSE] Response size: " + QString::number(responseData.size()) + " bytes");
    logToFile("[SIGNUP-RESPONSE] Raw response: " + QString::fromUtf8(responseData));
    
    // === Xử lý encoding - normalize ===
    responseData = responseData.trimmed();
    
    // Loại BOM UTF-8 nếu có
    if (responseData.size() >= 3 &&
        (unsigned char)responseData[0] == 0xEF &&
        (unsigned char)responseData[1] == 0xBB &&
        (unsigned char)responseData[2] == 0xBF) {
        responseData = responseData.mid(3);
    }
    
    if (responseData.isEmpty()) {
        logToFile("[SIGNUP-RESPONSE] ERROR: Response is EMPTY!");
        emit signupFailed("Server returned empty response");
        signupReply->deleteLater();
        signupReply = 0;
        return;
    }
    
    QString decodedStr = QString::fromUtf8(responseData.constData(), responseData.size());
    
    // Kiểm tra invalid UTF-8
    bool hasReplacement = false;
    for (int i = 0; i < decodedStr.length(); ++i) {
        if (decodedStr.at(i).unicode() == 0xFFFD) { hasReplacement = true; break; }
    }
    if (hasReplacement) {
        logToFile("[SIGNUP-RESPONSE] Detected invalid UTF-8, trying fromLatin1...");
        decodedStr = QString::fromLatin1(responseData.constData(), responseData.size());
    }
    
    QByteArray normalizedData = decodedStr.toUtf8();
    logToFile("[SIGNUP-RESPONSE] Normalized response: " + decodedStr.left(500));
    
    // === Remove message field (may contain Vietnamese characters that cause JSON parse issues) ===
    QString cleanedStr = decodedStr;
    int msgIdx = cleanedStr.indexOf("\"message\"");
    if (msgIdx != -1) {
        int colonIdx = cleanedStr.indexOf(':', msgIdx);
        int commaIdx = cleanedStr.indexOf(',', msgIdx);
        int braceIdx = cleanedStr.indexOf('}', msgIdx);
        
        if (colonIdx != -1) {
            // Find the end of the message value
            int endIdx = commaIdx;
            if (commaIdx == -1 || (braceIdx != -1 && braceIdx < commaIdx)) {
                endIdx = braceIdx;
            }
            if (endIdx > colonIdx) {
                // Remove the message field
                cleanedStr.remove(msgIdx, endIdx - msgIdx + 1);
                // If there's a trailing comma, clean it up
                cleanedStr = cleanedStr.replace(",}", "}").replace(",,", ",");
            }
        }
    }
    
    QByteArray cleanedData = cleanedStr.toUtf8();
    logToFile("[SIGNUP-RESPONSE] Cleaned response (message removed): " + cleanedStr.left(500));
    
    // === Parse JSON ===
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(cleanedData, &jsonError);
    
    if (doc.isNull()) {
        logToFile("[SIGNUP-RESPONSE] ERROR: Cannot parse JSON: " + jsonError.errorString());
        emit signupFailed("Invalid JSON response from server");
        signupReply->deleteLater();
        signupReply = 0;
        return;
    }
    
    if (!doc.isObject()) {
        logToFile("[SIGNUP-RESPONSE] ERROR: Response is not a JSON object!");
        emit signupFailed("Invalid response from server");
        signupReply->deleteLater();
        signupReply = 0;
        return;
    }
    
    QJsonObject obj = doc.object();
    logToFile("[SIGNUP-RESPONSE] JSON keys: " + obj.keys().join(", "));
    
    // === Kiểm tra status code ===
    if (httpStatus == 201 || httpStatus == 200) {
        // Đăng ký thành công
        logToFile("[SIGNUP-RESPONSE] Signup successful! Status: " + QString::number(httpStatus));
        
        QString message = obj["message"].toString("Đăng ký thành công");
        
        // Nếu có user info, store lại
        if (obj.contains("user")) {
            QJsonObject user = obj["user"].toObject();
            logToFile("[SIGNUP-RESPONSE] User created: " + user["username"].toString());
            logToFile("[SIGNUP-RESPONSE] Email: " + user["email"].toString());
            logToFile("[SIGNUP-RESPONSE] Full Name: " + user["fullName"].toString());
        }
        
        emit signupSuccess();
    }
    else {
        // Đăng ký thất bại
        logToFile("[SIGNUP-RESPONSE] Signup failed! Status: " + QString::number(httpStatus));
        
        QString errorMessage = obj["message"].toString("Đăng ký thất bại");
        if (obj.contains("error")) {
            errorMessage = obj["error"].toString(errorMessage);
        }
        
        logToFile("[SIGNUP-RESPONSE] Error message: " + errorMessage);
        emit signupFailed(errorMessage);
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
