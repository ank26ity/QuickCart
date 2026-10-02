/**
 * @file securestorage.cpp
 * @brief Implementation of hardware Keychain and encrypted fallback credential storage.
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
#include <QtCore/QSysInfo>
#include <QtCore/QDataStream>
#include <QtCore/QMessageAuthenticationCode>

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include <Security/Security.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

static SecureStorage *s_secureStorageInstance = nullptr;

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

bool SecureStorage::saveSecret(const QString &key, const QString &value)
{
    QByteArray data = value.toUtf8();
    bool saved = false;
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    saved = saveToKeychain(key, data);
#endif
    saved = saveToEncryptedStore(key, data) || saved;
    if (saved) emit secretsChanged();
    return saved;
}

QString SecureStorage::getSecret(const QString &key) const
{
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    QByteArray data = getFromKeychain(key);
    if (!data.isEmpty()) {
        return QString::fromUtf8(data);
    }
#endif
    QByteArray fallbackData = getFromEncryptedStore(key);
    return QString::fromUtf8(fallbackData);
}

bool SecureStorage::deleteSecret(const QString &key)
{
    bool deleted = false;
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    deleted = deleteFromKeychain(key) || deleted;
#endif
    deleted = deleteFromEncryptedStore(key) || deleted;
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
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataDir);
    QStringList files = dir.entryList(QStringList() << QStringLiteral(".qc_vault_*"), QDir::Files);
    for (const QString &f : files) {
        dir.remove(f);
    }
}

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
    if (status == errSecDuplicateItem) {
        CFMutableDictionaryRef updateQuery = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue(updateQuery, kSecClass, kSecClassGenericPassword);
        CFDictionarySetValue(updateQuery, kSecAttrService, service);
        CFDictionarySetValue(updateQuery, kSecAttrAccount, account);

        CFMutableDictionaryRef updateAttrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue(updateAttrs, kSecValueData, cfData);

        status = SecItemUpdate(updateQuery, updateAttrs);

        CFRelease(updateQuery);
        CFRelease(updateAttrs);
    }

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

QByteArray SecureStorage::deriveDeviceKey() const
{
    QString machineId = QSysInfo::machineUniqueId();
    if (machineId.isEmpty()) {
        machineId = QSysInfo::machineHostName() + QStringLiteral("_qc_vault_salt_9823");
    }
    QByteArray salt = "QuickCart_Secure_Vault_PBKDF2_Salt_#2026_Salt$";
    QByteArray derived = machineId.toUtf8();
    for (int i = 0; i < 2000; ++i) {
        derived = QCryptographicHash::hash(derived + salt, QCryptographicHash::Sha256);
    }
    return derived;
}

QByteArray SecureStorage::encryptData(const QByteArray &plain, const QByteArray &key) const
{
    if (plain.isEmpty()) return QByteArray();
    QByteArray cipher;
    cipher.resize(plain.size());
    int keyLen = key.size();
    for (int i = 0; i < plain.size(); ++i) {
        quint8 streamByte = static_cast<quint8>(key.at(i % keyLen)) ^ static_cast<quint8>((i * 37) & 0xFF);
        cipher[i] = plain.at(i) ^ streamByte;
    }
    QByteArray mac = QMessageAuthenticationCode::hash(cipher, key, QCryptographicHash::Sha256);
    return mac + cipher;
}

QByteArray SecureStorage::decryptData(const QByteArray &cipherWithMac, const QByteArray &key) const
{
    if (cipherWithMac.size() < 32) return QByteArray();
    QByteArray expectedMac = cipherWithMac.left(32);
    QByteArray cipher = cipherWithMac.mid(32);

    QByteArray calculatedMac = QMessageAuthenticationCode::hash(cipher, key, QCryptographicHash::Sha256);
    if (expectedMac != calculatedMac) {
        qCWarning(qcStorage) << "Encrypted storage HMAC integrity check failed! Data may have been tampered.";
        return QByteArray();
    }

    QByteArray plain;
    plain.resize(cipher.size());
    int keyLen = key.size();
    for (int i = 0; i < cipher.size(); ++i) {
        quint8 streamByte = static_cast<quint8>(key.at(i % keyLen)) ^ static_cast<quint8>((i * 37) & 0xFF);
        plain[i] = cipher.at(i) ^ streamByte;
    }
    return plain;
}

bool SecureStorage::saveToEncryptedStore(const QString &key, const QByteArray &data)
{
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataDir);
    QString filePath = appDataDir + QStringLiteral("/.qc_vault_") + QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex();

    QByteArray devKey = deriveDeviceKey();
    QByteArray encrypted = encryptData(data, devKey);

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
    QString filePath = appDataDir + QStringLiteral("/.qc_vault_") + QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex();

    QFile file(filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    QByteArray encrypted = file.readAll();
    file.close();

    QByteArray devKey = deriveDeviceKey();
    return decryptData(encrypted, devKey);
}

bool SecureStorage::deleteFromEncryptedStore(const QString &key)
{
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString filePath = appDataDir + QStringLiteral("/.qc_vault_") + QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex();
    return QFile::remove(filePath);
}
