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
    cryptoClient = new KernelCryptoClient();
    logToFile("[API-CLIENT] Crypto client initialized");
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

QString ApiClient::hashPassword(const QString &password)
{
    // Hash password using kernel crypto driver (SHA1)
    if (!cryptoClient || !cryptoClient->isOpen()) {
        logToFile("[hashPassword] ERROR: Crypto device not available, using plain password");
        return password;
    }
    
    QByteArray hashedBytes = cryptoClient->sha1Hash(password);
    if (hashedBytes.isEmpty()) {
        logToFile("[hashPassword] ERROR: SHA1 hash failed, using plain password");
        return password;
    }
    
    // Convert hash to hex string
    QString hashedHex = QString::fromLatin1(hashedBytes.toHex());
    logToFile("[hashPassword] Password hashed: " + password + " -> " + hashedHex.left(20) + "...");
    return hashedHex;
}

void ApiClient::login(const QString &username, const QString &password)
{
    logToFile("================================");
    logToFile("[LOGIN] Starting login process");
    logToFile("[LOGIN] Username: " + username);
    logToFile("[LOGIN] Base URL: " + base_url);
    
    QNetworkRequest request = createRequest("/auth/login");
    
    // Hash password using kernel crypto driver
    QString hashedPassword = hashPassword(password);
    
    QJsonObject json;
    json["username"] = username;
    json["password"] = hashedPassword;
    
    QJsonDocument doc(json);
    logToFile("[LOGIN] Request body: " + QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    
    loginReply = manager->post(request, doc.toJson());
    
    if (!loginReply) {
        logToFile("[LOGIN] ERROR: Failed to create network reply!");
        emit loginFailed("Lỗi mạng");
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
    
    // Hash password using kernel crypto driver
    QString hashedPassword = hashPassword(password);
    
    QNetworkRequest request = createRequest("/auth/signup");
    
    QJsonObject json;
    json["username"] = normalizedUsername;
    json["password"] = hashedPassword;
    json["fullName"] = normalizedFullName;
    json["email"] = email.toLower();
    
    QJsonDocument doc(json);
    logToFile("[SIGNUP] Request body: " + QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    
    signupReply = manager->post(request, doc.toJson());
    
    if (!signupReply) {
        logToFile("[SIGNUP] ERROR: Failed to create network reply!");
        emit signupFailed("Lỗi mạng");
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
    QUrl url(base_url + "/users/search");
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("keyword", query);
    url.setQuery(urlQuery);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    }
    
    logToFile("[SEARCH-USERS] Searching: " + query);
    logToFile("[SEARCH-USERS] URL: " + url.toString());
    searchReply = manager->get(request);
    
    if (!searchReply) {
        logToFile("[SEARCH-USERS] ERROR: Failed to create network reply!");
        emit searchFailed("Lỗi mạng");
        return;
    }
    
    connect(searchReply, SIGNAL(finished()), this, SLOT(onSearchFinished()));
}

void ApiClient::getMessageUsers()
{
    QNetworkRequest request = createRequest("/messages/users");
    logToFile("[GET-MESSAGE-USERS] Fetching message users");
    logToFile("[GET-MESSAGE-USERS] URL: " + base_url + "/messages/users");
    
    messageUsersReply = manager->get(request);
    
    if (!messageUsersReply) {
        logToFile("[GET-MESSAGE-USERS] ERROR: Failed to create network reply!");
        emit error("Lỗi mạng");
        return;
    }
    
    connect(messageUsersReply, SIGNAL(finished()), this, SLOT(onMessageUsersFinished()));
}

void ApiClient::getMessages(const QString &userId)
{
    QNetworkRequest request = createRequest("/messages/" + userId);
    logToFile("[GET-MESSAGES] Fetching messages for user: " + userId);
    logToFile("[GET-MESSAGES] URL: " + base_url + "/messages/" + userId);
    
    messagesReply = manager->get(request);
    
    if (!messagesReply) {
        logToFile("[GET-MESSAGES] ERROR: Failed to create network reply!");
        emit error("Lỗi mạng");
        return;
    }
    
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
        emit loginFailed("Không có phản hồi từ máy chủ");
        return;
    }

    int httpStatus = loginReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    logToFile("[LOGIN-RESPONSE] HTTP Status: " + QString::number(httpStatus));
    logToFile("[LOGIN-RESPONSE] Error code: " + QString::number(loginReply->error()));

    QByteArray responseData = loginReply->readAll();
    logToFile("[LOGIN-RESPONSE] Response size: " + QString::number(responseData.size()) + " bytes");

    // Check network error only if there's no response data
    if (responseData.isEmpty() && loginReply->error() != QNetworkReply::NoError) {
        QString errorStr = loginReply->errorString();
        logToFile("[LOGIN-RESPONSE] Network Error: " + errorStr);
        emit loginFailed("Lỗi mạng: " + errorStr);
        loginReply->deleteLater();
        loginReply = 0;
        return;
    }

    if (responseData.isEmpty()) {
        logToFile("[LOGIN-RESPONSE] ERROR: Response is EMPTY!");
        emit loginFailed("Máy chủ trả về phản hồi trống");
        loginReply->deleteLater();
        loginReply = 0;
        return;
    }

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
        emit loginFailed("Máy chủ trả về phản hồi trống");
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
    
    // === Check HTTP status first to determine success/failure ===
    if (httpStatus >= 400) {
        // Failure case - extract error message using string parsing
        logToFile("[LOGIN-RESPONSE] Login failed! Status: " + QString::number(httpStatus));
        
        // Extract message value
        QString errorMessage = "Đăng nhập thất bại";
        int msgIdx = decodedStr.indexOf("\"message\"");
        if (msgIdx != -1) {
            int startIdx = decodedStr.indexOf('\"', msgIdx + 10); // Find opening quote after colon
            if (startIdx != -1) {
                int endIdx = decodedStr.indexOf('\"', startIdx + 1); // Find closing quote
                if (endIdx != -1) {
                    errorMessage = decodedStr.mid(startIdx + 1, endIdx - startIdx - 1);
                }
            }
        }
        
        logToFile("[LOGIN-RESPONSE] Error message: " + errorMessage);
        emit loginFailed(errorMessage);
        loginReply->deleteLater();
        loginReply = 0;
        return;
    }
    
    // === Success case - remove message field and parse JSON ===
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
    logToFile("[LOGIN-RESPONSE] Cleaned response (message removed): " + cleanedStr.left(300));
    
    // === Parse JSON ===
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(cleanedData, &jsonError);
    if (doc.isNull()) {
        logToFile("[LOGIN-RESPONSE] ERROR: Cannot parse JSON: " + jsonError.errorString());
        emit loginFailed("Phản hồi JSON không hợp lệ từ máy chủ");
        loginReply->deleteLater();
        loginReply = 0;
        return;
    }

    if (!doc.isObject()) {
        logToFile("[LOGIN-RESPONSE] ERROR: Response is not a JSON object!");
        emit loginFailed("Phản hồi không hợp lệ từ máy chủ");
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
        emit signupFailed("Không có phản hồi từ máy chủ");
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
        emit signupFailed("Máy chủ trả về phản hồi trống");
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
    
    // === Check HTTP status first to determine success/failure ===
    if (httpStatus >= 400) {
        // Failure case - extract error message using regex/string parsing
        logToFile("[SIGNUP-RESPONSE] Signup failed! Status: " + QString::number(httpStatus));
        
        // Extract message value using regex-like approach
        QString errorMessage = "Đăng ký thất bại";
        int msgIdx = decodedStr.indexOf("\"message\"");
        if (msgIdx != -1) {
            int startIdx = decodedStr.indexOf('\"', msgIdx + 10); // Find opening quote after colon
            if (startIdx != -1) {
                int endIdx = decodedStr.indexOf('\"', startIdx + 1); // Find closing quote
                if (endIdx != -1) {
                    errorMessage = decodedStr.mid(startIdx + 1, endIdx - startIdx - 1);
                }
            }
        }
        
        logToFile("[SIGNUP-RESPONSE] Error message: " + errorMessage);
        emit signupFailed(errorMessage);
        signupReply->deleteLater();
        signupReply = 0;
        return;
    }
    
    // === Success case - remove message field and parse JSON ===
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
        emit signupFailed("Phản hồi JSON không hợp lệ từ máy chủ");
        signupReply->deleteLater();
        signupReply = 0;
        return;
    }
    
    if (!doc.isObject()) {
        logToFile("[SIGNUP-RESPONSE] ERROR: Response is not a JSON object!");
        emit signupFailed("Phản hồi không hợp lệ từ máy chủ");
        signupReply->deleteLater();
        signupReply = 0;
        return;
    }
    
    QJsonObject obj = doc.object();
    logToFile("[SIGNUP-RESPONSE] JSON keys: " + obj.keys().join(", "));
    
    // === Success case - extract user info ===
    logToFile("[SIGNUP-RESPONSE] Signup successful! Status: " + QString::number(httpStatus));
    
    // Nếu có user info, store lại
    if (obj.contains("user")) {
        QJsonObject user = obj["user"].toObject();
        logToFile("[SIGNUP-RESPONSE] User created: " + user["username"].toString());
        logToFile("[SIGNUP-RESPONSE] Email: " + user["email"].toString());
        logToFile("[SIGNUP-RESPONSE] Full Name: " + user["fullName"].toString());
    }
    
    emit signupSuccess();
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
    logToFile("[SEARCH-RESPONSE] Received response");
    
    if (!searchReply) {
        logToFile("[SEARCH-RESPONSE] ERROR: searchReply is NULL!");
        emit searchFailed("Không có phản hồi từ máy chủ");
        return;
    }
    
    int httpStatus = searchReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    logToFile("[SEARCH-RESPONSE] HTTP Status: " + QString::number(httpStatus));
    
    QByteArray responseData = searchReply->readAll();
    
    if (searchReply->error() != QNetworkReply::NoError) {
        QString errorMsg = searchReply->errorString();
        logToFile("[SEARCH-RESPONSE] Network Error: " + errorMsg);
        emit searchFailed("Lỗi mạng: " + errorMsg);
        searchReply->deleteLater();
        searchReply = 0;
        return;
    }
    
    // === Handle encoding ===
    responseData = responseData.trimmed();
    
    // Remove UTF-8 BOM if present
    if (responseData.size() >= 3 &&
        (unsigned char)responseData[0] == 0xEF &&
        (unsigned char)responseData[1] == 0xBB &&
        (unsigned char)responseData[2] == 0xBF) {
        logToFile("[SEARCH-RESPONSE] Removing UTF-8 BOM");
        responseData = responseData.mid(3);
    }
    
    if (responseData.isEmpty()) {
        logToFile("[SEARCH-RESPONSE] ERROR: Response is EMPTY!");
        emit searchFailed("Máy chủ trả về phản hồi trống");
        searchReply->deleteLater();
        searchReply = 0;
        return;
    }
    
    logToFile("[SEARCH-RESPONSE] Raw response: " + QString::fromLatin1(responseData));
    
    // === Remove message field to avoid UTF-8 encoding issues ===
    QString responseStr = QString::fromLatin1(responseData.constData(), responseData.size());
    
    int msgIdx = responseStr.indexOf("\"message\"");
    if (msgIdx != -1) {
        int colonIdx = responseStr.indexOf(':', msgIdx);
        int commaIdx = responseStr.indexOf(',', msgIdx);
        int braceIdx = responseStr.indexOf('}', msgIdx);
        
        if (colonIdx != -1) {
            int endIdx = commaIdx;
            if (commaIdx == -1 || (braceIdx != -1 && braceIdx < commaIdx)) {
                endIdx = braceIdx;
            }
            if (endIdx > colonIdx) {
                responseStr.remove(msgIdx, endIdx - msgIdx + 1);
                responseStr = responseStr.replace(",}", "}").replace(",,", ",");
                logToFile("[SEARCH-RESPONSE] Removed message field");
            }
        }
    }
    
    QByteArray cleanedData = responseStr.toUtf8();
    logToFile("[SEARCH-RESPONSE] Cleaned response: " + responseStr.left(200));
    
    // === Parse JSON ===
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(cleanedData, &jsonError);
    
    if (doc.isNull()) {
        logToFile("[SEARCH-RESPONSE] ERROR: Cannot parse JSON: " + jsonError.errorString());
        emit searchFailed("Phản hồi JSON không hợp lệ");
        searchReply->deleteLater();
        searchReply = 0;
        return;
    }
    
    if (!doc.isObject()) {
        logToFile("[SEARCH-RESPONSE] ERROR: Response is not a JSON object!");
        emit searchFailed("Phản hồi không hợp lệ");
        searchReply->deleteLater();
        searchReply = 0;
        return;
    }
    
    QJsonObject obj = doc.object();
    logToFile("[SEARCH-RESPONSE] JSON keys: " + obj.keys().join(", "));
    
    // === Extract users array ===
    if (httpStatus >= 400) {
        QString errorMessage = "Tìm kiếm thất bại";
        emit searchFailed(errorMessage);
    } else {
        QJsonArray users;
        if (obj.contains("users") && obj["users"].isArray()) {
            users = obj["users"].toArray();
            logToFile("[SEARCH-RESPONSE] Found " + QString::number(users.size()) + " users");
        } else {
            logToFile("[SEARCH-RESPONSE] WARNING: 'users' field not found");
        }
        emit searchResults(users);
    }
    
    searchReply->deleteLater();
    searchReply = 0;
}

void ApiClient::onMessageUsersFinished()
{
    logToFile("[GET-MESSAGE-USERS-RESPONSE] Received response");
    
    if (!messageUsersReply) {
        logToFile("[GET-MESSAGE-USERS-RESPONSE] ERROR: messageUsersReply is NULL!");
        emit error("Không có phản hồi từ máy chủ");
        return;
    }
    
    int httpStatus = messageUsersReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    logToFile("[GET-MESSAGE-USERS-RESPONSE] HTTP Status: " + QString::number(httpStatus));
    
    QByteArray responseData = messageUsersReply->readAll();
    
    if (messageUsersReply->error() != QNetworkReply::NoError) {
        QString errorMsg = messageUsersReply->errorString();
        logToFile("[GET-MESSAGE-USERS-RESPONSE] Network Error: " + errorMsg);
        emit error("Lỗi mạng: " + errorMsg);
        messageUsersReply->deleteLater();
        messageUsersReply = 0;
        return;
    }
    
    // === Handle encoding ===
    responseData = responseData.trimmed();
    
    // Remove UTF-8 BOM if present
    if (responseData.size() >= 3 &&
        (unsigned char)responseData[0] == 0xEF &&
        (unsigned char)responseData[1] == 0xBB &&
        (unsigned char)responseData[2] == 0xBF) {
        logToFile("[GET-MESSAGE-USERS-RESPONSE] Removing UTF-8 BOM");
        responseData = responseData.mid(3);
    }
    
    if (responseData.isEmpty()) {
        logToFile("[GET-MESSAGE-USERS-RESPONSE] ERROR: Response is EMPTY!");
        emit error("Máy chủ trả về phản hồi trống");
        messageUsersReply->deleteLater();
        messageUsersReply = 0;
        return;
    }
    
    logToFile("[GET-MESSAGE-USERS-RESPONSE] Raw response: " + QString::fromLatin1(responseData.left(200)));
    
    // === Parse JSON ===
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &jsonError);
    
    if (doc.isNull()) {
        logToFile("[GET-MESSAGE-USERS-RESPONSE] ERROR: Cannot parse JSON: " + jsonError.errorString());
        emit error("Phản hồi JSON không hợp lệ");
        messageUsersReply->deleteLater();
        messageUsersReply = 0;
        return;
    }
    
    if (!doc.isObject()) {
        logToFile("[GET-MESSAGE-USERS-RESPONSE] ERROR: Response is not a JSON object!");
        emit error("Phản hồi không hợp lệ");
        messageUsersReply->deleteLater();
        messageUsersReply = 0;
        return;
    }
    
    QJsonObject obj = doc.object();
    logToFile("[GET-MESSAGE-USERS-RESPONSE] JSON keys: " + obj.keys().join(", "));
    
    // === Extract users array ===
    if (httpStatus >= 400) {
        QString errorMessage = "Lỗi lấy danh sách người dùng";
        logToFile("[GET-MESSAGE-USERS-RESPONSE] Error: " + errorMessage);
        emit error(errorMessage);
    } else {
        QJsonArray users;
        if (obj.contains("users") && obj["users"].isArray()) {
            users = obj["users"].toArray();
            logToFile("[GET-MESSAGE-USERS-RESPONSE] Found " + QString::number(users.size()) + " users");
        } else {
            logToFile("[GET-MESSAGE-USERS-RESPONSE] WARNING: 'users' field not found");
        }
        emit messageUsersReceived(users);
    }
    
    messageUsersReply->deleteLater();
    messageUsersReply = 0;
}

void ApiClient::onMessagesFinished()
{
    logToFile("[GET-MESSAGES-RESPONSE] Received response");
    
    if (!messagesReply) {
        logToFile("[GET-MESSAGES-RESPONSE] ERROR: messagesReply is NULL!");
        emit error("Không có phản hồi từ máy chủ");
        return;
    }
    
    int httpStatus = messagesReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    logToFile("[GET-MESSAGES-RESPONSE] HTTP Status: " + QString::number(httpStatus));
    
    QByteArray responseData = messagesReply->readAll();
    
    if (messagesReply->error() != QNetworkReply::NoError) {
        QString errorMsg = messagesReply->errorString();
        logToFile("[GET-MESSAGES-RESPONSE] Network Error: " + errorMsg);
        emit error("Lỗi mạng: " + errorMsg);
        messagesReply->deleteLater();
        messagesReply = 0;
        return;
    }
    
    // === Handle encoding ===
    responseData = responseData.trimmed();
    
    // Remove UTF-8 BOM if present
    if (responseData.size() >= 3 &&
        (unsigned char)responseData[0] == 0xEF &&
        (unsigned char)responseData[1] == 0xBB &&
        (unsigned char)responseData[2] == 0xBF) {
        logToFile("[GET-MESSAGES-RESPONSE] Removing UTF-8 BOM");
        responseData = responseData.mid(3);
    }
    
    if (responseData.isEmpty()) {
        logToFile("[GET-MESSAGES-RESPONSE] ERROR: Response is EMPTY!");
        emit error("Máy chủ trả về phản hồi trống");
        messagesReply->deleteLater();
        messagesReply = 0;
        return;
    }
    
    // Try UTF-8 first, then fallback to Latin1
    QString decodedStr = QString::fromUtf8(responseData.constData(), responseData.size());
    bool hasReplacement = false;
    for (int i = 0; i < decodedStr.length(); ++i) {
        if (decodedStr.at(i).unicode() == 0xFFFD) { 
            hasReplacement = true; 
            break; 
        }
    }
    
    if (hasReplacement) {
        logToFile("[GET-MESSAGES-RESPONSE] Detected invalid UTF-8, trying Latin1...");
        decodedStr = QString::fromLatin1(responseData.constData(), responseData.size());
    }
    
    logToFile("[GET-MESSAGES-RESPONSE] Decoded response length: " + QString::number(decodedStr.length()));
    logToFile("[GET-MESSAGES-RESPONSE] Sample: " + decodedStr.left(200));
    
    // Convert back to UTF-8 bytes for JSON parsing
    QByteArray cleanedData = decodedStr.toUtf8();
    
    // === Parse JSON ===
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(cleanedData, &jsonError);
    
    if (doc.isNull()) {
        logToFile("[GET-MESSAGES-RESPONSE] ERROR: Cannot parse JSON: " + jsonError.errorString());
        logToFile("[GET-MESSAGES-RESPONSE] Error offset: " + QString::number(jsonError.offset));
        emit error("Phản hồi JSON không hợp lệ");
        messagesReply->deleteLater();
        messagesReply = 0;
        return;
    }
    
    if (!doc.isObject()) {
        logToFile("[GET-MESSAGES-RESPONSE] ERROR: Response is not a JSON object!");
        emit error("Phản hồi không hợp lệ");
        messagesReply->deleteLater();
        messagesReply = 0;
        return;
    }
    
    QJsonObject obj = doc.object();
    logToFile("[GET-MESSAGES-RESPONSE] JSON keys: " + obj.keys().join(", "));
    
    // === Extract messages array ===
    if (httpStatus >= 400) {
        QString errorMessage = "Lỗi lấy tin nhắn";
        logToFile("[GET-MESSAGES-RESPONSE] Error: " + errorMessage);
        emit error(errorMessage);
    } else {
        QJsonArray messages;
        if (obj.contains("messages") && obj["messages"].isArray()) {
            messages = obj["messages"].toArray();
            logToFile("[GET-MESSAGES-RESPONSE] Found " + QString::number(messages.size()) + " messages");
            
            // Extract and log first message as debug
            if (messages.size() > 0) {
                QJsonObject firstMsg = messages[0].toObject();
                logToFile("[GET-MESSAGES-RESPONSE] First message content: " + firstMsg["content"].toString().left(100));
            }
        } else {
            logToFile("[GET-MESSAGES-RESPONSE] WARNING: 'messages' field not found");
        }
        emit messagesReceived(messages);
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
