/**
 * @file securestorage.h
 * @brief Hardware-backed credential and token storage interface.
 * @layer Security (Layer 2 - Core Services)
 *
 * Supported Target Platforms:
 * - iOS: Apple Keychain Services (Secure Enclave hardware-backed)
 * - Android: Android Keystore Provider (hardware-backed key encrypting with AES-256-GCM)
 * - Windows: Windows DPAPI (Data Protection API with user-credential hardware bound key)
 * - macOS: Apple Keychain (developer machine and iOS test runner host)
 *
 * Note: Linux is NOT a supported target platform. All file-based key fallbacks
 * have been eliminated in favor of strict OS hardware keystores.
 *
 * Cryptographic Architecture:
 * - Cipher: AES-256-GCM with 96-bit (12-byte) IV and 128-bit (16-byte) tag
 * - Key Source: Random 256-bit symmetric key via CSPRNG (RAND_bytes) held in OS Keystore.
 *               NEVER derived from machine GUID or machine-id.
 *
 * Tests:
 * - Covered by tests/cpp/test_securestorage.cpp
 */

#ifndef SECURESTORAGE_H
#define SECURESTORAGE_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QByteArray>

class SecureStorage : public QObject {
    Q_OBJECT

public:
    enum class Backend {
        PlatformDefault,
        AppleKeychain,
        WindowsDPAPI,
        AndroidKeystore
    };
    Q_ENUM(Backend)

    explicit SecureStorage(QObject *parent = nullptr);

    /**
     * @brief Singleton accessor with lazy auto-instantiation.
     */
    static SecureStorage *instance();

    /**
     * @brief Persist an encrypted secret key-value pair.
     */
    Q_INVOKABLE bool saveSecret(const QString &key, const QString &value);

    /**
     * @brief Retrieve and decrypt a stored secret.
     */
    Q_INVOKABLE QString getSecret(const QString &key) const;

    /**
     * @brief Delete a secret from keystore.
     */
    Q_INVOKABLE bool deleteSecret(const QString &key);

    /**
     * @brief Purge all session credentials and tokens.
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

signals:
    void secretsChanged();

private:
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    bool saveToKeychain(const QString &key, const QByteArray &data);
    QByteArray getFromKeychain(const QString &key) const;
    bool deleteFromKeychain(const QString &key);
#endif

    bool saveToWindowsDPAPI(const QString &key, const QByteArray &data);
    QByteArray getFromWindowsDPAPI(const QString &key) const;
    bool deleteFromWindowsDPAPI(const QString &key);

    bool saveToAndroidKeystore(const QString &key, const QByteArray &data);
    QByteArray getFromAndroidKeystore(const QString &key) const;
    bool deleteFromAndroidKeystore(const QString &key);

    Backend m_activeBackend{Backend::PlatformDefault};
};

#endif // SECURESTORAGE_H
