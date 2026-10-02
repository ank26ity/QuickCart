/**
 * @file test_securestorage.cpp
 * @brief Automated unit test suite for SecureStorage AES-256-GCM, PBKDF2, and multi-backend storage.
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

    void testAesGcmEncryptionDecryptionRoundtrip()
    {
        QByteArray key = SecureStorage::generateRandomKey(32);
        QCOMPARE(key.size(), 32);

        QByteArray plaintext = "Confidential customer session payload: user=customer_42,role=customer";
        QByteArray ciphertext = SecureStorage::encryptAesGcm(plaintext, key);

        QVERIFY(!ciphertext.isEmpty());
        // Minimum size: 12 (IV) + 16 (Tag) + plaintext.size()
        QVERIFY(ciphertext.size() >= 28 + plaintext.size());

        QByteArray decrypted = SecureStorage::decryptAesGcm(ciphertext, key);
        QCOMPARE(decrypted, plaintext);

        // Edge case validations
        QVERIFY(SecureStorage::encryptAesGcm("", key).isEmpty());
        QVERIFY(SecureStorage::encryptAesGcm("test", "short_key").isEmpty());
        QVERIFY(SecureStorage::decryptAesGcm("short", key).isEmpty());
        QVERIFY(SecureStorage::decryptAesGcm(ciphertext, "short_key").isEmpty());
    }

    void testAesGcmTamperDetection()
    {
        QByteArray key = SecureStorage::generateRandomKey(32);
        QByteArray plaintext = "QuickCart critical bank/UPI payment token";
        QByteArray ciphertext = SecureStorage::encryptAesGcm(plaintext, key);

        QVERIFY(ciphertext.size() >= 28);

        // Scenario 1: Flip a bit in the ciphertext body
        QByteArray tamperedCipher = ciphertext;
        tamperedCipher[tamperedCipher.size() - 1] ^= 0x01;
        QByteArray tamperedDecrypted = SecureStorage::decryptAesGcm(tamperedCipher, key);
        QVERIFY2(tamperedDecrypted.isEmpty(), "Tampered ciphertext payload must fail GCM authentication");

        // Scenario 2: Flip a bit in the authentication tag (bytes 12..27)
        QByteArray tamperedTag = ciphertext;
        tamperedTag[14] ^= 0xFF;
        QByteArray tagTamperDecrypted = SecureStorage::decryptAesGcm(tamperedTag, key);
        QVERIFY2(tagTamperDecrypted.isEmpty(), "Tampered GCM tag must fail authentication");

        // Scenario 3: Decrypt with a different 256-bit key
        QByteArray wrongKey = SecureStorage::generateRandomKey(32);
        QByteArray wrongKeyDecrypted = SecureStorage::decryptAesGcm(ciphertext, wrongKey);
        QVERIFY2(wrongKeyDecrypted.isEmpty(), "Decryption with incorrect key must fail authentication");
    }

    void testAesGcmNonceUniqueness()
    {
        QByteArray key = SecureStorage::generateRandomKey(32);
        QByteArray plaintext = "Identical repeated token";

        QByteArray cipher1 = SecureStorage::encryptAesGcm(plaintext, key);
        QByteArray cipher2 = SecureStorage::encryptAesGcm(plaintext, key);

        // Every encryption must generate a fresh 96-bit CSPRNG IV
        QVERIFY(!cipher1.isEmpty());
        QVERIFY(!cipher2.isEmpty());
        QVERIFY(cipher1 != cipher2);
        QVERIFY(cipher1.left(12) != cipher2.left(12));

        // Both decrypt to identical original plaintext
        QCOMPARE(SecureStorage::decryptAesGcm(cipher1, key), plaintext);
        QCOMPARE(SecureStorage::decryptAesGcm(cipher2, key), plaintext);
    }

    void testPbkdf2KeyDerivationWith100kIterations()
    {
        QString password = QStringLiteral("P@ssw0rd!SuperSecretQuickCart2026");
        QByteArray salt = "QuickCart_Salt_128bit_Entropy_#49";

        QByteArray key1 = SecureStorage::deriveKeyPbkdf2(password, salt, 100000);
        QCOMPARE(key1.size(), 32);

        // Deterministic: same password + salt yields identical 256-bit key
        QByteArray key2 = SecureStorage::deriveKeyPbkdf2(password, salt, 100000);
        QCOMPARE(key1, key2);

        // Different salt yields completely distinct key
        QByteArray altSalt = "QuickCart_Salt_Alternative_Entropy_99";
        QByteArray key3 = SecureStorage::deriveKeyPbkdf2(password, altSalt, 100000);
        QVERIFY(key1 != key3);
    }

    void testWindowsCredentialManagerBackend()
    {
        SecureStorage *store = SecureStorage::instance();
        store->setBackendForTesting(SecureStorage::Backend::WindowsCredManager);
        QCOMPARE(store->activeBackend(), SecureStorage::Backend::WindowsCredManager);

        QString testKey = QStringLiteral("win_session_token");
        QString testVal = QStringLiteral("win_jwt_secret_value_12345");

        QVERIFY(store->saveSecret(testKey, testVal));
        QCOMPARE(store->getSecret(testKey), testVal);

        QVERIFY(store->deleteSecret(testKey));
        QVERIFY(store->getSecret(testKey).isEmpty());
    }

    void testAndroidKeystoreBackend()
    {
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

    void testEncryptedVaultBackend()
    {
        SecureStorage *store = SecureStorage::instance();
        store->setBackendForTesting(SecureStorage::Backend::EncryptedVault);
        QCOMPARE(store->activeBackend(), SecureStorage::Backend::EncryptedVault);

        QString testKey = QStringLiteral("vault_api_key");
        QString testVal = QStringLiteral("vault_secret_key_value_999");

        QVERIFY(store->saveSecret(testKey, testVal));
        QCOMPARE(store->getSecret(testKey), testVal);

        QVERIFY(store->deleteSecret(testKey));
        QVERIFY(store->getSecret(testKey).isEmpty());
    }

    void testSaveAndRetrieveSecretDefault()
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

    void testMasterKeyFileFallback()
    {
        SecureStorage *store = SecureStorage::instance();
        store->resetForTesting();

        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QString masterKeyFile = appDataDir + QStringLiteral("/.qc_master.key");

        // 1. Pre-seed file with 32-byte key
        QFile file(masterKeyFile);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QByteArray seededKey = SecureStorage::generateRandomKey(32);
        file.write(seededKey);
        file.close();

        store->setBackendForTesting(SecureStorage::Backend::EncryptedVault);
        QVERIFY(store->saveSecret(QStringLiteral("k1"), QStringLiteral("v1")));
        QCOMPARE(store->getSecret(QStringLiteral("k1")), QStringLiteral("v1"));

        // 2. Remove file and reset - triggers fresh generation and write to file
        store->resetForTesting();
        QVERIFY(store->saveSecret(QStringLiteral("k2"), QStringLiteral("v2")));
        QCOMPARE(store->getSecret(QStringLiteral("k2")), QStringLiteral("v2"));
        QVERIFY(QFile::exists(masterKeyFile));

        store->setBackendForTesting(SecureStorage::Backend::PlatformDefault);
    }
};

QTEST_MAIN(TestSecureStorage)
#include "test_securestorage.moc"
