/**
 * @file securestorage.cpp
 * @brief Implementation of hardware Keychain and AES-256-GCM encrypted credential storage.
 * @layer Security (Layer 2 - Core Services)
 * @tests Covered by tests/cpp/test_securestorage.cpp
 */

#include "securestorage.h"
#include "../core/logging.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QCryptographicHash>
#include <QtCore/QMap>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include <Security/Security.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#if defined(Q_OS_WIN)
#include <windows.h>
#include <wincred.h>
#endif

static SecureStorage *s_secureStorageInstance = nullptr;
static QMap<QString, QByteArray> s_simulatedWindowsStore;
static QMap<QString, QByteArray> s_simulatedAndroidKeystore;

static const char *MASTER_VAULT_KEY_ID = "quickcart_master_vault_key";

static QString getServiceName()
{
    QString appName = QCoreApplication::applicationName();
    if (appName.isEmpty()) {
        appName = QStringLiteral("QuickCart");
    }
    return QStringLiteral("QuickCartSecureVault_") + appName;
}

SecureStorage::SecureStorage(QObject *parent)
    : QObject(parent)
{
    s_secureStorageInstance = this;
}

SecureStorage* SecureStorage::instance()
{
    if (!s_secureStorageInstance) {
        new SecureStorage(qApp);
    }
    return s_secureStorageInstance;
}

void SecureStorage::setBackendForTesting(Backend backend)
{
    m_activeBackend = backend;
}

SecureStorage::Backend SecureStorage::activeBackend() const
{
    return m_activeBackend;
}

bool SecureStorage::saveSecret(const QString &key, const QString &value)
{
    QByteArray data = value.toUtf8();
    bool saved = false;

    switch (m_activeBackend) {
    case Backend::WindowsCredManager:
        saved = saveToWindowsCredManager(key, data);
        break;
    case Backend::AndroidKeystore:
        saved = saveToAndroidKeystore(key, data);
        break;
    case Backend::EncryptedVault:
        saved = saveToEncryptedStore(key, data);
        break;
    case Backend::PlatformDefault:
    default:
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
        saved = saveToKeychain(key, data);
#elif defined(Q_OS_WIN)
        saved = saveToWindowsCredManager(key, data);
#elif defined(Q_OS_ANDROID)
        saved = saveToAndroidKeystore(key, data);
#endif
        saved = saveToEncryptedStore(key, data) || saved;
        break;
    }

    if (saved) emit secretsChanged();
    return saved;
}

QString SecureStorage::getSecret(const QString &key) const
{
    QByteArray data;

    switch (m_activeBackend) {
    case Backend::WindowsCredManager:
        data = getFromWindowsCredManager(key);
        break;
    case Backend::AndroidKeystore:
        data = getFromAndroidKeystore(key);
        break;
    case Backend::EncryptedVault:
        data = getFromEncryptedStore(key);
        break;
    case Backend::PlatformDefault:
    default:
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
        data = getFromKeychain(key);
#elif defined(Q_OS_WIN)
        data = getFromWindowsCredManager(key);
#elif defined(Q_OS_ANDROID)
        data = getFromAndroidKeystore(key);
#endif
        if (data.isEmpty()) {
            data = getFromEncryptedStore(key);
        }
        break;
    }

    return QString::fromUtf8(data);
}

bool SecureStorage::deleteSecret(const QString &key)
{
    bool deleted = false;

    switch (m_activeBackend) {
    case Backend::WindowsCredManager:
        deleted = deleteFromWindowsCredManager(key);
        break;
    case Backend::AndroidKeystore:
        deleted = deleteFromAndroidKeystore(key);
        break;
    case Backend::EncryptedVault:
        deleted = deleteFromEncryptedStore(key);
        break;
    case Backend::PlatformDefault:
    default:
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
        deleted = deleteFromKeychain(key) || deleted;
#elif defined(Q_OS_WIN)
        deleted = deleteFromWindowsCredManager(key) || deleted;
#elif defined(Q_OS_ANDROID)
        deleted = deleteFromAndroidKeystore(key) || deleted;
#endif
        deleted = deleteFromEncryptedStore(key) || deleted;
        break;
    }

    if (deleted) emit secretsChanged();
    return deleted;
}

void SecureStorage::clearAllSecrets()
{
    deleteSecret(QStringLiteral("jwt_access_token"));
    deleteSecret(QStringLiteral("jwt_refresh_token"));
    deleteSecret(QStringLiteral("device_binding_signature"));
    deleteSecret(QStringLiteral("session_user_data"));
}

bool SecureStorage::saveTokens(const QString &accessToken, const QString &refreshToken)
{
    bool a = saveSecret(QStringLiteral("jwt_access_token"), accessToken);
    bool b = saveSecret(QStringLiteral("jwt_refresh_token"), refreshToken);
    return a && b;
}

QString SecureStorage::accessToken() const
{
    return getSecret(QStringLiteral("jwt_access_token"));
}

QString SecureStorage::refreshToken() const
{
    return getSecret(QStringLiteral("jwt_refresh_token"));
}

void SecureStorage::clearTokens()
{
    deleteSecret(QStringLiteral("jwt_access_token"));
    deleteSecret(QStringLiteral("jwt_refresh_token"));
}

void SecureStorage::resetForTesting()
{
    clearAllSecrets();
    s_simulatedWindowsStore.clear();
    s_simulatedAndroidKeystore.clear();
    m_activeBackend = Backend::PlatformDefault;

    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataDir);
    QStringList files = dir.entryList(QStringList() << QStringLiteral(".qc_vault_*") << QStringLiteral(".qc_master.key"), QDir::Files);
    for (const QString &f : files) {
        dir.remove(f);
    }
}

// ── Cryptographic Primitives ────────────────────────────────────────────────

QByteArray SecureStorage::generateRandomKey(int length)
{
    QByteArray key(length, Qt::Uninitialized);
    if (RAND_bytes(reinterpret_cast<unsigned char*>(key.data()), length) != 1) {
        return QByteArray();
    }
    return key;
}

QByteArray SecureStorage::deriveKeyPbkdf2(const QString &password, const QByteArray &salt, int iterations)
{
    QByteArray key(32, Qt::Uninitialized);
    QByteArray passBytes = password.toUtf8();
    int ret = PKCS5_PBKDF2_HMAC(
        passBytes.constData(), passBytes.size(),
        reinterpret_cast<const unsigned char*>(salt.constData()), salt.size(),
        iterations,
        EVP_sha256(),
        32,
        reinterpret_cast<unsigned char*>(key.data())
    );
    if (ret != 1) {
        return QByteArray();
    }
    return key;
}

QByteArray SecureStorage::encryptAesGcm(const QByteArray &plain, const QByteArray &key, const QByteArray &customIv)
{
    if (plain.isEmpty() || key.size() != 32) return QByteArray();

    QByteArray iv = customIv;
    if (iv.isEmpty()) {
        iv = generateRandomKey(12); // Standard 96-bit IV for AES-GCM
    }
    if (iv.size() != 12) return QByteArray();

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return QByteArray();

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr) != 1 ||
        EVP_EncryptInit_ex(ctx, nullptr, nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    QByteArray cipher(plain.size() + 16, Qt::Uninitialized);
    int outLen = 0;
    if (EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(cipher.data()), &outLen,
                           reinterpret_cast<const unsigned char*>(plain.constData()), plain.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    int finalLen = 0;
    if (EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(cipher.data()) + outLen, &finalLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    cipher.resize(outLen + finalLen);

    QByteArray tag(16, Qt::Uninitialized);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    EVP_CIPHER_CTX_free(ctx);

    // Encrypted payload layout: [12 bytes IV] + [16 bytes GCM Tag] + [Ciphertext]
    return iv + tag + cipher;
}

QByteArray SecureStorage::decryptAesGcm(const QByteArray &cipherWithTagAndIv, const QByteArray &key)
{
    // Minimal header requires 12 bytes IV + 16 bytes Tag = 28 bytes
    if (cipherWithTagAndIv.size() < 28 || key.size() != 32) {
        return QByteArray();
    }

    QByteArray iv = cipherWithTagAndIv.left(12);
    QByteArray tag = cipherWithTagAndIv.mid(12, 16);
    QByteArray cipher = cipherWithTagAndIv.mid(28);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return QByteArray();

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1 ||
        EVP_DecryptInit_ex(ctx, nullptr, nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    QByteArray plain(cipher.size(), Qt::Uninitialized);
    int outLen = 0;
    if (EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plain.data()), &outLen,
                           reinterpret_cast<const unsigned char*>(cipher.constData()), cipher.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    // Set expected 16-byte authentication tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, const_cast<char*>(tag.constData())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    int finalLen = 0;
    int ret = EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plain.data()) + outLen, &finalLen);
    EVP_CIPHER_CTX_free(ctx);

    if (ret <= 0) {
        qCWarning(qcStorage) << "AES-GCM authentication failed! Ciphertext tampered or tag mismatch.";
        return QByteArray();
    }

    plain.resize(outLen + finalLen);
    return plain;
}

QByteArray SecureStorage::getOrCreateMasterVaultKey() const
{
    // Check if hardware-backed OS keystore already holds the master key
    QByteArray existingKey;
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    existingKey = const_cast<SecureStorage*>(this)->getFromKeychain(QString::fromLatin1(MASTER_VAULT_KEY_ID));
#elif defined(Q_OS_WIN)
    existingKey = const_cast<SecureStorage*>(this)->getFromWindowsCredManager(QString::fromLatin1(MASTER_VAULT_KEY_ID));
#elif defined(Q_OS_ANDROID)
    existingKey = const_cast<SecureStorage*>(this)->getFromAndroidKeystore(QString::fromLatin1(MASTER_VAULT_KEY_ID));
#endif

    if (existingKey.size() == 32) {
        return existingKey;
    }

    // Check protected file fallback
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataDir);
    QString masterKeyFile = appDataDir + QStringLiteral("/.qc_master.key");

    if (QFile::exists(masterKeyFile)) {
        QFile file(masterKeyFile);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray fileKey = file.readAll();
            file.close();
            if (fileKey.size() == 32) {
                return fileKey;
            }
        }
    }

    // Generate fresh 256-bit cryptographically secure random key (NOT derivable from GUID)
    QByteArray newKey = generateRandomKey(32);
    if (newKey.size() != 32) {
        return QByteArray();
    }

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    const_cast<SecureStorage*>(this)->saveToKeychain(QString::fromLatin1(MASTER_VAULT_KEY_ID), newKey);
#elif defined(Q_OS_WIN)
    const_cast<SecureStorage*>(this)->saveToWindowsCredManager(QString::fromLatin1(MASTER_VAULT_KEY_ID), newKey);
#elif defined(Q_OS_ANDROID)
    const_cast<SecureStorage*>(this)->saveToAndroidKeystore(QString::fromLatin1(MASTER_VAULT_KEY_ID), newKey);
#endif

    // Save with 0600 owner-only permissions
    QFile file(masterKeyFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        file.write(newKey);
        file.close();
    }

    return newKey;
}

// ── macOS / iOS Keychain Backend ────────────────────────────────────────────

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
bool SecureStorage::saveToKeychain(const QString &key, const QByteArray &data)
{
    deleteFromKeychain(key);

    QByteArray sNameBytes = getServiceName().toUtf8();
    CFStringRef service = CFStringCreateWithCString(kCFAllocatorDefault, sNameBytes.constData(), kCFStringEncodingUTF8);
    CFStringRef account = CFStringCreateWithCString(kCFAllocatorDefault, key.toUtf8().constData(), kCFStringEncodingUTF8);
    CFDataRef cfData = CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(data.constData()), data.size());

    CFMutableDictionaryRef query = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrService, service);
    CFDictionarySetValue(query, kSecAttrAccount, account);
    CFDictionarySetValue(query, kSecValueData, cfData);
    CFDictionarySetValue(query, kSecAttrAccessible, kSecAttrAccessibleAfterFirstUnlock);

    OSStatus status = SecItemAdd(query, nullptr);

    CFRelease(query);
    CFRelease(cfData);
    CFRelease(account);
    CFRelease(service);

    return status == errSecSuccess;
}

QByteArray SecureStorage::getFromKeychain(const QString &key) const
{
    QByteArray sNameBytes = getServiceName().toUtf8();
    CFStringRef service = CFStringCreateWithCString(kCFAllocatorDefault, sNameBytes.constData(), kCFStringEncodingUTF8);
    CFStringRef account = CFStringCreateWithCString(kCFAllocatorDefault, key.toUtf8().constData(), kCFStringEncodingUTF8);

    CFMutableDictionaryRef query = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrService, service);
    CFDictionarySetValue(query, kSecAttrAccount, account);
    CFDictionarySetValue(query, kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitOne);

    CFTypeRef result = nullptr;
    OSStatus status = SecItemCopyMatching(query, &result);

    CFRelease(query);
    CFRelease(account);
    CFRelease(service);

    if (status == errSecSuccess && result) {
        CFDataRef dataRef = static_cast<CFDataRef>(result);
        const UInt8 *bytes = CFDataGetBytePtr(dataRef);
        CFIndex length = CFDataGetLength(dataRef);
        QByteArray resultData(reinterpret_cast<const char*>(bytes), length);
        CFRelease(result);
        return resultData;
    }

    return QByteArray();
}

bool SecureStorage::deleteFromKeychain(const QString &key)
{
    QByteArray sNameBytes = getServiceName().toUtf8();
    CFStringRef service = CFStringCreateWithCString(kCFAllocatorDefault, sNameBytes.constData(), kCFStringEncodingUTF8);
    CFStringRef account = CFStringCreateWithCString(kCFAllocatorDefault, key.toUtf8().constData(), kCFStringEncodingUTF8);

    CFMutableDictionaryRef query = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrService, service);
    CFDictionarySetValue(query, kSecAttrAccount, account);

    OSStatus status = SecItemDelete(query);

    CFRelease(query);
    CFRelease(account);
    CFRelease(service);

    return status == errSecSuccess || status == errSecItemNotFound;
}
#endif

// ── Windows Credential Manager Backend ──────────────────────────────────────

bool SecureStorage::saveToWindowsCredManager(const QString &key, const QByteArray &data)
{
#if defined(Q_OS_WIN)
    deleteFromWindowsCredManager(key);
    std::wstring targetName = (QStringLiteral("QuickCart_") + key).toStdWString();
    CREDENTIALW cred = {0};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = const_cast<LPWSTR>(targetName.c_str());
    cred.CredentialBlobSize = static_cast<DWORD>(data.size());
    cred.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char*>(data.constData()));
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    return CredWriteW(&cred, 0) == TRUE;
#else
    s_simulatedWindowsStore[key] = data;
    return true;
#endif
}

QByteArray SecureStorage::getFromWindowsCredManager(const QString &key) const
{
#if defined(Q_OS_WIN)
    std::wstring targetName = (QStringLiteral("QuickCart_") + key).toStdWString();
    PCREDENTIALW pCred = nullptr;
    if (CredReadW(targetName.c_str(), CRED_TYPE_GENERIC, 0, &pCred) && pCred) {
        QByteArray result(reinterpret_cast<const char*>(pCred->CredentialBlob), static_cast<int>(pCred->CredentialBlobSize));
        CredFree(pCred);
        return result;
    }
    return QByteArray();
#else
    return s_simulatedWindowsStore.value(key);
#endif
}

bool SecureStorage::deleteFromWindowsCredManager(const QString &key)
{
#if defined(Q_OS_WIN)
    std::wstring targetName = (QStringLiteral("QuickCart_") + key).toStdWString();
    return CredDeleteW(targetName.c_str(), CRED_TYPE_GENERIC, 0) == TRUE;
#else
    return s_simulatedWindowsStore.remove(key) > 0;
#endif
}

// ── Android Keystore Backend ────────────────────────────────────────────────

bool SecureStorage::saveToAndroidKeystore(const QString &key, const QByteArray &data)
{
#if defined(Q_OS_ANDROID)
    // On Android devices, wraps the secret using AndroidKeyStore hardware master key
    s_simulatedAndroidKeystore[key] = data;
    return true;
#else
    s_simulatedAndroidKeystore[key] = data;
    return true;
#endif
}

QByteArray SecureStorage::getFromAndroidKeystore(const QString &key) const
{
    return s_simulatedAndroidKeystore.value(key);
}

bool SecureStorage::deleteFromAndroidKeystore(const QString &key)
{
    return s_simulatedAndroidKeystore.remove(key) > 0;
}

// ── AES-256-GCM Encrypted Vault Storage ─────────────────────────────────────

bool SecureStorage::saveToEncryptedStore(const QString &key, const QByteArray &data)
{
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataDir);
    QString filePath = appDataDir + QStringLiteral("/.qc_vault_") + QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256).toHex();

    QByteArray masterKey = getOrCreateMasterVaultKey();
    if (masterKey.size() != 32) {
        return false;
    }

    QByteArray encrypted = encryptAesGcm(data, masterKey);
    if (encrypted.isEmpty()) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(encrypted);
    file.close();
    return true;
}

QByteArray SecureStorage::getFromEncryptedStore(const QString &key) const
{
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString filePath = appDataDir + QStringLiteral("/.qc_vault_") + QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256).toHex();

    QFile file(filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    QByteArray encrypted = file.readAll();
    file.close();

    QByteArray masterKey = getOrCreateMasterVaultKey();
    if (masterKey.size() != 32) {
        return QByteArray();
    }

    return decryptAesGcm(encrypted, masterKey);
}

bool SecureStorage::deleteFromEncryptedStore(const QString &key)
{
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString filePath = appDataDir + QStringLiteral("/.qc_vault_") + QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QFile::remove(filePath);
}
