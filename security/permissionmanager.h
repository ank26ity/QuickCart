/**
 * @file permissionmanager.h
 * @brief Role-Based Access Control (RBAC) engine and route navigation guard.
 * @layer Security (Layer 2 - Core Services)
 *
 * Public API Summary:
 * - enum class Role: Guest, Customer, Shopkeeper, Delivery, Admin.
 * - currentRole(), setCurrentRole(role): Active security context.
 * - currentUserId(), setCurrentUserId(id): Active subject ID.
 * - currentShopId(), setCurrentShopId(id): Owned merchant ID for resource filtering.
 * - hasPermission(permissionName): Verify if active role possesses permission.
 * - canNavigateTo(viewName): Route guard for view screens.
 * - canManageShop(targetShopId): Multi-tenant ownership guard.
 * - canUpdateOrder(shopId, courierId, customerId): Order participant verification.
 * - resetForTesting(): Reset singleton state between test executions.
 *
 * Dependencies:
 * - <QtCore/QObject>, <QtCore/QSet>, <QtCore/QMap>
 *
 * Tests:
 * - Covered by tests/cpp/test_permissionmanager.cpp
 */

#ifndef PERMISSIONMANAGER_H
#define PERMISSIONMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QSet>
#include <QtCore/QMap>

class PermissionManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentRole READ currentRole WRITE setCurrentRole NOTIFY roleChanged)
    Q_PROPERTY(QString currentUserId READ currentUserId WRITE setCurrentUserId NOTIFY userChanged)
    Q_PROPERTY(QString currentShopId READ currentShopId WRITE setCurrentShopId NOTIFY shopChanged)
    Q_PROPERTY(QStringList activePermissions READ activePermissions NOTIFY permissionsChanged)

public:
    enum class Role {
        Guest,
        Customer,
        Shopkeeper,
        Delivery,
        Admin
    };
    Q_ENUM(Role)

    explicit PermissionManager(QObject *parent = nullptr);

    /**
     * @brief Singleton accessor with lazy auto-instantiation.
     */
    static PermissionManager* instance();

    /**
     * @brief Retrieve current role as lowercase string.
     */
    QString currentRole() const;

    /**
     * @brief Set active role and recompute permissions.
     */
    void setCurrentRole(const QString &role);

    /**
     * @brief Retrieve active authenticated user ID.
     */
    QString currentUserId() const;

    /**
     * @brief Set active user ID.
     */
    void setCurrentUserId(const QString &userId);

    /**
     * @brief Retrieve current managed merchant shop ID.
     */
    QString currentShopId() const;

    /**
     * @brief Set current managed merchant shop ID.
     */
    void setCurrentShopId(const QString &shopId);

    /**
     * @brief Retrieve all permissions granted to the current role.
     */
    QStringList activePermissions() const;

    /**
     * @brief Query if current role is granted a named permission.
     */
    Q_INVOKABLE bool hasPermission(const QString &permissionName) const;

    /**
     * @brief Route guard checking if current role may open the specified view.
     */
    Q_INVOKABLE bool canNavigateTo(const QString &viewName) const;

    /**
     * @brief Verify if user is admin or owns the specified shop.
     */
    Q_INVOKABLE bool canManageShop(const QString &targetShopId);

    /**
     * @brief Verify if user is an authorized participant in the order.
     */
    Q_INVOKABLE bool canUpdateOrder(const QString &orderShopId, const QString &orderCourierId, const QString &orderCustomerId);

    /**
     * @brief Default home view based on current role.
     */
    Q_INVOKABLE QString defaultViewForCurrentRole() const;

    /**
     * @brief Log security auditing events for authorization attempts.
     */
    void logAccessAttempt(const QString &action, const QString &resource, bool granted, const QString &reason = QString());

    /**
     * @brief Resets role and identity state for clean automated test isolation.
     */
    void resetForTesting();

signals:
    void roleChanged();
    void userChanged();
    void shopChanged();
    void permissionsChanged();
    void accessDenied(const QString &action, const QString &resource, const QString &reason);

private:
    Role m_role{Role::Guest};
    QString m_roleString{QStringLiteral("guest")};
    QString m_userId;
    QString m_shopId;
    QMap<Role, QSet<QString>> m_rolePermissions;

    void initializePermissionMatrix();
    Role roleFromString(const QString &roleStr) const;
};

#endif // PERMISSIONMANAGER_H
