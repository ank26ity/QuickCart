pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import "components"
import "theme"

ApplicationWindow {
    id: window

    property var authService: null
    property var shopModel: null
    property var productModel: null
    property var cartManager: null
    property var orderModel: null
    property var networkManager: null
    property var permissionManager: null
    property var appConfig: null
    property var themeManager: null
    property var secureStorage: null

    visible: true
    width: 1200
    height: 800
    minimumWidth: 360
    minimumHeight: 640
    title: "QuickCart | Responsive Multi-Platform Delivery Platform"
    color: Theme.background

    Behavior on color { ColorAnimation { duration: Theme.durationFast } }

    Component.onCompleted: {
        Theme.themeManager = window.themeManager
        if (mainViewLoader.item) {
            mainViewLoader.injectDependencies(mainViewLoader.item)
        }
    }

    onAuthServiceChanged: {
        if (mainViewLoader.item) {
            mainViewLoader.injectDependencies(mainViewLoader.item)
        }
    }

    onShopModelChanged: {
        if (mainViewLoader.item) {
            mainViewLoader.injectDependencies(mainViewLoader.item)
        }
    }

    onPermissionManagerChanged: {
        if (mainViewLoader.item) {
            mainViewLoader.injectDependencies(mainViewLoader.item)
        }
    }

    AppScaffold {
        anchors.fill: parent
        authService: window.authService
        permissionManager: window.permissionManager
        onLogoutClicked: {
            if (window.authService) window.authService.logout()
        }
        onBrandClicked: {
            if (window.authService && window.authService.isLoggedIn) {
                if (window.permissionManager && window.permissionManager.hasPermission("Catalog:Browse") && window.shopModel) {
                    window.shopModel.fetchShops()
                }
            }
        }

        Loader {
            id: mainViewLoader
            anchors.fill: parent

            function injectDependencies(targetItem) {
                if (!targetItem) return
                if ("authService" in targetItem) targetItem.authService = window.authService
                if ("shopModel" in targetItem) targetItem.shopModel = window.shopModel
                if ("productModel" in targetItem) targetItem.productModel = window.productModel
                if ("cartManager" in targetItem) targetItem.cartManager = window.cartManager
                if ("orderModel" in targetItem) targetItem.orderModel = window.orderModel
                if ("networkManager" in targetItem) targetItem.networkManager = window.networkManager
                if ("permissionManager" in targetItem) targetItem.permissionManager = window.permissionManager
            }

            source: {
                if (!window.authService || !window.authService.isLoggedIn) {
                    return "views/AuthView.qml"
                }
                if (window.permissionManager) {
                    return window.permissionManager.defaultViewForCurrentRole()
                }
                return "views/AuthView.qml"
            }

            onLoaded: {
                injectDependencies(item)
            }
        }
    }
}
