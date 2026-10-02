/**
 * @file authservice.h
 * @brief Authentication, session lifecycle, and courier compliance management.
 * @layer ViewModels / Services (Layer 2 - Core Domain Services)
 *
 * Public API Summary:
 * - isLoggedIn, userId, userName, userEmail, userPhone, userRole, userShopId
 * - complianceStatus: Current status ("not_submitted", "pending", "approved", "rejected")
 * - login(emailOrPhone, password): Authenticate with credential validation
 * - signup(userData): Register new role with schema validation
 * - submitCourierCompliance(licenseNumber, vehicleNumber): Document verification flow
 * - logout(): Clear hardware keychain secrets and reset RBAC
 * - checkSession(): Restore persisted JWT session on application launch
 * - resetForTesting(): Wipe state for isolated testing
 *
 * Dependencies:
 * - core/validators.h, security/securestorage.h, security/permissionmanager.h, api/networkmanager.h
 *
 * Tests:
 * - Covered by tests/cpp/test_auth_flow.cpp
 */

#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QJsonObject>
#include <QtCore/QVariantMap>

class AuthService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY authStateChanged)
    Q_PROPERTY(QString userId READ userId NOTIFY userProfileChanged)
    Q_PROPERTY(QString userName READ userName NOTIFY userProfileChanged)
    Q_PROPERTY(QString userEmail READ userEmail NOTIFY userProfileChanged)
    Q_PROPERTY(QString userPhone READ userPhone NOTIFY userProfileChanged)
    Q_PROPERTY(QString userRole READ userRole NOTIFY userProfileChanged)
    Q_PROPERTY(QString userShopId READ userShopId NOTIFY userProfileChanged)
    Q_PROPERTY(QString complianceStatus READ complianceStatus NOTIFY complianceStatusChanged)
    Q_PROPERTY(QVariantMap currentUserData READ currentUserData NOTIFY userProfileChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(bool isOtpSent READ isOtpSent NOTIFY otpStateChanged)

public:
    explicit AuthService(QObject *parent = nullptr);
    static AuthService *instance();

    bool isLoggedIn() const;
    QString userId() const;
    QString userName() const;
    QString userEmail() const;
    QString userPhone() const;
    QString userRole() const;
    QString userShopId() const;
    QString complianceStatus() const;
    QVariantMap currentUserData() const;
    bool isLoading() const;
    QString errorMessage() const;
    bool isOtpSent() const;

    Q_INVOKABLE void login(const QString &emailOrPhone, const QString &password);
    Q_INVOKABLE void signup(const QVariantMap &userData);
    Q_INVOKABLE void submitCourierCompliance(const QString &licenseNumber, const QString &vehicleNumber);
    Q_INVOKABLE void requestOtp(const QString &phone);
    Q_INVOKABLE void verifyOtp(const QString &phone, const QString &otp);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void checkSession();

    /**
     * @brief Reset singleton state for unit test isolation.
     */
    void resetForTesting();

signals:
    void authStateChanged();
    void userProfileChanged();
    void complianceStatusChanged();
    void loadingChanged();
    void errorMessageChanged();
    void otpStateChanged();
    void loginSuccess();
    void signupSuccess();
    void complianceSubmitted();
    void sessionExpired();

private:
    void setUser(const QJsonObject &userObj, const QString &accessToken = QString(),
                 const QString &refreshToken = QString());
    void syncWithRBAC();
    QString getDeviceFingerprint() const;

    bool m_isLoggedIn{false};
    QJsonObject m_userObj;
    QString m_complianceStatus{QStringLiteral("not_submitted")};
    bool m_isLoading{false};
    QString m_errorMessage;
    bool m_isOtpSent{false};
};

#endif // AUTHSERVICE_H
