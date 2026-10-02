/**
 * @file apiclient.h
 * @brief High-level typed API client with DTO mapping, error categorization, and retry semantics.
 * @layer API (Layer 3 - Repositories & ApiClient)
 *
 * Public API Summary:
 * - struct AuthResponseDto, ShopDto, ProductDto, OrderCreateDto, OrderDto.
 * - mapHttpStatusToError(int statusCode, const QString &body): Maps HTTP status codes to AppError.
 * - generateIdempotencyKey(): Creates a UUID-based idempotency token for order placement.
 * - login(...), registerUser(...), fetchShops(...), createOrder(...), updateOrderStatus(...).
 *
 * Dependencies:
 * - core/result.h, api/networkmanager.h, security/securestorage.h
 *
 * Tests:
 * - Covered by tests/cpp/test_apiclient.cpp
 */

#ifndef APICLIENT_H
#define APICLIENT_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QUuid>
#include <functional>
#include "../core/result.h"

// ── DTOs ─────────────────────────────────────────────────────────────────────

struct AuthResponseDto {
    QString accessToken;
    QString refreshToken;
    QString userId;
    QString role;
    QString name;
    QString email;
};

struct ShopDto {
    QString id;
    QString name;
    QString description;
    double lat{0.0};
    double lng{0.0};
    bool isOpen{true};
    double rating{5.0};
};

struct ProductDto {
    QString id;
    QString shopId;
    QString name;
    QString description;
    double price{0.0};
    int stock{0};
    QString category;
};

struct OrderCreateDto {
    QString shopId;
    QString deliveryAddress;
    QVariantList items;
    double subtotal{0.0};
    double deliveryFee{50.0};
    double total{0.0};
    QString idempotencyKey;
};

struct OrderDto {
    QString id;
    QString customerId;
    QString shopId;
    QString courierId;
    QString status;
    double total{0.0};
    QString createdAt;
    QVariantList items;
};

// ── ApiClient Declaration ────────────────────────────────────────────────────

class ApiClient : public QObject
{
    Q_OBJECT

public:
    using AuthCallback = std::function<void(const Result<AuthResponseDto> &result)>;
    using ShopListCallback = std::function<void(const Result<QList<ShopDto>> &result)>;
    using OrderCallback = std::function<void(const Result<OrderDto> &result)>;
    using VoidCallback = std::function<void(const Result<void> &result)>;

    explicit ApiClient(QObject *parent = nullptr);
    static ApiClient* instance();

    /**
     * @brief Map HTTP response status and raw body into a strongly-typed AppError.
     */
    static AppError mapHttpStatusToError(int statusCode, const QString &responseBody);

    /**
     * @brief Generate unique UUID-based idempotency key for mutations.
     */
    static QString generateIdempotencyKey();

    /**
     * @brief Reset singleton state for testing.
     */
    void resetForTesting();

    /**
     * @brief Authenticate user credentials against backend service.
     */
    void login(const QString &email, const QString &password, AuthCallback callback);

    /**
     * @brief Register a new user account.
     */
    void registerUser(const QString &email, const QString &password, const QString &role,
                      const QString &name, AuthCallback callback);

    /**
     * @brief Fetch nearby shops with coordinate filtering.
     */
    void fetchShops(double userLat, double userLng, ShopListCallback callback);

    /**
     * @brief Place an order with mandatory idempotency key to prevent double charging.
     */
    void createOrder(const OrderCreateDto &order, OrderCallback callback);

    /**
     * @brief Update order lifecycle status.
     */
    void updateOrderStatus(const QString &orderId, const QString &newStatus, VoidCallback callback);

private:
    static AuthResponseDto parseAuthResponse(const QJsonObject &json);
    static ShopDto parseShopDto(const QJsonObject &json);
    static OrderDto parseOrderDto(const QJsonObject &json);
};

#endif // APICLIENT_H
