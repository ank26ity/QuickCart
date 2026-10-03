/**
 * @file mockapiserver.cpp
 * @brief Implementation of MockApiServer for integration test flows.
 * @layer Tests / Tools (C++ / Qt Network)
 */

#include "mockapiserver.h"
#include <QtCore/QJsonDocument>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QUuid>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <cmath>

static double calculateHaversine(double lat1, double lon1, double lat2, double lon2) {
    constexpr double R = 6371.0;
    double dLat = (lat2 - lat1) * 3.14159265358979323846 / 180.0;
    double dLon = (lon2 - lon1) * 3.14159265358979323846 / 180.0;
    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) + std::cos(lat1 * 3.14159265358979323846 / 180.0) *
                                                                 std::cos(lat2 * 3.14159265358979323846 / 180.0) *
                                                                 std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return R * c;
}

MockApiServer::MockApiServer(QObject *parent) : QObject(parent), m_server(new QTcpServer(this)) {
    connect(m_server, &QTcpServer::newConnection, this, &MockApiServer::handleNewConnection);
    resetData();
}

MockApiServer::~MockApiServer() {
    stop();
}

bool MockApiServer::start() {
    if (m_server->listen(QHostAddress::LocalHost, 0)) {
        m_port = m_server->serverPort();
        return true;
    }
    return false;
}

void MockApiServer::stop() {
    if (m_server->isListening()) {
        m_server->close();
    }
}

QString MockApiServer::url() const {
    return QString(QStringLiteral("http://127.0.0.1:%1")).arg(m_port);
}

quint16 MockApiServer::port() const {
    return m_port;
}

void MockApiServer::setFailNextRequests(int count, int statusCode) {
    m_failCount = count;
    m_failStatusCode = statusCode;
}

void MockApiServer::resetData() {
    m_users.clear();
    m_shops.clear();
    m_products.clear();
    m_orders.clear();
    m_orderClaims.clear();

    // Seed test users
    QJsonObject cust;
    cust["_id"] = "user_cust_1";
    cust["name"] = "Alice Customer";
    cust["email"] = "customer@quickcart.com";
    cust["phone"] = "+919876543210";
    cust["role"] = "customer";
    m_users["customer@quickcart.com"] = cust;
    m_users["alice@quickcart.com"] = cust;
    m_users["+919876543210"] = cust;

    QJsonObject merch;
    merch["_id"] = "user_merch_1";
    merch["name"] = "Bob Merchant";
    merch["email"] = "merchant@quickcart.com";
    merch["role"] = "shopkeeper";
    merch["shopId"] = "shop_1";
    m_users["merchant@quickcart.com"] = merch;
    m_users["bob@quickcart.com"] = merch;

    QJsonObject courier;
    courier["_id"] = "user_courier_1";
    courier["name"] = "Charlie Courier";
    courier["email"] = "charlie@quickcart.com";
    courier["role"] = "delivery";
    courier["complianceStatus"] = "not_submitted";
    m_users["charlie@quickcart.com"] = courier;

    QJsonObject courierVerified = courier;
    courierVerified["email"] = "courier@quickcart.com";
    courierVerified["complianceStatus"] = "verified";
    m_users["courier@quickcart.com"] = courierVerified;

    QJsonObject admin;
    admin["_id"] = "user_admin_1";
    admin["name"] = "Admin User";
    admin["email"] = "admin@quickcart.com";
    admin["role"] = "admin";
    m_users["admin@quickcart.com"] = admin;

    // Seed test shops
    QJsonObject shop1;
    shop1["_id"] = "shop_1";
    shop1["name"] = "Fresh Mart Daily";
    shop1["category"] = "groceries";
    shop1["description"] = "Daily organic vegetables and dairy";
    shop1["lat"] = 12.9716;
    shop1["lng"] = 77.5946;
    shop1["rating"] = 4.8;
    shop1["address"] = "100 Feet Rd, Indiranagar";
    m_shops["shop_1"] = shop1;

    QJsonObject shop2;
    shop2["_id"] = "shop_2";
    shop2["name"] = "Corner Pharmacy";
    shop2["category"] = "pharmacy";
    shop2["description"] = "24/7 medicines and wellness";
    shop2["lat"] = 12.9750;
    shop2["lng"] = 77.5980;
    shop2["rating"] = 4.9;
    shop2["address"] = "MG Road, Bangalore";
    m_shops["shop_2"] = shop2;

    // Shop 3: 15 km away, beyond 3km radius
    QJsonObject shop3;
    shop3["_id"] = "shop_3";
    shop3["name"] = "Faraway Hypermarket";
    shop3["category"] = "groceries";
    shop3["lat"] = 12.8399;
    shop3["lng"] = 77.6770;
    shop3["rating"] = 4.2;
    m_shops["shop_3"] = shop3;

    // Shop 4 & 5: Central Delhi (at 28.6139, 77.2090)
    QJsonObject shop4;
    shop4["_id"] = "shop_4";
    shop4["name"] = "Delhi Express Mart";
    shop4["category"] = "groceries";
    shop4["description"] = "Instant groceries and dairy delivered in 10 mins";
    shop4["lat"] = 28.6139;
    shop4["lng"] = 77.2090;
    shop4["rating"] = 4.9;
    shop4["address"] = "Connaught Place, New Delhi";
    shop4["isOpen"] = true;
    m_shops["shop_4"] = shop4;

    QJsonObject shop5;
    shop5["_id"] = "shop_5";
    shop5["name"] = "Apollo Pharmacy Express";
    shop5["category"] = "pharmacy";
    shop5["description"] = "Essential medicines & healthcare supplies";
    shop5["lat"] = 28.6145;
    shop5["lng"] = 77.2095;
    shop5["rating"] = 4.8;
    shop5["address"] = "Barakhamba Road, New Delhi";
    shop5["isOpen"] = true;
    m_shops["shop_5"] = shop5;

    // Seed test products
    QJsonObject prod1;
    prod1["_id"] = "prod_1";
    prod1["shopId"] = "shop_1";
    prod1["name"] = "Organic Apples 1kg";
    prod1["price"] = 120.0;
    prod1["stock"] = 10;
    prod1["category"] = "groceries";
    m_products["prod_1"] = prod1;

    QJsonObject prod2;
    prod2["_id"] = "prod_2";
    prod2["shopId"] = "shop_1";
    prod2["name"] = "Almond Milk 1L";
    prod2["price"] = 80.0;
    prod2["stock"] = 2; // Low stock
    prod2["category"] = "groceries";
    m_products["prod_2"] = prod2;

    QJsonObject prod3;
    prod3["_id"] = "prod_3";
    prod3["shopId"] = "shop_2";
    prod3["name"] = "First Aid Kit";
    prod3["price"] = 250.0;
    prod3["stock"] = 5;
    prod3["category"] = "pharmacy";
    m_products["prod_3"] = prod3;

    QJsonObject prod4;
    prod4["_id"] = "prod_4";
    prod4["shopId"] = "shop_4";
    prod4["name"] = "Fresh Milk 1L";
    prod4["price"] = 65.0;
    prod4["stock"] = 25;
    prod4["category"] = "groceries";
    m_products["prod_4"] = prod4;

    QJsonObject prod5;
    prod5["_id"] = "prod_5";
    prod5["shopId"] = "shop_5";
    prod5["name"] = "Paracetamol 500mg Strip";
    prod5["price"] = 35.0;
    prod5["stock"] = 50;
    prod5["category"] = "pharmacy";
    m_products["prod_5"] = prod5;
}

void MockApiServer::addOrder(const QJsonObject &order) {
    QString id = order.value("_id").toString(order.value("id").toString());
    m_orders[id] = order;
}

QString MockApiServer::getOrderDeliveryOtp(const QString &orderId) const {
    return m_orderOtps.value(orderId);
}

void MockApiServer::setOrderDeliveryOtp(const QString &orderId, const QString &otp) {
    m_orderOtps[orderId] = otp;
}

void MockApiServer::handleNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        auto readData = [this, socket]() {
            m_buffers[socket].append(socket->readAll());
            const QByteArray &buf = m_buffers[socket];

            int headerEnd = buf.indexOf("\r\n\r\n");
            if (headerEnd == -1)
                return;

            int contentLength = 0;
            QByteArray headers = buf.left(headerEnd);
            int clPos = headers.indexOf("Content-Length: ");
            if (clPos == -1)
                clPos = headers.indexOf("content-length: ");
            if (clPos != -1) {
                int lineEnd = headers.indexOf("\r\n", clPos);
                contentLength = headers.mid(clPos + 16, lineEnd - (clPos + 16)).trimmed().toInt();
            }

            if (buf.size() >= headerEnd + 4 + contentLength) {
                QByteArray fullReq = m_buffers.take(socket);
                processHttpRequest(socket, fullReq);
            }
        };

        connect(socket, &QTcpSocket::readyRead, this, readData);
        if (socket->bytesAvailable() > 0) {
            readData();
        }

        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            m_buffers.remove(socket);
            socket->deleteLater();
        });
    }
}

void MockApiServer::sendResponse(QTcpSocket *socket, int statusCode, const QByteArray &contentType,
                                 const QByteArray &body) {
    QByteArray statusText = (statusCode == 200)   ? "OK"
                            : (statusCode == 201) ? "Created"
                            : (statusCode == 400) ? "Bad Request"
                            : (statusCode == 401) ? "Unauthorized"
                            : (statusCode == 403) ? "Forbidden"
                            : (statusCode == 404) ? "Not Found"
                            : (statusCode == 409) ? "Conflict"
                            : (statusCode == 503) ? "Service Unavailable"
                                                  : "Internal Server Error";

    QByteArray response;
    response += QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(QString::fromLatin1(statusText)).toLatin1();
    response += QString("Content-Type: %1\r\n").arg(QString::fromLatin1(contentType)).toLatin1();
    response += QString("Content-Length: %1\r\n").arg(body.size()).toLatin1();
    response += "Connection: close\r\n";
    response += "Access-Control-Allow-Origin: *\r\n\r\n";
    response += body;

    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}

void MockApiServer::sendJsonResponse(QTcpSocket *socket, int statusCode, const QJsonDocument &doc) {
    sendResponse(socket, statusCode, "application/json", doc.toJson(QJsonDocument::Compact));
}

void MockApiServer::processHttpRequest(QTcpSocket *socket, const QByteArray &rawRequest) {
    QString reqStr = QString::fromUtf8(rawRequest);
    QStringList lines = reqStr.split(QStringLiteral("\r\n"));
    if (lines.isEmpty())
        return;

    QString reqLine = lines.first();
    QStringList tokens = reqLine.split(QLatin1Char(' '));
    if (tokens.size() < 2)
        return;

    QString method = tokens.at(0).toUpper();
    QUrl parsedUrl(tokens.at(1));
    QString path = parsedUrl.path();

    emit requestReceived(method, path);

    // Check artificial transient failures
    if (m_failCount > 0) {
        m_failCount--;
        QJsonObject err;
        err["error"] = "Simulated transient failure";
        sendJsonResponse(socket, m_failStatusCode, QJsonDocument(err));
        return;
    }

    // Extract HTTP Headers
    QMap<QString, QString> reqHeaders;
    for (int i = 1; i < lines.size(); ++i) {
        if (lines[i].trimmed().isEmpty())
            break;
        int colon = lines[i].indexOf(QLatin1Char(':'));
        if (colon > 0) {
            reqHeaders[lines[i].left(colon).trimmed().toLower()] = lines[i].mid(colon + 1).trimmed();
        }
    }

    // ── Enforce 401 Unauthorized and 403 Forbidden for Admin Endpoints ────────────
    if (path.startsWith(QStringLiteral("/api/admin/")) && !path.endsWith(QStringLiteral("/assign"))) {
        QString authHdr = reqHeaders.value(QStringLiteral("authorization"));
        if (authHdr.isEmpty() || authHdr.trimmed() == QStringLiteral("Bearer") ||
            authHdr.contains(QStringLiteral("invalid"), Qt::CaseInsensitive)) {
            QJsonObject err;
            err[QStringLiteral("error")] = QStringLiteral("Unauthorized: Missing or invalid authentication token");
            err[QStringLiteral("message")] = QStringLiteral("Unauthorized: Authentication required (HTTP 401)");
            err[QStringLiteral("statusCode")] = 401;
            sendJsonResponse(socket, 401, QJsonDocument(err));
            return;
        }

        QString roleHdr = reqHeaders.value(QStringLiteral("x-user-role")).toLower();
        bool isNonAdmin = authHdr.contains(QStringLiteral("customer"), Qt::CaseInsensitive) ||
                          authHdr.contains(QStringLiteral("delivery"), Qt::CaseInsensitive) ||
                          authHdr.contains(QStringLiteral("courier"), Qt::CaseInsensitive) ||
                          authHdr.contains(QStringLiteral("shopkeeper"), Qt::CaseInsensitive) ||
                          authHdr.contains(QStringLiteral("merchant"), Qt::CaseInsensitive) ||
                          authHdr.contains(QStringLiteral("non_admin"), Qt::CaseInsensitive) ||
                          roleHdr == QStringLiteral("customer") || roleHdr == QStringLiteral("delivery") ||
                          roleHdr == QStringLiteral("shopkeeper") || roleHdr == QStringLiteral("merchant");

        if (isNonAdmin) {
            QJsonObject err;
            err[QStringLiteral("error")] = QStringLiteral("Forbidden: Administrative privileges required");
            err[QStringLiteral("message")] = QStringLiteral("Forbidden: Insufficient privileges (HTTP 403)");
            err[QStringLiteral("statusCode")] = 403;
            sendJsonResponse(socket, 403, QJsonDocument(err));
            return;
        }
    }

    // Extract JSON body
    int bodyIdx = rawRequest.indexOf("\r\n\r\n");
    QByteArray bodyData = (bodyIdx >= 0) ? rawRequest.mid(bodyIdx + 4) : QByteArray();
    QJsonDocument bodyDoc = QJsonDocument::fromJson(bodyData);
    QJsonObject bodyObj = bodyDoc.isObject() ? bodyDoc.object() : QJsonObject();

    // ── Endpoints ───────────────────────────────────────────────────────────
    if (path == "/api/health") {
        sendJsonResponse(socket, 200, QJsonDocument(QJsonObject{{"status", "ok"}}));
        return;
    }

    // 1. Auth Login
    if (method == "POST" && (path == "/api/auth/login-request" || path == "/api/auth/login")) {
        QString emailOrPhone = bodyObj.value("emailOrPhone").toString(bodyObj.value("email").toString());
        if (m_users.contains(emailOrPhone)) {
            QJsonObject user = m_users.value(emailOrPhone);
            QJsonObject resp;
            resp["accessToken"] = "mock_jwt_access_token_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
            resp["refreshToken"] = "mock_jwt_refresh_token_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
            resp["user"] = user;
            sendJsonResponse(socket, 200, QJsonDocument(resp));
            return;
        }
        QJsonObject err;
        err["message"] = "Invalid credentials";
        sendJsonResponse(socket, 401, QJsonDocument(err));
        return;
    }

    // 2. Auth Signup
    if (method == "POST" && (path == "/api/auth/signup-request" || path == "/api/auth/signup")) {
        QString email = bodyObj.value("email").toString();
        QJsonObject newUser = bodyObj;
        newUser["_id"] = "user_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        newUser["role"] = bodyObj.value("role").toString("customer");
        m_users[email] = newUser;

        QJsonObject resp;
        resp["accessToken"] = "mock_jwt_access_token_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        resp["refreshToken"] = "mock_jwt_refresh_token_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        resp["user"] = newUser;
        sendJsonResponse(socket, 200, QJsonDocument(resp));
        return;
    }

    // 2B. Auth Refresh Token
    if (method == "POST" && path == "/api/auth/refresh-token") {
        QString refToken = bodyObj.value("refreshToken").toString();
        if (!refToken.isEmpty() && !refToken.contains("expired")) {
            QJsonObject resp;
            resp["accessToken"] = "mock_refreshed_access_token_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
            resp["refreshToken"] = refToken;
            sendJsonResponse(socket, 200, QJsonDocument(resp));
            return;
        }
        QJsonObject err;
        err["message"] = "Invalid refresh token";
        sendJsonResponse(socket, 401, QJsonDocument(err));
        return;
    }

    // 3. Courier Compliance
    if (method == "POST" && path == "/api/auth/compliance") {
        QString courierId = bodyObj.value("courierId").toString();
        for (auto it = m_users.begin(); it != m_users.end(); ++it) {
            if (it.value().value("_id").toString() == courierId) {
                QJsonObject u = it.value();
                u["complianceStatus"] = "pending";
                *it = u;
                break;
            }
        }
        QJsonObject resp;
        resp["status"] = "pending";
        sendJsonResponse(socket, 200, QJsonDocument(resp));
        return;
    }

    // 3b. Auth OTP Send
    if (method == "POST" && path == "/api/auth/otp/send") {
        QJsonObject resp;
        resp["success"] = true;
        resp["message"] = "OTP sent successfully";
        sendJsonResponse(socket, 200, QJsonDocument(resp));
        return;
    }

    // 3c. Auth OTP Verify
    if (method == "POST" && path == "/api/auth/otp/verify") {
        QString phone = bodyObj.value("phone").toString();
        QString otp = bodyObj.value("otp").toString();
        if (otp == "1234" || otp == "123456") {
            QJsonObject user;
            user["_id"] = "user_otp_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
            user["phone"] = phone;
            user["role"] = "customer";
            user["name"] = "Phone User";

            QJsonObject resp;
            resp["accessToken"] = "mock_jwt_otp_access_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
            resp["refreshToken"] = "mock_jwt_otp_refresh_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
            resp["user"] = user;
            sendJsonResponse(socket, 200, QJsonDocument(resp));
            return;
        } else {
            QJsonObject err;
            err["message"] = "Invalid verification code";
            sendJsonResponse(socket, 400, QJsonDocument(err));
            return;
        }
    }

    // 4. Shops Listing with 3km Haversine Filter
    if (method == "GET" && path == "/api/shops") {
        QUrlQuery query(parsedUrl.query());
        double userLat = query.queryItemValue("lat").toDouble();
        double userLng = query.queryItemValue("lng").toDouble();
        QString category = query.queryItemValue("category");

        QJsonArray shopArr;
        for (const auto &shop : m_shops) {
            if (!category.isEmpty() && category != "all" && shop.value("category").toString() != category) {
                continue;
            }
            if (userLat != 0.0 || userLng != 0.0) {
                double dist =
                    calculateHaversine(userLat, userLng, shop.value("lat").toDouble(), shop.value("lng").toDouble());
                if (dist > 3.0)
                    continue; // Boundary filter
                QJsonObject s = shop;
                s["distance"] = dist;
                shopArr.append(s);
            } else {
                shopArr.append(shop);
            }
        }
        sendJsonResponse(socket, 200, QJsonDocument(shopArr));
        return;
    }

    // 4B. Shop Details / Products for Shop
    if (method == "GET" && path.startsWith("/api/shops/")) {
        QString sId = path.mid(QStringLiteral("/api/shops/").length());
        QJsonObject resp;
        resp["_id"] = sId;
        QJsonArray itemsArr;
        for (const auto &prod : m_products) {
            if (prod.value("shopId").toString() == sId) {
                itemsArr.append(prod);
            }
        }
        resp["items"] = itemsArr;
        sendJsonResponse(socket, 200, QJsonDocument(resp));
        return;
    }

    // 5. Products Listing
    if (method == "GET" && path == "/api/products") {
        QUrlQuery query(parsedUrl.query());
        QString shopId = query.queryItemValue("shopId");
        QJsonArray prodArr;
        for (const auto &prod : m_products) {
            if (shopId.isEmpty() || prod.value("shopId").toString() == shopId) {
                prodArr.append(prod);
            }
        }
        sendJsonResponse(socket, 200, QJsonDocument(prodArr));
        return;
    }

    // 6. Orders Creation
    if (method == "POST" && path == "/api/orders") {
        QString idempotencyKey = bodyObj.value("idempotency_key").toString();
        // Create order
        QString orderId = "order_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        QJsonObject newOrder = bodyObj;
        newOrder["_id"] = orderId;
        newOrder["status"] = "pending";
        newOrder["created_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        m_orders[orderId] = newOrder;
        sendJsonResponse(socket, 201, QJsonDocument(newOrder));
        return;
    }

    // 7. Orders Listing
    if (method == "GET" && path == "/api/orders") {
        QJsonArray orderArr;
        for (const auto &order : m_orders) {
            orderArr.append(order);
        }
        sendJsonResponse(socket, 200, QJsonDocument(orderArr));
        return;
    }

    // 8. Order Status Update
    if (method == "PATCH" && path.startsWith("/api/orders/")) {
        QString orderId = path.mid(QStringLiteral("/api/orders/").length());
        if (m_orders.contains(orderId)) {
            QJsonObject o = m_orders[orderId];
            o["status"] = bodyObj.value("status").toString();
            m_orders[orderId] = o;
            sendJsonResponse(socket, 200, QJsonDocument(o));
            return;
        }
        sendJsonResponse(socket, 404, QJsonDocument());
        return;
    }

    // 9. Courier Order Assignment with Double Claim Prevention
    if (method == "PATCH" && path.endsWith("/assign")) {
        QStringList parts = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
        QString orderId = (parts.size() >= 2) ? parts.at(parts.size() - 2) : QString();
        QString riderId = bodyObj.value("delivery_boy_id").toString();

        if (m_orderClaims.contains(orderId)) {
            // Already claimed!
            QJsonObject conflictErr;
            conflictErr["error"] = "Order already claimed by another courier";
            sendJsonResponse(socket, 409, QJsonDocument(conflictErr));
            return;
        }

        m_orderClaims[orderId] = riderId;
        if (m_orders.contains(orderId)) {
            QJsonObject o = m_orders[orderId];
            o["delivery_boy_id"] = riderId;
            o["status"] = "assigned";
            m_orders[orderId] = o;
            sendJsonResponse(socket, 200, QJsonDocument(o));
            return;
        }
        sendJsonResponse(socket, 404, QJsonDocument());
        return;
    }

    // 10. Admin Metrics
    if (method == "GET" && path == "/api/admin/metrics") {
        QJsonObject metrics;
        metrics["totalOrders"] = m_orders.size();
        metrics["activeShops"] = m_shops.size();
        metrics["activeUsers"] = m_users.size();
        sendJsonResponse(socket, 200, QJsonDocument(metrics));
        return;
    }

    // 10b. Admin Register Shop
    if (method == "POST" && path == "/api/admin/shops") {
        QString sId = QString("shop_%1").arg(m_shops.size() + 1);
        QJsonObject shop;
        shop["_id"] = sId;
        shop["name"] = bodyObj.value("name").toString();
        shop["category"] = bodyObj.value("category").toString("grocery");
        shop["description"] = QString("QuickCart Partner Store %1").arg(sId);
        shop["lat"] = bodyObj.value("latitude").toDouble(12.9716);
        shop["lng"] = bodyObj.value("longitude").toDouble(77.5946);
        shop["rating"] = 5.0;
        shop["address"] = QString("Platform Express Hub, Bangalore");
        shop["isOpen"] = true;
        m_shops[sId] = shop;

        QJsonObject resp;
        resp["success"] = true;
        resp["shop"] = shop;
        sendJsonResponse(socket, 201, QJsonDocument(resp));
        return;
    }

    // 10c. Admin Provision User
    if (method == "POST" && path == "/api/admin/users") {
        QString email = bodyObj.value("email").toString();
        QJsonObject user;
        user["_id"] = QString("user_%1").arg(m_users.size() + 1);
        user["name"] = bodyObj.value("name").toString();
        user["email"] = email;
        user["phone"] = bodyObj.value("phone").toString();
        user["role"] = bodyObj.value("role").toString("customer");
        user["status"] = "active";
        m_users[email] = user;

        QJsonObject resp;
        resp["success"] = true;
        resp["user"] = user;
        sendJsonResponse(socket, 201, QJsonDocument(resp));
        return;
    }

    // 11. Courier Earnings Dynamic Server Config
    if (method == "GET" && path == "/api/config/delivery") {
        QJsonObject config;
        config["baseFee"] = 40.0;
        config["perKmRate"] = 12.0;
        config["baseFeePaise"] = 4000;
        config["perKmRatePaise"] = 1200;
        config["surgeMultiplier"] = 1.0;
        sendJsonResponse(socket, 200, QJsonDocument(config));
        return;
    }

    // 12. Server-Side Delivery OTP Verification
    if (method == "POST" && path.contains("/verify-delivery-otp")) {
        QStringList parts = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
        QString orderId = (parts.size() >= 3) ? parts.at(2) : QString();
        QString submittedOtp = bodyObj.value("otp").toString().trimmed();
        QString expectedOtp = m_orderOtps.value(orderId);

        if (!expectedOtp.isEmpty() && submittedOtp == expectedOtp) {
            if (m_orders.contains(orderId)) {
                QJsonObject o = m_orders[orderId];
                o["status"] = "delivered";
                m_orders[orderId] = o;
            }
            QJsonObject audit;
            audit["action"] = "order_delivered";
            audit["orderId"] = orderId;
            audit["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
            m_auditLogs.append(audit);

            QJsonObject resp;
            resp["success"] = true;
            resp["status"] = "delivered";
            resp["orderId"] = orderId;
            sendJsonResponse(socket, 200, QJsonDocument(resp));
            return;
        }

        QJsonObject err;
        err["error"] = "Invalid delivery verification code";
        sendJsonResponse(socket, 400, QJsonDocument(err));
        return;
    }

    // 13. Admin Courier Document Approval
    if (method == "POST" && path.startsWith("/api/admin/courier/") && path.endsWith("/approve")) {
        QString courierId = path.section('/', 4, 4);
        for (auto it = m_users.begin(); it != m_users.end(); ++it) {
            if (it.value().value("_id").toString() == courierId || it.key() == courierId) {
                QJsonObject u = it.value();
                u["complianceStatus"] = "approved";
                *it = u;
                break;
            }
        }
        QJsonObject audit;
        audit["action"] = "courier_approved";
        audit["targetId"] = courierId;
        audit["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        m_auditLogs.append(audit);

        QJsonObject resp;
        resp["success"] = true;
        resp["status"] = "approved";
        sendJsonResponse(socket, 200, QJsonDocument(resp));
        return;
    }

    // 14. Admin User/Shop Suspension
    if (method == "POST" && path.startsWith("/api/admin/users/") && path.endsWith("/suspend")) {
        QString userId = path.section('/', 4, 4);
        for (auto it = m_users.begin(); it != m_users.end(); ++it) {
            if (it.value().value("_id").toString() == userId || it.key() == userId) {
                QJsonObject u = it.value();
                u["status"] = "suspended";
                *it = u;
                break;
            }
        }
        QJsonObject audit;
        audit["action"] = "user_suspended";
        audit["targetId"] = userId;
        audit["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        m_auditLogs.append(audit);

        QJsonObject resp;
        resp["success"] = true;
        resp["status"] = "suspended";
        sendJsonResponse(socket, 200, QJsonDocument(resp));
        return;
    }

    // 15. Admin Manual Dispatch
    if (method == "POST" && path.startsWith("/api/admin/orders/") && path.endsWith("/dispatch")) {
        QString orderId = path.section('/', 4, 4);
        QString courierId = bodyObj.value("delivery_boy_id").toString();
        if (m_orders.contains(orderId)) {
            QJsonObject o = m_orders[orderId];
            o["status"] = "assigned";
            o["delivery_boy_id"] = courierId;
            m_orders[orderId] = o;
        }
        QJsonObject audit;
        audit["action"] = "manual_dispatch";
        audit["orderId"] = orderId;
        audit["courierId"] = courierId;
        audit["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        m_auditLogs.append(audit);

        QJsonObject resp;
        resp["success"] = true;
        resp["status"] = "assigned";
        sendJsonResponse(socket, 200, QJsonDocument(resp));
        return;
    }

    // 16. Admin Audit Logs
    if (method == "GET" && path == "/api/admin/audit-logs") {
        sendJsonResponse(socket, 200, QJsonDocument(m_auditLogs));
        return;
    }

    // 17. Server Config (Paise Delivery Fee & Threshold)
    if (method == "GET" && path == "/api/config") {
        QJsonObject conf;
        conf["deliveryFeePaise"] = 4900;
        conf["freeDeliveryThresholdPaise"] = 49900;
        conf["currency"] = "INR";
        conf["currencySymbol"] = "₹";
        sendJsonResponse(socket, 200, QJsonDocument(conf));
        return;
    }

    // 18. Cart Server-Side Calculation (End-to-end Integer Paise)
    if (method == "POST" && path == "/api/cart/calculate") {
        QJsonArray items = bodyObj.value("items").toArray();
        qint64 subtotalPaise = 0;
        for (const auto &itVal : items) {
            QJsonObject it = itVal.toObject();
            int qty = it.value("quantity").toInt(1);
            qint64 pricePaise = static_cast<qint64>(it.value("pricePaise").toVariant().toLongLong());
            subtotalPaise += (qty * pricePaise);
        }
        qint64 deliveryFeePaise = (subtotalPaise == 0 || subtotalPaise >= 49900) ? 0 : 4900;
        qint64 totalPaise = subtotalPaise + deliveryFeePaise;

        QJsonObject resp;
        resp["subtotalPaise"] = subtotalPaise;
        resp["deliveryFeePaise"] = deliveryFeePaise;
        resp["totalPaise"] = totalPaise;
        sendJsonResponse(socket, 200, QJsonDocument(resp));
        return;
    }

    // Default 404
    sendJsonResponse(socket, 404, QJsonDocument());
}
