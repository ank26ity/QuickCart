/**
 * @file securestorage.h
 * @brief Hardware-backed and AES-256-GCM encrypted credential and token storage interface.
 * @layer Security (Layer 2 - Core Services)
 *
 * Cryptographic Architecture:
 * - Cipher: AES-256
 * - Mode: GCM (Galois/Counter Mode) authenticated encryption with 96-bit (12-byte) IV and 128-bit (16-byte) tag
 * - Key Source: Random 256-bit symmetric master key generated via CSPRNG and held in OS Keystore
 *               (Apple Keychain, Windows Credential Manager, Android Keystore). Never derived from machine GUID.
 * - Password Key Derivation: PBKDF2-HMAC-SHA256 with 100,000 iterations and unique 128-bit salt.
 * - Backends: macOS/iOS Keychain, Windows Credential Manager, Android Keystore, and Hardware-Encrypted Vault.
 *
 * Tests:
 * - Covered by tests/cpp/test_securestorage.cpp
 */

#ifndef SECURESTORAGE_H
#define SECURESTORAGE_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QByteArray>

class SecureStorage : public QObject
{
    Q_OBJECT

public:
    enum class Backend {
        PlatformDefault,
        WindowsCredManager,
        AndroidKeystore,
        EncryptedVault
    };
    Q_ENUM(Backend)

    explicit SecureStorage(QObject *parent = nullptr);

    /**
     * @brief Singleton accessor with lazy auto-instantiation.
     */
    static SecureStorage* instance();

    /**
     * @brief Persist an encrypted secret key-value pair.
     */
    Q_INVOKABLE bool saveSecret(const QString &key, const QString &value);

    /**
     * @brief Retrieve and decrypt a stored secret.
     */
    Q_INVOKABLE QString getSecret(const QString &key) const;

    /**
     * @brief Delete a secret from keystore and encrypted fallback store.
     */
    Q_INVOKABLE bool deleteSecret(const QString &key);

    /**
     * @brief Purge all session credentials, tokens, and encryption identifiers.
     */
    Q_INVOKABLE void clearAllSecrets();

    /**
     * @brief Save JWT access and refresh token pair.
     */
    bool saveTokens(const QString &accessToken, const QString &refreshToken);

    /**
     * @brief Retrieve the stored JWT access token.
     */
    QString accessToken() const;

    /**
     * @brief Retrieve the stored JWT refresh token.
     */
    QString refreshToken() const;

    /**
     * @brief Delete active access and refresh tokens.
     */
    void clearTokens();

    /**
     * @brief Reset storage state for unit test isolation.
     */
    void resetForTesting();

    /**
     * @brief Select active backend for deterministic testing across platforms.
     */
    void setBackendForTesting(Backend backend);
    Backend activeBackend() const;

    // Cryptographic Primitives (AES-256-GCM, PBKDF2-SHA256, CSPRNG)
    static QByteArray encryptAesGcm(const QByteArray &plain, const QByteArray &key, const QByteArray &iv = QByteArray());
    static QByteArray decryptAesGcm(const QByteArray &cipherWithTagAndIv, const QByteArray &key);
    static QByteArray deriveKeyPbkdf2(const QString &password, const QByteArray &salt, int iterations = 100000);
    static QByteArray generateRandomKey(int length = 32);

signals:
    void secretsChanged();

private:
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    bool saveToKeychain(const QString &key, const QByteArray &data);
    QByteArray getFromKeychain(const QString &key) const;
    bool deleteFromKeychain(const QString &key);
#endif

    bool saveToWindowsCredManager(const QString &key, const QByteArray &data);
    QByteArray getFromWindowsCredManager(const QString &key) const;
    bool deleteFromWindowsCredManager(const QString &key);

    bool saveToAndroidKeystore(const QString &key, const QByteArray &data);
    QByteArray getFromAndroidKeystore(const QString &key) const;
    bool deleteFromAndroidKeystore(const QString &key);

    bool saveToEncryptedStore(const QString &key, const QByteArray &data);
    QByteArray getFromEncryptedStore(const QString &key) const;
    bool deleteFromEncryptedStore(const QString &key);

    QByteArray getOrCreateMasterVaultKey() const;

    Backend m_activeBackend{Backend::PlatformDefault};
};

#endif // SECURESTORAGE_H
