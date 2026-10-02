/**
 * @file securestorage.h
 * @brief Hardware-backed and encrypted credential and token storage interface.
 * @layer Security (Layer 2 - Core Services)
 *
 * Public API Summary:
 * - saveSecret(key, value): Persist encrypted credential (macOS Keychain or AES/HMAC vault).
 * - getSecret(key): Decrypt and retrieve stored credential.
 * - deleteSecret(key): Remove credential by key.
 * - clearAllSecrets(): Wipe all active session tokens and device keys.
 * - saveTokens(accessToken, refreshToken): Store JWT credentials atomically.
 * - accessToken(), refreshToken(): Retrieve current JWT session tokens.
 * - clearTokens(): Remove active authentication tokens.
 * - resetForTesting(): Wipe store for test isolation.
 *
 * Dependencies:
 * - <QtCore/QObject>, <QtCore/QString>, <QtCore/QByteArray>
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
    explicit SecureStorage(QObject *parent = nullptr);

    /**
     * @brief Singleton accessor with lazy auto-instantiation.
     */
    static SecureStorage* instance();

    /**
     * @brief Persist an encrypted secret key-value pair.
     * Uses platform Keychain when available, with encrypted fallback.
     */
    Q_INVOKABLE bool saveSecret(const QString &key, const QString &value);

    /**
     * @brief Retrieve and decrypt a stored secret.
     */
    Q_INVOKABLE QString getSecret(const QString &key) const;

    /**
     * @brief Delete a secret from Keychain and encrypted fallback store.
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

signals:
    void secretsChanged();

private:
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    bool saveToKeychain(const QString &key, const QByteArray &data);
    QByteArray getFromKeychain(const QString &key) const;
    bool deleteFromKeychain(const QString &key);
#endif

    bool saveToEncryptedStore(const QString &key, const QByteArray &data);
    QByteArray getFromEncryptedStore(const QString &key) const;
    bool deleteFromEncryptedStore(const QString &key);

    QByteArray deriveDeviceKey() const;
    QByteArray encryptData(const QByteArray &plain, const QByteArray &key) const;
    QByteArray decryptData(const QByteArray &cipher, const QByteArray &key) const;
};

#endif // SECURESTORAGE_H
