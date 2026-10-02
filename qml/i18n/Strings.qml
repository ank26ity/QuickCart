/**
 * @file Strings.qml
 * @brief Centralized localization string repository using qsTr.
 * @layer Presentation (QML Singleton)
 */

pragma Singleton
pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: root

    // ── Application Branding ───────────────────────────────────────────────
    readonly property string appName: qsTr("QuickShopp")
    readonly property string appTagline: qsTr("Premium Hyperlocal Delivery")

    // ── Navigation & Actions ───────────────────────────────────────────────
    readonly property string navMarketplace: qsTr("Marketplace")
    readonly property string navOrders: qsTr("Orders")
    readonly property string navAccount: qsTr("Account")
    readonly property string logout: qsTr("Logout")
    readonly property string login: qsTr("Login")
    readonly property string signup: qsTr("Sign Up")
    readonly property string backToStores: qsTr("Back to Stores")
    readonly property string close: qsTr("Close")

    // ── Catalog & Cart ─────────────────────────────────────────────────────
    readonly property string allShops: qsTr("All Shops")
    readonly property string grocery: qsTr("Grocery")
    readonly property string restaurants: qsTr("Restaurants")
    readonly property string pharmacy: qsTr("Pharmacy")
    readonly property string shoppingCart: qsTr("Shopping Cart")
    readonly property string subtotal: qsTr("Subtotal")
    readonly property string deliveryFee: qsTr("Delivery Fee")
    readonly property string total: qsTr("Total")
    readonly property string placeOrder: qsTr("Place Order Now")
    readonly property string emptyCart: qsTr("Your cart is currently empty")
    readonly property string outOfStock: qsTr("Out of Stock")
    readonly property string lowStock: qsTr("Low Stock")

    // ── Status Badges ──────────────────────────────────────────────────────
    readonly property string statusPending: qsTr("Pending")
    readonly property string statusPreparing: qsTr("Preparing")
    readonly property string statusReady: qsTr("Ready for Pickup")
    readonly property string statusOutForDelivery: qsTr("Out for Delivery")
    readonly property string statusDelivered: qsTr("Delivered")
    readonly property string statusCancelled: qsTr("Cancelled")

    // ── Appearance ─────────────────────────────────────────────────────────
    readonly property string themeSystem: qsTr("System")
    readonly property string themeLight: qsTr("Light")
    readonly property string themeDark: qsTr("Dark")
}
