/**
 * @file test_apiclient.cpp
 * @brief Automated unit and integration test suite for ApiClient DTO mapping, error mapping, and network calls.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include "../../api/apiclient.h"
#include "../../api/networkmanager.h"
#include "../tools/mockapiserver.h"

class TestApiClient : public QObject
{
    Q_OBJECT

private:
    MockApiServer m_server;

private slots:
    void initTestCase()
    {
        QVERIFY(m_server.start());
        NetworkManager::instance()->setBaseUrl(m_server.url());
    }

    void cleanupTestCase()
    {
        m_server.stop();
    }

    void init()
    {
        m_server.resetData();
    }

    void testIdempotencyKeyGeneration()
    {
        QString key1 = ApiClient::generateIdempotencyKey();
        QString key2 = ApiClient::generateIdempotencyKey();

        QVERIFY(!key1.isEmpty());
        QVERIFY(!key2.isEmpty());
        QVERIFY(key1 != key2);
        QVERIFY(key1.length() >= 32);
    }

    void testHttpStatusMapping()
    {
        // 400 Validation
        auto err400 = ApiClient::mapHttpStatusToError(400, "{\"error\": \"Invalid quantity\"}");
        QCOMPARE(err400.category, ErrorCategory::Validation);
        QCOMPARE(err400.message, QStringLiteral("Invalid quantity"));

        // 401 Auth
        auto err401 = ApiClient::mapHttpStatusToError(401, "{\"message\": \"Token expired\"}");
        QCOMPARE(err401.category, ErrorCategory::Authentication);
        QCOMPARE(err401.message, QStringLiteral("Token expired"));

        // 403 Forbidden
        auto err403 = ApiClient::mapHttpStatusToError(403, "Forbidden resource");
        QCOMPARE(err403.category, ErrorCategory::Authorization);
        QCOMPARE(err403.message, QStringLiteral("Forbidden resource"));

        // 404 Not Found
        auto err404 = ApiClient::mapHttpStatusToError(404, "{\"error\": \"Order not found\"}");
        QCOMPARE(err404.category, ErrorCategory::NotFound);

        // 408 Timeout
        auto err408 = ApiClient::mapHttpStatusToError(408, "Gateway timeout");
        QCOMPARE(err408.category, ErrorCategory::Timeout);

        // 409 Conflict
        auto err409 = ApiClient::mapHttpStatusToError(409, "{\"error\": \"Item already assigned\"}");
        QCOMPARE(err409.category, ErrorCategory::Conflict);

        // 429 Rate Limit
        auto err429 = ApiClient::mapHttpStatusToError(429, "Rate limit exceeded");
        QCOMPARE(err429.category, ErrorCategory::RateLimited);

        // 500 Server Error
        auto err500 = ApiClient::mapHttpStatusToError(500, "Internal Server Error");
        QCOMPARE(err500.category, ErrorCategory::Server);
        QCOMPARE(err500.statusCode, 500);

        // 503 Service Unavailable
        auto err503 = ApiClient::mapHttpStatusToError(503, "Service Unavailable");
        QCOMPARE(err503.category, ErrorCategory::Server);
        QCOMPARE(err503.statusCode, 503);
    }

    void testRegisterAndLoginApi()
    {
        ApiClient client;
        bool registerDone = false;
        bool registerSuccess = false;

        client.registerUser("client_user@quickcart.com", "Password123!", "customer", "Client User",
            [&](const Result<AuthResponseDto> &res) {
                registerDone = true;
                registerSuccess = res.isSuccess();
                if (res.isSuccess()) {
                    QCOMPARE(res.value().email, QStringLiteral("client_user@quickcart.com"));
                    QCOMPARE(res.value().role, QStringLiteral("customer"));
                }
            });

        QTRY_VERIFY_WITH_TIMEOUT(registerDone, 30000);
        QVERIFY(registerSuccess);

        // Test Login
        bool loginDone = false;
        bool loginSuccess = false;
        client.login("client_user@quickcart.com", "Password123!",
            [&](const Result<AuthResponseDto> &res) {
                loginDone = true;
                loginSuccess = res.isSuccess();
                if (res.isSuccess()) {
                    QVERIFY(!res.value().accessToken.isEmpty());
                }
            });

        QTRY_VERIFY_WITH_TIMEOUT(loginDone, 30000);
        QVERIFY(loginSuccess);
    }

    void testFetchShopsApi()
    {
        ApiClient client;
        bool done = false;
        bool success = false;

        client.fetchShops(28.6139, 77.2090, [&](const Result<QList<ShopDto>> &res) {
            done = true;
            success = res.isSuccess();
            if (res.isSuccess()) {
                QVERIFY(res.value().size() >= 0);
            }
        });

        QTRY_VERIFY_WITH_TIMEOUT(done, 5000);
        QVERIFY(success);
    }

    void testCreateOrderAndUpdateStatusApi()
    {
        ApiClient client;

        OrderCreateDto createDto;
        createDto.shopId = QStringLiteral("shop_1");
        createDto.deliveryAddress = QStringLiteral("123 Test St");
        createDto.subtotal = 300.0;
        createDto.deliveryFee = 50.0;
        createDto.total = 350.0;
        createDto.idempotencyKey = ApiClient::generateIdempotencyKey();

        bool orderDone = false;
        bool orderSuccess = false;
        QString createdOrderId;

        client.createOrder(createDto, [&](const Result<OrderDto> &res) {
            orderDone = true;
            orderSuccess = res.isSuccess();
            if (res.isSuccess()) {
                createdOrderId = res.value().id;
                QVERIFY(!createdOrderId.isEmpty());
                QCOMPARE(res.value().status, QStringLiteral("pending"));
            }
        });

        QTRY_VERIFY_WITH_TIMEOUT(orderDone, 5000);
        QVERIFY(orderSuccess);

        // Update status
        bool updateDone = false;
        bool updateSuccess = false;
        client.updateOrderStatus(createdOrderId, QStringLiteral("preparing"), [&](const Result<void> &res) {
            updateDone = true;
            updateSuccess = res.isSuccess();
        });

        QTRY_VERIFY_WITH_TIMEOUT(updateDone, 5000);
        QVERIFY(updateSuccess);
    }
};

QTEST_MAIN(TestApiClient)
#include "test_apiclient.moc"
