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
    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(lat1 * 3.14159265358979323846 / 180.0) * std::cos(lat2 * 3.14159265358979323846 / 180.0) *
               std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return R * c;
}

MockApiServer::MockApiServer(QObject *parent)
    : QObject(parent), m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &MockApiServer::handleNewConnection);
    resetData();
}

MockApiServer::~MockApiServer()
{
    stop();
}

bool MockApiServer::start()
{
    if (m_server->listen(QHostAddress::LocalHost, 0)) {
        m_port = m_server->serverPort();
        return true;
    }
    return false;
}

void MockApiServer::stop()
{
    if (m_server->isListening()) {
        m_server->close();
    }
}

QString MockApiServer::url() const
{
    return QString(QStringLiteral("http://127.0.0.1:%1")).arg(m_port);
}

quint16 MockApiServer::port() const
{
    return m_port;
}

void MockApiServer::setFailNextRequests(int count, int statusCode)
{
    m_failCount = count;
    m_failStatusCode = statusCode;
}

void MockApiServer::resetData()
{
    m_users.clear();
    m_shops.clear();
    m_products.clear();
    m_orders.clear();
    m_orderClaims.clear();

    // Seed test users
    QJsonObject cust;
    cust["_id"] = "user_cust_1";
    cust["name"] = "Alice Customer";
    cust["email"] = "alice@quickcart.com";
    cust["phone"] = "+919876543210";
    cust["role"] = "customer";
    m_users["alice@quickcart.com"] = cust;
    m_users["+919876543210"] = cust;

    QJsonObject merch;
    merch["_id"] = "user_merch_1";
    merch["name"] = "Bob Merchant";
    merch["email"] = "bob@quickcart.com";
    merch["role"] = "shopkeeper";
    merch["shopId"] = "shop_1";
    m_users["bob@quickcart.com"] = merch;

    QJsonObject courier;
    courier["_id"] = "user_courier_1";
    courier["name"] = "Charlie Courier";
    courier["email"] = "charlie@quickcart.com";
    courier["role"] = "delivery";
    courier["complianceStatus"] = "not_submitted";
    m_users["charlie@quickcart.com"] = courier;

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
}

void MockApiServer::addOrder(const QJsonObject &order)
{
    QString id = order.value("_id").toString(order.value("id").toString());
    m_orders[id] = order;
}

void MockApiServer::handleNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            m_buffers[socket].append(socket->readAll());
            const QByteArray &buf = m_buffers[socket];

            int headerEnd = buf.indexOf("\r\n\r\n");
            if (headerEnd == -1) return;

            int contentLength = 0;
            QByteArray headers = buf.left(headerEnd);
            int clPos = headers.indexOf("Content-Length: ");
            if (clPos == -1) clPos = headers.indexOf("content-length: ");
            if (clPos != -1) {
                int lineEnd = headers.indexOf("\r\n", clPos);
                contentLength = headers.mid(clPos + 16, lineEnd - (clPos + 16)).trimmed().toInt();
            }

            if (buf.size() >= headerEnd + 4 + contentLength) {
                QByteArray fullReq = m_buffers.take(socket);
                processHttpRequest(socket, fullReq);
            }
        });

        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            m_buffers.remove(socket);
            socket->deleteLater();
        });
    }
}

void MockApiServer::sendResponse(QTcpSocket *socket, int statusCode, const QByteArray &contentType, const QByteArray &body)
{
    QByteArray statusText = (statusCode == 200) ? "OK" :
                            (statusCode == 201) ? "Created" :
                            (statusCode == 400) ? "Bad Request" :
                            (statusCode == 401) ? "Unauthorized" :
                            (statusCode == 403) ? "Forbidden" :
                            (statusCode == 404) ? "Not Found" :
                            (statusCode == 409) ? "Conflict" :
                            (statusCode == 503) ? "Service Unavailable" : "Internal Server Error";

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

void MockApiServer::sendJsonResponse(QTcpSocket *socket, int statusCode, const QJsonDocument &doc)
{
    sendResponse(socket, statusCode, "application/json", doc.toJson(QJsonDocument::Compact));
}

void MockApiServer::processHttpRequest(QTcpSocket *socket, const QByteArray &rawRequest)
{
    QString reqStr = QString::fromUtf8(rawRequest);
    QStringList lines = reqStr.split(QStringLiteral("\r\n"));
    if (lines.isEmpty()) return;

    QString reqLine = lines.first();
    QStringList tokens = reqLine.split(QLatin1Char(' '));
    if (tokens.size() < 2) return;

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

    // Extract JSON body
    int bodyIdx = rawRequest.indexOf("\r\n\r\n");
    QByteArray bodyData = (bodyIdx >= 0) ? rawRequest.mid(bodyIdx + 4) : QByteArray();
    QJsonDocument bodyDoc = QJsonDocument::fromJson(bodyData);
    QJsonObject bodyObj = bodyDoc.isObject() ? bodyDoc.object() : QJsonObject();

    // ── Endpoints ───────────────────────────────────────────────────────────

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
                double dist = calculateHaversine(userLat, userLng, shop.value("lat").toDouble(), shop.value("lng").toDouble());
                if (dist > 3.0) continue; // Boundary filter
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

    // 11. Courier Earnings Dynamic Server Config
    if (method == "GET" && path == "/api/config/delivery") {
        QJsonObject config;
        config["baseFee"] = 40.0;
        config["perKmRate"] = 12.0;
        config["surgeMultiplier"] = 1.0;
        sendJsonResponse(socket, 200, QJsonDocument(config));
        return;
    }

    // Default 404
    sendJsonResponse(socket, 404, QJsonDocument());
}
