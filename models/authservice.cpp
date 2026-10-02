/**
 * @file authservice.cpp
 * @brief Implementation of AuthService.
 * @layer ViewModels / Services (Layer 2 - Core Domain Services)
 * @tests Covered by tests/cpp/test_auth_flow.cpp
 */

#include "authservice.h"
#include "../api/networkmanager.h"
#include "../security/securestorage.h"
#include "../security/permissionmanager.h"
#include "../core/validators.h"
#include "../core/logging.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QSysInfo>
#include <QtCore/QCryptographicHash>

static AuthService *s_authServiceInstance = nullptr;

AuthService::AuthService(QObject *parent) : QObject(parent) {
    s_authServiceInstance = this;

    if (NetworkManager::instance()) {
        connect(NetworkManager::instance(), &NetworkManager::tokenRefreshRequired, this, [this]() {
            qCWarning(qcAuth) << "Token refresh failed in NetworkManager. Logging out session.";
            logout();
            emit sessionExpired();
        });
    }

    checkSession();
}

AuthService *AuthService::instance() {
    if (!s_authServiceInstance) {
        new AuthService(qApp);
    }
    return s_authServiceInstance;
}

bool AuthService::isLoggedIn() const {
    return m_isLoggedIn;
}

QString AuthService::userId() const {
    return m_userObj.value(QStringLiteral("_id")).toString(m_userObj.value(QStringLiteral("id")).toString());
}

QString AuthService::userName() const {
    return m_userObj.value(QStringLiteral("name")).toString();
}

QString AuthService::userEmail() const {
    return m_userObj.value(QStringLiteral("email")).toString();
}

QString AuthService::userPhone() const {
    return m_userObj.value(QStringLiteral("phone")).toString();
}

QString AuthService::userRole() const {
    return m_userObj.value(QStringLiteral("role")).toString(QStringLiteral("customer")).toLower();
}

QString AuthService::userShopId() const {
    return m_userObj.value(QStringLiteral("shopId")).toString();
}

QString AuthService::complianceStatus() const {
    return m_complianceStatus;
}

QVariantMap AuthService::currentUserData() const {
    return m_userObj.toVariantMap();
}

bool AuthService::isLoading() const {
    return m_isLoading;
}

QString AuthService::errorMessage() const {
    return m_errorMessage;
}

bool AuthService::isOtpSent() const {
    return m_isOtpSent;
}

QString AuthService::getDeviceFingerprint() const {
    QString raw = QSysInfo::machineUniqueId() + QStringLiteral("_") + QSysInfo::prettyProductName();
    return QCryptographicHash::hash(raw.toUtf8(), QCryptographicHash::Sha256).toHex();
}

void AuthService::login(const QString &emailOrPhone, const QString &password) {
    QString trimmedInput = emailOrPhone.trimmed();

    // Client-side pre-flight validation
    if (trimmedInput.contains(QLatin1Char('@'))) {
        auto emailRes = Validators::validateEmail(trimmedInput);
        if (emailRes.isError()) {
            m_errorMessage = emailRes.error().message;
            emit errorMessageChanged();
            return;
        }
    } else {
        auto phoneRes = Validators::validatePhone(trimmedInput);
        if (phoneRes.isError()) {
            m_errorMessage = phoneRes.error().message;
            emit errorMessageChanged();
            return;
        }
    }

    if (password.trimmed().isEmpty()) {
        m_errorMessage = QStringLiteral("Password cannot be empty.");
        emit errorMessageChanged();
        return;
    }

    m_isLoading = true;
    m_errorMessage.clear();
    emit loadingChanged();
    emit errorMessageChanged();

    QJsonObject body;
    body[QStringLiteral("emailOrPhone")] = trimmedInput;
    body[QStringLiteral("password")] = password;
    body[QStringLiteral("deviceId")] = getDeviceFingerprint();

    qCDebug(qcAuth) << "Initiating login request for user:" << StructuredLogger::maskEmail(trimmedInput);

    NetworkManager::instance()->post(
        QStringLiteral("/api/auth/login-request"), body,
        [this](bool success, const QJsonDocument &doc, const QString &err) {
            m_isLoading = false;
            emit loadingChanged();

            if (!success) {
                m_errorMessage = err.isEmpty() ? QStringLiteral("Invalid login credentials") : err;
                qCWarning(qcAuth) << "Login failed:" << m_errorMessage;
                emit errorMessageChanged();
                return;
            }

            if (doc.isObject()) {
                QJsonObject rootObj = doc.object();
                QJsonObject userObj = rootObj.contains(QStringLiteral("user"))
                                          ? rootObj.value(QStringLiteral("user")).toObject()
                                          : rootObj;
                QString access = rootObj.value(QStringLiteral("accessToken"))
                                     .toString(rootObj.value(QStringLiteral("token")).toString());
                QString refresh = rootObj.value(QStringLiteral("refreshToken")).toString();

                setUser(userObj, access, refresh);
                qCDebug(qcAuth) << "User login successful. Role:" << userRole();
                emit loginSuccess();
            }
        });
}

void AuthService::signup(const QVariantMap &userData) {
    QString email = userData.value(QStringLiteral("email")).toString().trimmed();
    QString phone = userData.value(QStringLiteral("phone")).toString().trimmed();
    QString password = userData.value(QStringLiteral("password")).toString();

    // Client-side validations
    auto emailRes = Validators::validateEmail(email);
    if (emailRes.isError()) {
        m_errorMessage = emailRes.error().message;
        emit errorMessageChanged();
        return;
    }

    if (!phone.isEmpty()) {
        auto phoneRes = Validators::validatePhone(phone);
        if (phoneRes.isError()) {
            m_errorMessage = phoneRes.error().message;
            emit errorMessageChanged();
            return;
        }
    }

    auto passRes = Validators::validatePassword(password);
    if (passRes.isError()) {
        m_errorMessage = passRes.error().message;
        emit errorMessageChanged();
        return;
    }

    m_isLoading = true;
    m_errorMessage.clear();
    emit loadingChanged();
    emit errorMessageChanged();

    QJsonObject body = QJsonObject::fromVariantMap(userData);
    body[QStringLiteral("deviceId")] = getDeviceFingerprint();

    NetworkManager::instance()->post(
        QStringLiteral("/api/auth/signup-request"), body,
        [this](bool success, const QJsonDocument &doc, const QString &err) {
            m_isLoading = false;
            emit loadingChanged();

            if (!success) {
                m_errorMessage = err.isEmpty() ? QStringLiteral("Registration failed") : err;
                qCWarning(qcAuth) << "Signup failed:" << m_errorMessage;
                emit errorMessageChanged();
                return;
            }

            if (doc.isObject()) {
                QJsonObject rootObj = doc.object();
                QJsonObject userObj = rootObj.contains(QStringLiteral("user"))
                                          ? rootObj.value(QStringLiteral("user")).toObject()
                                          : rootObj;
                QString access = rootObj.value(QStringLiteral("accessToken"))
                                     .toString(rootObj.value(QStringLiteral("token")).toString());
                QString refresh = rootObj.value(QStringLiteral("refreshToken")).toString();

                setUser(userObj, access, refresh);
                emit signupSuccess();
            }
        });
}

void AuthService::submitCourierCompliance(const QString &licenseNumber, const QString &vehicleNumber) {
    if (licenseNumber.trimmed().isEmpty() || vehicleNumber.trimmed().isEmpty()) {
        m_errorMessage = QStringLiteral("Driver license and vehicle registration numbers are required.");
        emit errorMessageChanged();
        return;
    }

    m_isLoading = true;
    m_errorMessage.clear();
    emit loadingChanged();
    emit errorMessageChanged();

    QJsonObject body;
    body[QStringLiteral("licenseNumber")] = licenseNumber.trimmed();
    body[QStringLiteral("vehicleNumber")] = vehicleNumber.trimmed();
    body[QStringLiteral("courierId")] = userId();

    NetworkManager::instance()->post(
        QStringLiteral("/api/auth/compliance"), body,
        [this](bool success, const QJsonDocument &doc, const QString &err) {
            Q_UNUSED(doc);
            m_isLoading = false;
            emit loadingChanged();

            if (!success) {
                m_errorMessage = err.isEmpty() ? QStringLiteral("Compliance document upload failed.") : err;
                emit errorMessageChanged();
                return;
            }

            m_complianceStatus = QStringLiteral("pending");
            m_userObj[QStringLiteral("complianceStatus")] = m_complianceStatus;
            emit complianceStatusChanged();
            emit complianceSubmitted();
        });
}

void AuthService::requestOtp(const QString &phone) {
    auto phoneRes = Validators::validatePhone(phone);
    if (phoneRes.isError()) {
        m_errorMessage = phoneRes.error().message;
        emit errorMessageChanged();
        return;
    }

    m_isLoading = true;
    m_errorMessage.clear();
    emit loadingChanged();
    emit errorMessageChanged();

    QJsonObject body;
    body[QStringLiteral("phone")] = phone.trimmed();
    body[QStringLiteral("deviceId")] = getDeviceFingerprint();

    NetworkManager::instance()->post(
        QStringLiteral("/api/auth/otp/send"), body, [this](bool success, const QJsonDocument &doc, const QString &err) {
            Q_UNUSED(doc);
            m_isLoading = false;
            emit loadingChanged();

            if (!success) {
                m_errorMessage = err.isEmpty() ? QStringLiteral("Failed to send verification OTP") : err;
                emit errorMessageChanged();
                return;
            }

            m_isOtpSent = true;
            emit otpStateChanged();
        });
}

void AuthService::verifyOtp(const QString &phone, const QString &otp) {
    auto otpRes = Validators::validateOtp(otp, 6);
    if (otpRes.isError() && Validators::validateOtp(otp, 4).isError()) {
        m_errorMessage = QStringLiteral("OTP must be 4 or 6 digits.");
        emit errorMessageChanged();
        return;
    }

    m_isLoading = true;
    m_errorMessage.clear();
    emit loadingChanged();
    emit errorMessageChanged();

    QJsonObject body;
    body[QStringLiteral("phone")] = phone.trimmed();
    body[QStringLiteral("otp")] = otp.trimmed();
    body[QStringLiteral("deviceId")] = getDeviceFingerprint();

    NetworkManager::instance()->post(
        QStringLiteral("/api/auth/otp/verify"), body,
        [this](bool success, const QJsonDocument &doc, const QString &err) {
            m_isLoading = false;
            emit loadingChanged();

            if (!success) {
                m_errorMessage = err.isEmpty() ? QStringLiteral("Invalid verification code") : err;
                emit errorMessageChanged();
                return;
            }

            if (doc.isObject()) {
                QJsonObject rootObj = doc.object();
                QJsonObject userObj = rootObj.contains(QStringLiteral("user"))
                                          ? rootObj.value(QStringLiteral("user")).toObject()
                                          : rootObj;
                QString access = rootObj.value(QStringLiteral("accessToken")).toString();
                QString refresh = rootObj.value(QStringLiteral("refreshToken")).toString();

                setUser(userObj, access, refresh);
                emit loginSuccess();
            }
        });
}

void AuthService::logout() {
    if (SecureStorage::instance()) {
        SecureStorage::instance()->clearAllSecrets();
    }
    m_isLoggedIn = false;
    m_userObj = QJsonObject();
    m_complianceStatus = QStringLiteral("not_submitted");
    m_isOtpSent = false;
    m_errorMessage.clear();

    syncWithRBAC();

    emit authStateChanged();
    emit userProfileChanged();
    emit complianceStatusChanged();
    emit otpStateChanged();
    emit errorMessageChanged();
}

void AuthService::checkSession() {
    if (!SecureStorage::instance())
        return;

    QString sessionJson = SecureStorage::instance()->getSecret(QStringLiteral("session_user_data"));
    if (!sessionJson.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(sessionJson.toUtf8());
        if (doc.isObject()) {
            m_userObj = doc.object();
            m_isLoggedIn = true;
            m_complianceStatus =
                m_userObj.value(QStringLiteral("complianceStatus")).toString(QStringLiteral("not_submitted"));
            syncWithRBAC();
            emit authStateChanged();
            emit userProfileChanged();
            emit complianceStatusChanged();
        }
    }
}

void AuthService::setUser(const QJsonObject &userObj, const QString &accessToken, const QString &refreshToken) {
    m_userObj = userObj;
    m_isLoggedIn = true;
    m_complianceStatus = m_userObj.value(QStringLiteral("complianceStatus")).toString(QStringLiteral("not_submitted"));

    if (SecureStorage::instance()) {
        if (!accessToken.isEmpty()) {
            SecureStorage::instance()->saveTokens(accessToken, refreshToken);
        }
        QJsonDocument doc(m_userObj);
        SecureStorage::instance()->saveSecret(QStringLiteral("session_user_data"),
                                              QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    }

    syncWithRBAC();

    emit authStateChanged();
    emit userProfileChanged();
    emit complianceStatusChanged();
}

void AuthService::syncWithRBAC() {
    if (PermissionManager::instance()) {
        if (m_isLoggedIn) {
            PermissionManager::instance()->setCurrentRole(userRole());
            PermissionManager::instance()->setCurrentUserId(userId());
            PermissionManager::instance()->setCurrentShopId(userShopId());
        } else {
            PermissionManager::instance()->setCurrentRole(QStringLiteral("guest"));
            PermissionManager::instance()->setCurrentUserId(QString());
            PermissionManager::instance()->setCurrentShopId(QString());
        }
    }
}

void AuthService::resetForTesting() {
    logout();
}
