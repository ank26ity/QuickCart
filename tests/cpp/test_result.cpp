/**
 * @file test_result.cpp
 * @brief Automated unit test suite for Result<T> monadic container and AppError.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include "../../core/result.h"

class TestResult : public QObject
{
    Q_OBJECT

private slots:
    void testSuccessValue()
    {
        Result<int> res = Result<int>::ok(42);
        QVERIFY(res.isSuccess());
        QVERIFY(!res.isError());
        QCOMPARE(res.value(), 42);
        QCOMPARE(res.valueOr(99), 42);
    }

    void testErrorState()
    {
        AppError err = AppError::validation(QStringLiteral("Invalid input"), QStringLiteral("field=email"));
        Result<int> res = Result<int>::error(err);

        QVERIFY(!res.isSuccess());
        QVERIFY(res.isError());
        QCOMPARE(res.error().category, ErrorCategory::Validation);
        QCOMPARE(res.error().statusCode, 400);
        QCOMPARE(res.error().message, QStringLiteral("Invalid input"));
        QCOMPARE(res.error().details, QStringLiteral("field=email"));
        QCOMPARE(res.valueOr(99), 99);
    }

    void testVoidSpecialization()
    {
        Result<void> successRes = Result<void>::ok();
        QVERIFY(successRes.isSuccess());
        QVERIFY(!successRes.isError());

        Result<void> errRes = Result<void>::error(AppError::auth(QStringLiteral("Session expired")));
        QVERIFY(errRes.isError());
        QCOMPARE(errRes.error().category, ErrorCategory::Authentication);
        QCOMPARE(errRes.error().statusCode, 401);
    }

    void testErrorFactories()
    {
        auto errForbidden = AppError::forbidden(QStringLiteral("Access denied"));
        QCOMPARE(errForbidden.category, ErrorCategory::Authorization);
        QCOMPARE(errForbidden.statusCode, 403);

        auto errConflict = AppError::conflict(QStringLiteral("Conflict detected"));
        QCOMPARE(errConflict.category, ErrorCategory::Conflict);
        QCOMPARE(errConflict.statusCode, 409);

        auto errRateLimit = AppError::rateLimited(QStringLiteral("Too many requests"));
        QCOMPARE(errRateLimit.category, ErrorCategory::RateLimited);
        QCOMPARE(errRateLimit.statusCode, 429);

        auto errTimeout = AppError::timeout(QStringLiteral("Gateway timed out"));
        QCOMPARE(errTimeout.category, ErrorCategory::Timeout);
        QCOMPARE(errTimeout.statusCode, 408);
    }
};

QTEST_MAIN(TestResult)
#include "test_result.moc"
