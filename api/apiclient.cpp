/**
 * @file apiclient.cpp
 * @brief Implementation of high-level typed ApiClient.
 * @layer API (Layer 3 - Repositories & ApiClient)
 * @tests Covered by tests/cpp/test_apiclient.cpp
 */

#include "apiclient.h"
#include "networkmanager.h"
#include "../security/securestorage.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>

static ApiClient *s_apiClientInstance = nullptr;

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
{
    s_apiClientInstance = this;
}

ApiClient* ApiClient::instance()
{
    if (!s_apiClientInstance) {
        new ApiClient(qApp);
    }
    return s_apiClientInstance;
}

void ApiClient::resetForTesting()
{
    // Resets state if needed
}

QString ApiClient::generateIdempotencyKey()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

AppError ApiClient::mapHttpStatusToError(int statusCode, const QString &responseBody)
{
    QString message = responseBody;
    // Attempt parsing JSON error message if present
    QJsonDocument doc = QJsonDocument::fromJson(responseBody.toUtf8());
    if (doc.isObject() && doc.object().contains(QStringLiteral("error"))) {
        message = doc.object().value(QStringLiteral("error")).toString();
    } else if (doc.isObject() && doc.object().contains(QStringLiteral("message"))) {
        message = doc.object().value(QStringLiteral("message")).toString();
    }

    if (message.trimmed().isEmpty()) {
        message = QString(QStringLiteral("HTTP Error %1")).arg(statusCode);
    }

    switch (statusCode) {
    case 400:
        return AppError::validation(message);
    case 401:
        return AppError::auth(message, QStringLiteral("ERR_AUTH_UNAUTHORIZED"));
    case 403:
        return AppError::forbidden(message);
    case 404:
        return AppError{ErrorCategory::NotFound, 404, message, QString(), QStringLiteral("ERR_NOT_FOUND")};
    case 408:
        return AppError::timeout(message);
    case 409:
        return AppError::conflict(message, QStringLiteral("ERR_CONFLICT"));
    case 429:
        return AppError::rateLimited(message);
    default:
        if (statusCode >= 500 && statusCode < 600) {
            return AppError::server(message, statusCode);
        }
        return AppError::network(message, statusCode);
    }
}

AuthResponseDto ApiClient::parseAuthResponse(const QJsonObject &json)
{
    AuthResponseDto dto;
    dto.accessToken = json.value(QStringLiteral("access_token")).toString(
        json.value(QStringLiteral("accessToken")).toString(
        json.value(QStringLiteral("token")).toString()));
    dto.refreshToken = json.value(QStringLiteral("refresh_token")).toString(
        json.value(QStringLiteral("refreshToken")).toString());
    QJsonObject user = json.value(QStringLiteral("user")).toObject();
    if (!user.isEmpty()) {
        dto.userId = user.value(QStringLiteral("id")).toString(user.value(QStringLiteral("_id")).toString());
        dto.role = user.value(QStringLiteral("role")).toString();
        dto.name = user.value(QStringLiteral("name")).toString();
        dto.email = user.value(QStringLiteral("email")).toString();
    }
    return dto;
}

ShopDto ApiClient::parseShopDto(const QJsonObject &json)
{
    ShopDto dto;
    dto.id = json.value(QStringLiteral("_id")).toString(json.value(QStringLiteral("id")).toString());
    dto.name = json.value(QStringLiteral("name")).toString();
    dto.description = json.value(QStringLiteral("description")).toString();
    dto.lat = json.value(QStringLiteral("lat")).toDouble(0.0);
    dto.lng = json.value(QStringLiteral("lng")).toDouble(0.0);
    dto.isOpen = json.value(QStringLiteral("is_open")).toBool(true);
    dto.rating = json.value(QStringLiteral("rating")).toDouble(5.0);
    return dto;
}

OrderDto ApiClient::parseOrderDto(const QJsonObject &json)
{
    OrderDto dto;
    dto.id = json.value(QStringLiteral("_id")).toString(json.value(QStringLiteral("id")).toString());
    dto.customerId = json.value(QStringLiteral("customer_id")).toString();
    dto.shopId = json.value(QStringLiteral("shop_id")).toString();
    dto.courierId = json.value(QStringLiteral("delivery_boy_id")).toString();
    dto.status = json.value(QStringLiteral("status")).toString(QStringLiteral("pending"));
    dto.total = json.value(QStringLiteral("total")).toDouble(0.0);
    dto.createdAt = json.value(QStringLiteral("created_at")).toString();
    dto.items = json.value(QStringLiteral("items")).toArray().toVariantList();
    return dto;
}

void ApiClient::login(const QString &email, const QString &password, AuthCallback callback)
{
    QJsonObject payload;
    payload[QStringLiteral("email")] = email;
    payload[QStringLiteral("password")] = password;

    NetworkManager::instance()->executePost(QStringLiteral("/api/auth/login"), payload, [callback](const Result<QJsonDocument> &res) {
        if (res.isError()) {
            callback(Result<AuthResponseDto>::error(res.error()));
            return;
        }
        QJsonObject root = res.value().object();
        AuthResponseDto dto = parseAuthResponse(root);
        SecureStorage::instance()->saveTokens(dto.accessToken, dto.refreshToken);
        callback(Result<AuthResponseDto>::ok(dto));
    });
}

void ApiClient::registerUser(const QString &email, const QString &password, const QString &role,
                             const QString &name, AuthCallback callback)
{
    QJsonObject payload;
    payload[QStringLiteral("email")] = email;
    payload[QStringLiteral("password")] = password;
    payload[QStringLiteral("role")] = role;
    payload[QStringLiteral("name")] = name;

    NetworkManager::instance()->executePost(QStringLiteral("/api/auth/signup"), payload, [callback](const Result<QJsonDocument> &res) {
        if (res.isError()) {
            callback(Result<AuthResponseDto>::error(res.error()));
            return;
        }
        QJsonObject root = res.value().object();
        AuthResponseDto dto = parseAuthResponse(root);
        SecureStorage::instance()->saveTokens(dto.accessToken, dto.refreshToken);
        callback(Result<AuthResponseDto>::ok(dto));
    });
}

void ApiClient::fetchShops(double userLat, double userLng, ShopListCallback callback)
{
    QString endpoint = QStringLiteral("/api/shops");
    if (userLat != 0.0 || userLng != 0.0) {
        endpoint += QString(QStringLiteral("?lat=%1&lng=%2&radius=3000")).arg(userLat).arg(userLng);
    }

    NetworkManager::instance()->executeGet(endpoint, [callback](const Result<QJsonDocument> &res) {
        if (res.isError()) {
            callback(Result<QList<ShopDto>>::error(res.error()));
            return;
        }
        QList<ShopDto> list;
        QJsonArray arr = res.value().array();
        for (const QJsonValue &v : arr) {
            if (v.isObject()) {
                list.append(parseShopDto(v.toObject()));
            }
        }
        callback(Result<QList<ShopDto>>::ok(list));
    });
}

void ApiClient::createOrder(const OrderCreateDto &order, OrderCallback callback)
{
    QJsonObject payload;
    payload[QStringLiteral("shop_id")] = order.shopId;
    payload[QStringLiteral("address")] = order.deliveryAddress;
    payload[QStringLiteral("items")] = QJsonArray::fromVariantList(order.items);
    payload[QStringLiteral("subtotal")] = order.subtotal;
    payload[QStringLiteral("delivery_fee")] = order.deliveryFee;
    payload[QStringLiteral("total")] = order.total;
    payload[QStringLiteral("idempotency_key")] = order.idempotencyKey.isEmpty() ? generateIdempotencyKey() : order.idempotencyKey;

    NetworkManager::instance()->executePost(QStringLiteral("/api/orders"), payload, [callback](const Result<QJsonDocument> &res) {
        if (res.isError()) {
            callback(Result<OrderDto>::error(res.error()));
            return;
        }
        callback(Result<OrderDto>::ok(parseOrderDto(res.value().object())));
    });
}

void ApiClient::updateOrderStatus(const QString &orderId, const QString &newStatus, VoidCallback callback)
{
    QJsonObject payload;
    payload[QStringLiteral("status")] = newStatus;

    NetworkManager::instance()->executePatch(QString(QStringLiteral("/api/orders/%1")).arg(orderId), payload, [callback](const Result<QJsonDocument> &res) {
        if (res.isError()) {
            callback(Result<void>::error(res.error()));
            return;
        }
        callback(Result<void>::ok());
    });
}
