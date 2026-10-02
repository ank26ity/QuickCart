/**
 * @file permissionmanager.cpp
 * @brief Implementation of PermissionManager RBAC engine.
 * @layer Security (Layer 2 - Core Services)
 * @tests Covered by tests/cpp/test_permissionmanager.cpp
 */

#include "permissionmanager.h"
#include "../core/logging.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>

static PermissionManager *s_permissionManagerInstance = nullptr;

PermissionManager::PermissionManager(QObject *parent)
    : QObject(parent)
{
    s_permissionManagerInstance = this;
    initializePermissionMatrix();
}

PermissionManager* PermissionManager::instance()
{
    if (!s_permissionManagerInstance) {
        new PermissionManager(qApp);
    }
    return s_permissionManagerInstance;
}

void PermissionManager::initializePermissionMatrix()
{
    // Guest permissions
    m_rolePermissions[Role::Guest] = {
        QStringLiteral("Auth:Login"),
        QStringLiteral("Auth:Signup"),
        QStringLiteral("Catalog:Browse")
    };

    // Customer permissions
    m_rolePermissions[Role::Customer] = {
        QStringLiteral("Auth:Login"),
        QStringLiteral("Auth:Logout"),
        QStringLiteral("Catalog:Browse"),
        QStringLiteral("Cart:Add"),
        QStringLiteral("Cart:Modify"),
        QStringLiteral("Cart:Clear"),
        QStringLiteral("Order:Create"),
        QStringLiteral("Order:ViewSelf"),
        QStringLiteral("Order:CancelSelf"),
        QStringLiteral("Profile:ManageSelf")
    };

    // Shopkeeper / Merchant permissions
    m_rolePermissions[Role::Shopkeeper] = {
        QStringLiteral("Auth:Login"),
        QStringLiteral("Auth:Logout"),
        QStringLiteral("Shop:ViewProfile"),
        QStringLiteral("Shop:UpdateProfile"),
        QStringLiteral("Inventory:View"),
        QStringLiteral("Inventory:UpdateStock"),
        QStringLiteral("Inventory:AddProduct"),
        QStringLiteral("Inventory:EditProduct"),
        QStringLiteral("Order:ViewShopQueue"),
        QStringLiteral("Order:Accept"),
        QStringLiteral("Order:Prepare"),
        QStringLiteral("Order:MarkReady"),
        QStringLiteral("Profile:ManageSelf")
    };

    // Courier / Delivery rider permissions
    m_rolePermissions[Role::Delivery] = {
        QStringLiteral("Auth:Login"),
        QStringLiteral("Auth:Logout"),
        QStringLiteral("Delivery:ToggleDuty"),
        QStringLiteral("Delivery:ViewAvailableJobs"),
        QStringLiteral("Delivery:AcceptJob"),
        QStringLiteral("Delivery:UpdateLocation"),
        QStringLiteral("Delivery:PickupOrder"),
        QStringLiteral("Delivery:CompleteDelivery"),
        QStringLiteral("Profile:ManageSelf")
    };

    // Admin permissions (Superuser)
    m_rolePermissions[Role::Admin] = {
        QStringLiteral("Auth:Login"),
        QStringLiteral("Auth:Logout"),
        QStringLiteral("Admin:Dashboard"),
        QStringLiteral("Admin:ManageUsers"),
        QStringLiteral("Admin:SuspendUser"),
        QStringLiteral("Admin:RegisterShop"),
        QStringLiteral("Admin:DispatchOrder"),
        QStringLiteral("Admin:ViewAuditLogs"),
        QStringLiteral("Catalog:Browse"),
        QStringLiteral("Inventory:View"),
        QStringLiteral("Order:ViewAll"),
        QStringLiteral("Order:CancelAny")
    };
}

PermissionManager::Role PermissionManager::roleFromString(const QString &roleStr) const
{
    QString r = roleStr.trimmed().toLower();
    if (r == QStringLiteral("customer")) return Role::Customer;
    if (r == QStringLiteral("shopkeeper") || r == QStringLiteral("merchant")) return Role::Shopkeeper;
    if (r == QStringLiteral("delivery") || r == QStringLiteral("courier") || r == QStringLiteral("rider")) return Role::Delivery;
    if (r == QStringLiteral("admin")) return Role::Admin;
    return Role::Guest;
}

QString PermissionManager::currentRole() const
{
    return m_roleString;
}

void PermissionManager::setCurrentRole(const QString &role)
{
    if (m_roleString != role) {
        m_roleString = role.toLower();
        m_role = roleFromString(role);
        qCDebug(qcRbac) << "RBAC role transitioned to:" << m_roleString;
        emit roleChanged();
        emit permissionsChanged();
    }
}

QString PermissionManager::currentUserId() const
{
    return m_userId;
}

void PermissionManager::setCurrentUserId(const QString &userId)
{
    if (m_userId != userId) {
        m_userId = userId;
        emit userChanged();
    }
}

QString PermissionManager::currentShopId() const
{
    return m_shopId;
}

void PermissionManager::setCurrentShopId(const QString &shopId)
{
    if (m_shopId != shopId) {
        m_shopId = shopId;
        emit shopChanged();
    }
}

QStringList PermissionManager::activePermissions() const
{
    return m_rolePermissions.value(m_role).values();
}

bool PermissionManager::hasPermission(const QString &permissionName) const
{
    bool granted = m_rolePermissions.value(m_role).contains(permissionName);
    if (!granted) {
        qCDebug(qcRbac) << "Permission check denied for" << permissionName << "under role" << m_roleString;
    }
    return granted;
}

bool PermissionManager::canNavigateTo(const QString &viewName) const
{
    if (viewName.contains(QStringLiteral("AuthView"), Qt::CaseInsensitive)) {
        return true;
    }
    if (viewName.contains(QStringLiteral("CustomerView"), Qt::CaseInsensitive)) {
        return m_role == Role::Customer || m_role == Role::Admin;
    }
    if (viewName.contains(QStringLiteral("ShopkeeperView"), Qt::CaseInsensitive)) {
        return m_role == Role::Shopkeeper || m_role == Role::Admin;
    }
    if (viewName.contains(QStringLiteral("DeliveryView"), Qt::CaseInsensitive)) {
        return m_role == Role::Delivery || m_role == Role::Admin;
    }
    if (viewName.contains(QStringLiteral("AdminView"), Qt::CaseInsensitive)) {
        return m_role == Role::Admin;
    }
    return false;
}

bool PermissionManager::canManageShop(const QString &targetShopId)
{
    if (m_role == Role::Admin) return true;
    if (m_role == Role::Shopkeeper && !m_shopId.isEmpty() && m_shopId == targetShopId) {
        return true;
    }
    logAccessAttempt(QStringLiteral("ManageShop"), targetShopId, false, QStringLiteral("Merchant does not own shop"));
    return false;
}

bool PermissionManager::canUpdateOrder(const QString &orderShopId, const QString &orderCourierId, const QString &orderCustomerId)
{
    if (m_role == Role::Admin) return true;
    if (m_role == Role::Shopkeeper && !m_shopId.isEmpty() && m_shopId == orderShopId) return true;
    if (m_role == Role::Delivery && !m_userId.isEmpty() && m_userId == orderCourierId) return true;
    if (m_role == Role::Customer && !m_userId.isEmpty() && m_userId == orderCustomerId) return true;

    logAccessAttempt(QStringLiteral("UpdateOrder"), orderShopId, false, QStringLiteral("User does not own or participate in order"));
    return false;
}

QString PermissionManager::defaultViewForCurrentRole() const
{
    switch (m_role) {
    case Role::Customer: return QStringLiteral("views/CustomerView.qml");
    case Role::Shopkeeper: return QStringLiteral("views/ShopkeeperView.qml");
    case Role::Delivery: return QStringLiteral("views/DeliveryView.qml");
    case Role::Admin: return QStringLiteral("views/AdminView.qml");
    default: return QStringLiteral("views/AuthView.qml");
    }
}

void PermissionManager::logAccessAttempt(const QString &action, const QString &resource, bool granted, const QString &reason)
{
    QString timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (!granted) {
        qCWarning(qcRbac) << "[AUDIT-DENIED]" << timestamp
                          << "User:" << m_userId
                          << "Role:" << m_roleString
                          << "Action:" << action
                          << "Resource:" << resource
                          << "Reason:" << reason;
        emit accessDenied(action, resource, reason);
    } else {
        qCDebug(qcRbac) << "[AUDIT-GRANTED]" << timestamp
                        << "User:" << m_userId
                        << "Action:" << action
                        << "Resource:" << resource;
    }
}

void PermissionManager::resetForTesting()
{
    m_role = Role::Guest;
    m_roleString = QStringLiteral("guest");
    m_userId.clear();
    m_shopId.clear();
    emit roleChanged();
    emit userChanged();
    emit shopChanged();
    emit permissionsChanged();
}
