/**
 * @file test_auth_flow.cpp
 * @brief Integration tests for Authentication flow (4a): login, signup, session restore, logout, courier compliance.
 * @layer Tests / Integration (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include "../../models/authservice.h"
#include "../../api/networkmanager.h"
#include "../../security/securestorage.h"
#include "../../security/permissionmanager.h"
#include "../tools/mockapiserver.h"

class TestAuthFlow : public QObject
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
        AuthService::instance()->resetForTesting();
        PermissionManager::instance()->resetForTesting();
        m_server.resetData();
    }

    void testInvalidInputErrors()
    {
        AuthService *auth = AuthService::instance();

        // Invalid email
        auth->login("invalid_email", "password123");
        QVERIFY(!auth->isLoggedIn());
        QVERIFY(!auth->errorMessage().isEmpty());

        // Empty password
        auth->login("alice@quickcart.com", "");
        QVERIFY(!auth->isLoggedIn());
        QCOMPARE(auth->errorMessage(), QStringLiteral("Password cannot be empty."));

        // Weak password on signup
        QVariantMap user;
        user["email"] = "newuser@quickcart.com";
        user["password"] = "short";
        user["name"] = "New User";
        auth->signup(user);
        QVERIFY(!auth->isLoggedIn());
        QVERIFY(!auth->errorMessage().isEmpty());
    }

    void testSuccessfulLoginAndSessionRestore()
    {
        AuthService *auth = AuthService::instance();
        QSignalSpy loginSpy(auth, &AuthService::loginSuccess);
        QSignalSpy authSpy(auth, &AuthService::authStateChanged);

        auth->login("alice@quickcart.com", "Password123");

        QVERIFY(loginSpy.wait(3000));
        QVERIFY(auth->isLoggedIn());
        QCOMPARE(auth->userRole(), QStringLiteral("customer"));
        QCOMPARE(auth->userName(), QStringLiteral("Alice Customer"));
        QCOMPARE(PermissionManager::instance()->currentRole(), QStringLiteral("customer"));

        // Verify SecureStorage saved tokens and session data
        QVERIFY(!SecureStorage::instance()->accessToken().isEmpty());
        QVERIFY(!SecureStorage::instance()->getSecret("session_user_data").isEmpty());

        // Simulate App Restart: clear in-memory state and check session
        auth->resetForTesting();
        QVERIFY(!auth->isLoggedIn());

        // Re-inject session into storage to simulate relaunch
        QJsonObject savedUser;
        savedUser["_id"] = "user_cust_1";
        savedUser["name"] = "Alice Customer";
        savedUser["role"] = "customer";
        savedUser["email"] = "alice@quickcart.com";
        SecureStorage::instance()->saveSecret("session_user_data", QString::fromUtf8(QJsonDocument(savedUser).toJson(QJsonDocument::Compact)));

        auth->checkSession();
        QVERIFY(auth->isLoggedIn());
        QCOMPARE(auth->userName(), QStringLiteral("Alice Customer"));
        QCOMPARE(auth->userRole(), QStringLiteral("customer"));
    }

    void testLogoutClearsSessionAndRBAC()
    {
        AuthService *auth = AuthService::instance();
        QSignalSpy loginSpy(auth, &AuthService::loginSuccess);

        auth->login("alice@quickcart.com", "Password123");
        QVERIFY(loginSpy.wait(3000));
        QVERIFY(auth->isLoggedIn());

        auth->logout();
        QVERIFY(!auth->isLoggedIn());
        QCOMPARE(PermissionManager::instance()->currentRole(), QStringLiteral("guest"));
        QVERIFY(SecureStorage::instance()->accessToken().isEmpty());
        QVERIFY(SecureStorage::instance()->getSecret("session_user_data").isEmpty());
    }

    void testCourierComplianceSubmission()
    {
        AuthService *auth = AuthService::instance();
        QSignalSpy loginSpy(auth, &AuthService::loginSuccess);

        auth->login("charlie@quickcart.com", "Password123");
        QVERIFY(loginSpy.wait(3000));
        QCOMPARE(auth->userRole(), QStringLiteral("delivery"));
        QCOMPARE(auth->complianceStatus(), QStringLiteral("not_submitted"));

        QSignalSpy compSpy(auth, &AuthService::complianceSubmitted);
        auth->submitCourierCompliance("DL-KA-01-2024-001234", "KA-01-EQ-9988");

        QVERIFY(compSpy.wait(3000));
        QCOMPARE(auth->complianceStatus(), QStringLiteral("pending"));
    }

    void testOtpRequestAndVerify()
    {
        AuthService *auth = AuthService::instance();

        // Invalid phone format
        auth->requestOtp("123");
        QVERIFY(!auth->isOtpSent());
        QVERIFY(!auth->errorMessage().isEmpty());

        // Valid phone format
        auth->requestOtp("+919876543210");
        QTRY_VERIFY_WITH_TIMEOUT(auth->isOtpSent(), 4000);

        // Invalid OTP length
        auth->verifyOtp("+919876543210", "12");
        QVERIFY(!auth->isLoggedIn());

        // Valid OTP verification
        auth->verifyOtp("+919876543210", "1234");
        QTRY_VERIFY_WITH_TIMEOUT(auth->isLoggedIn(), 4000);
    }
};

QTEST_MAIN(TestAuthFlow)
#include "test_auth_flow.moc"
