/**
 * @file test_securestorage.cpp
 * @brief Automated unit test suite for SecureStorage AES-256-GCM and multi-backend storage.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include "../../security/securestorage.h"

class TestSecureStorage : public QObject {
    Q_OBJECT

private slots:
    void init() { SecureStorage::instance()->resetForTesting(); }

    void cleanup() { SecureStorage::instance()->resetForTesting(); }

    void testBackendEnumCoverage() {
        SecureStorage *store = SecureStorage::instance();
        store->setBackendForTesting(SecureStorage::Backend::AppleKeychain);
        QCOMPARE(store->activeBackend(), SecureStorage::Backend::AppleKeychain);
        store->setBackendForTesting(SecureStorage::Backend::WindowsDPAPI);
        QCOMPARE(store->activeBackend(), SecureStorage::Backend::WindowsDPAPI);
        store->setBackendForTesting(SecureStorage::Backend::AndroidKeystore);
        QCOMPARE(store->activeBackend(), SecureStorage::Backend::AndroidKeystore);
        store->setBackendForTesting(SecureStorage::Backend::PlatformDefault);
        QCOMPARE(store->activeBackend(), SecureStorage::Backend::PlatformDefault);
    }

    void testWindowsDPAPIBackend() {
        SecureStorage *store = SecureStorage::instance();
        store->setBackendForTesting(SecureStorage::Backend::WindowsDPAPI);
        QCOMPARE(store->activeBackend(), SecureStorage::Backend::WindowsDPAPI);

        QString testKey = QStringLiteral("win_session_token");
        QString testVal = QStringLiteral("win_jwt_secret_value_12345");

        QVERIFY(store->saveSecret(testKey, testVal));
        QCOMPARE(store->getSecret(testKey), testVal);

#if defined(Q_OS_WIN)
        // Verify real Windows DPAPI created encrypted file on disk
        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QString dpapiFile = appDataDir + QStringLiteral("/dpapi_") +
            QCryptographicHash::hash(testKey.toUtf8(), QCryptographicHash::Sha256).toHex() + QStringLiteral(".dat");
        QVERIFY2(QFile::exists(dpapiFile), "Real DPAPI encrypted file must exist on disk");
        QFile file(dpapiFile);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QByteArray cipherBytes = file.readAll();
        file.close();
        // Ciphertext on disk must NOT equal plaintext
        QVERIFY(cipherBytes != testVal.toUtf8());
#endif

        QVERIFY(store->deleteSecret(testKey));
        QVERIFY(store->getSecret(testKey).isEmpty());

#if defined(Q_OS_WIN)
        QVERIFY2(!QFile::exists(dpapiFile), "DPAPI encrypted file must be purged after deleteSecret");
#endif
    }

    void testAndroidKeystoreBackend() {
        SecureStorage *store = SecureStorage::instance();
        store->setBackendForTesting(SecureStorage::Backend::AndroidKeystore);
        QCOMPARE(store->activeBackend(), SecureStorage::Backend::AndroidKeystore);

        QString testKey = QStringLiteral("android_auth_token");
        QString testVal = QStringLiteral("android_keystore_secret_token_987");

        QVERIFY(store->saveSecret(testKey, testVal));
        QCOMPARE(store->getSecret(testKey), testVal);

        QVERIFY(store->deleteSecret(testKey));
        QVERIFY(store->getSecret(testKey).isEmpty());
    }

    void testAppleKeychainBackend() {
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
        SecureStorage *store = SecureStorage::instance();
        store->setBackendForTesting(SecureStorage::Backend::AppleKeychain);
        QCOMPARE(store->activeBackend(), SecureStorage::Backend::AppleKeychain);

        QString testKey = QStringLiteral("apple_keychain_token");
        QString testVal = QStringLiteral("apple_keychain_secret_999");

        QVERIFY(store->saveSecret(testKey, testVal));
        QCOMPARE(store->getSecret(testKey), testVal);

        QVERIFY(store->deleteSecret(testKey));
        QVERIFY(store->getSecret(testKey).isEmpty());
#endif
    }

    void testSaveAndRetrieveSecretDefault() {
        SecureStorage *store = SecureStorage::instance();
        QString testKey = QStringLiteral("unit_test_api_key");
        QString testValue = QStringLiteral("secret_jwt_payload_xyz_987");

        QVERIFY(store->saveSecret(testKey, testValue));
        QCOMPARE(store->getSecret(testKey), testValue);

        QVERIFY(store->deleteSecret(testKey));
        QVERIFY(store->getSecret(testKey).isEmpty());
    }

    void testTokenManagement() {
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

    void testNonexistentSecretReturnsEmpty() {
        SecureStorage *store = SecureStorage::instance();
        QVERIFY(store->getSecret(QStringLiteral("non_existent_key_999")).isEmpty());
    }

    void testClearAllSecrets() {
        SecureStorage *store = SecureStorage::instance();
        store->saveTokens("token_a", "token_r");
        store->clearAllSecrets();

        QVERIFY(store->accessToken().isEmpty());
        QVERIFY(store->refreshToken().isEmpty());
    }

    void testMultiPlatformBackends() {
        SecureStorage *store = SecureStorage::instance();

        // Windows DPAPI backend
        store->setBackendForTesting(SecureStorage::Backend::WindowsDPAPI);
        QVERIFY(store->saveSecret(QStringLiteral("win_key"), QStringLiteral("win_val")));
        QCOMPARE(store->getSecret(QStringLiteral("win_key")), QStringLiteral("win_val"));
        QVERIFY(store->deleteSecret(QStringLiteral("win_key")));

        // Android Keystore backend
        store->setBackendForTesting(SecureStorage::Backend::AndroidKeystore);
        QVERIFY(store->saveSecret(QStringLiteral("droid_key"), QStringLiteral("droid_val")));
        QCOMPARE(store->getSecret(QStringLiteral("droid_key")), QStringLiteral("droid_val"));
        QVERIFY(store->deleteSecret(QStringLiteral("droid_key")));

        store->setBackendForTesting(SecureStorage::Backend::PlatformDefault);
    }
};

QTEST_MAIN(TestSecureStorage)
#include "test_securestorage.moc"
