/**
 * @file test_apiclient.cpp
 * @brief Automated unit test suite for ApiClient DTO mapping, error mapping, and idempotency key generation.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include "../../api/apiclient.h"

class TestApiClient : public QObject
{
    Q_OBJECT

private slots:
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
};

QTEST_MAIN(TestApiClient)
#include "test_apiclient.moc"
