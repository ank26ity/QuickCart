/**
 * @file test_securestorage.cpp
 * @brief Automated unit test suite for SecureStorage encryption, token management, and persistence.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include "../../security/securestorage.h"

class TestSecureStorage : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        SecureStorage::instance()->resetForTesting();
    }

    void cleanup()
    {
        SecureStorage::instance()->resetForTesting();
    }

    void testSaveAndRetrieveSecret()
    {
        SecureStorage *store = SecureStorage::instance();
        QString testKey = QStringLiteral("unit_test_api_key");
        QString testValue = QStringLiteral("secret_jwt_payload_xyz_987");

        QVERIFY(store->saveSecret(testKey, testValue));
        QCOMPARE(store->getSecret(testKey), testValue);

        QVERIFY(store->deleteSecret(testKey));
        QVERIFY(store->getSecret(testKey).isEmpty());
    }

    void testTokenManagement()
    {
        SecureStorage *store = SecureStorage::instance();
        QString access = QStringLiteral("ey...access.token.jwt");
        QString refresh = QStringLiteral("ey...refresh.token.jwt");

        QVERIFY(store->saveTokens(access, refresh));
        QCOMPARE(store->accessToken(), access);
        QCOMPARE(store->refreshToken(), refresh);

        store->clearTokens();
        QVERIFY(store->accessToken().isEmpty());
        QVERIFY(store->refreshToken().isEmpty());
    }

    void testNonexistentSecretReturnsEmpty()
    {
        SecureStorage *store = SecureStorage::instance();
        QVERIFY(store->getSecret(QStringLiteral("non_existent_key_999")).isEmpty());
    }

    void testClearAllSecrets()
    {
        SecureStorage *store = SecureStorage::instance();
        store->saveTokens("token_a", "token_r");
        store->clearAllSecrets();

        QVERIFY(store->accessToken().isEmpty());
        QVERIFY(store->refreshToken().isEmpty());
    }
};

QTEST_MAIN(TestSecureStorage)
#include "test_securestorage.moc"
