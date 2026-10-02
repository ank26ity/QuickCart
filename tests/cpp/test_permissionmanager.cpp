/**
 * @file test_permissionmanager.cpp
 * @brief Automated unit test suite for Role-Based Access Control and view navigation guards.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include "../../security/permissionmanager.h"

class TestPermissionManager : public QObject {
    Q_OBJECT

private slots:
    void init() { PermissionManager::instance()->resetForTesting(); }

    void cleanup() { PermissionManager::instance()->resetForTesting(); }

    void testDefaultGuestState() {
        PermissionManager *pm = PermissionManager::instance();
        QCOMPARE(pm->currentRole(), QStringLiteral("guest"));
        QVERIFY(pm->hasPermission(QStringLiteral("Catalog:Browse")));
        QVERIFY(!pm->hasPermission(QStringLiteral("Cart:Add")));
        QVERIFY(!pm->hasPermission(QStringLiteral("Order:Create")));
        QVERIFY(!pm->hasPermission(QStringLiteral("Admin:Dashboard")));
    }

    void testCustomerRole() {
        PermissionManager *pm = PermissionManager::instance();
        pm->setCurrentRole(QStringLiteral("customer"));

        QVERIFY(pm->hasPermission(QStringLiteral("Catalog:Browse")));
        QVERIFY(pm->hasPermission(QStringLiteral("Cart:Add")));
        QVERIFY(pm->hasPermission(QStringLiteral("Order:Create")));

        // Forbidden to customer
        QVERIFY(!pm->hasPermission(QStringLiteral("Inventory:AddProduct")));
        QVERIFY(!pm->hasPermission(QStringLiteral("Delivery:ToggleDuty")));
        QVERIFY(!pm->hasPermission(QStringLiteral("Admin:ManageUsers")));
    }

    void testMerchantRole() {
        PermissionManager *pm = PermissionManager::instance();
        pm->setCurrentRole(QStringLiteral("shopkeeper"));

        QVERIFY(pm->hasPermission(QStringLiteral("Inventory:AddProduct")));
        QVERIFY(pm->hasPermission(QStringLiteral("Inventory:UpdateStock")));
        QVERIFY(pm->hasPermission(QStringLiteral("Order:Accept")));
        QVERIFY(pm->hasPermission(QStringLiteral("Order:Prepare")));

        QVERIFY(!pm->hasPermission(QStringLiteral("Admin:ManageUsers")));
        QVERIFY(!pm->hasPermission(QStringLiteral("Delivery:ToggleDuty")));
    }

    void testCourierRole() {
        PermissionManager *pm = PermissionManager::instance();
        pm->setCurrentRole(QStringLiteral("delivery"));

        QVERIFY(pm->hasPermission(QStringLiteral("Delivery:ToggleDuty")));
        QVERIFY(pm->hasPermission(QStringLiteral("Delivery:AcceptJob")));
        QVERIFY(pm->hasPermission(QStringLiteral("Delivery:PickupOrder")));
        QVERIFY(pm->hasPermission(QStringLiteral("Delivery:CompleteDelivery")));

        QVERIFY(!pm->hasPermission(QStringLiteral("Admin:ManageUsers")));
        QVERIFY(!pm->hasPermission(QStringLiteral("Inventory:AddProduct")));
    }

    void testAdminRole() {
        PermissionManager *pm = PermissionManager::instance();
        pm->setCurrentRole(QStringLiteral("admin"));

        QVERIFY(pm->hasPermission(QStringLiteral("Admin:Dashboard")));
        QVERIFY(pm->hasPermission(QStringLiteral("Admin:ManageUsers")));
        QVERIFY(pm->hasPermission(QStringLiteral("Admin:SuspendUser")));
        QVERIFY(pm->hasPermission(QStringLiteral("Order:ViewAll")));
        QVERIFY(pm->hasPermission(QStringLiteral("Order:CancelAny")));
    }

    void testNavigationGuards() {
        PermissionManager *pm = PermissionManager::instance();

        // Customer
        pm->setCurrentRole(QStringLiteral("customer"));
        QVERIFY(pm->canNavigateTo(QStringLiteral("views/CustomerView.qml")));
        QVERIFY(pm->canNavigateTo(QStringLiteral("views/AuthView.qml")));
        QVERIFY(!pm->canNavigateTo(QStringLiteral("views/AdminView.qml")));
        QVERIFY(!pm->canNavigateTo(QStringLiteral("views/ShopkeeperView.qml")));
        QVERIFY(!pm->canNavigateTo(QStringLiteral("views/DeliveryView.qml")));

        // Admin can navigate to all views
        pm->setCurrentRole(QStringLiteral("admin"));
        QVERIFY(pm->canNavigateTo(QStringLiteral("views/CustomerView.qml")));
        QVERIFY(pm->canNavigateTo(QStringLiteral("views/ShopkeeperView.qml")));
        QVERIFY(pm->canNavigateTo(QStringLiteral("views/DeliveryView.qml")));
        QVERIFY(pm->canNavigateTo(QStringLiteral("views/AdminView.qml")));
    }

    void testShopManagementGuards() {
        PermissionManager *pm = PermissionManager::instance();
        pm->setCurrentRole(QStringLiteral("shopkeeper"));
        pm->setCurrentShopId(QStringLiteral("shop_123"));

        // Own shop
        QVERIFY(pm->canManageShop(QStringLiteral("shop_123")));
        // Other merchant's shop
        QVERIFY(!pm->canManageShop(QStringLiteral("shop_999")));

        // Admin can manage any shop
        pm->setCurrentRole(QStringLiteral("admin"));
        QVERIFY(pm->canManageShop(QStringLiteral("shop_999")));
    }

    void testOrderUpdateGuards() {
        PermissionManager *pm = PermissionManager::instance();

        // Customer can update their own order
        pm->setCurrentRole(QStringLiteral("customer"));
        pm->setCurrentUserId(QStringLiteral("cust_1"));
        QVERIFY(pm->canUpdateOrder("shop_A", "rider_B", "cust_1"));
        QVERIFY(!pm->canUpdateOrder("shop_A", "rider_B", "cust_other"));

        // Courier can update assigned order
        pm->setCurrentRole(QStringLiteral("delivery"));
        pm->setCurrentUserId(QStringLiteral("rider_B"));
        QVERIFY(pm->canUpdateOrder("shop_A", "rider_B", "cust_1"));
        QVERIFY(!pm->canUpdateOrder("shop_A", "rider_other", "cust_1"));
    }
};

QTEST_MAIN(TestPermissionManager)
#include "test_permissionmanager.moc"
