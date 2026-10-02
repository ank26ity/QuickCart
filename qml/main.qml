pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import "components"
import "theme"

ApplicationWindow {
    id: window

    property var authService: null
    property var shopModel: null
    property var permissionManager: null
    property var appConfig: null
    property var themeManager: null

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

            source: {
                if (!window.authService || !window.authService.isLoggedIn) {
                    return "views/AuthView.qml"
                }
                if (window.permissionManager) {
                    return window.permissionManager.defaultViewForCurrentRole()
                }
                return "views/AuthView.qml"
            }
        }
    }
}
