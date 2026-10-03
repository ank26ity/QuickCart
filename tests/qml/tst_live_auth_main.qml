import QtQuick
import QtQuick.Controls
import QtTest

Item {
    id: testRoot
    width: 1200
    height: 800

    property var mainWindow: null

    TestCase {
        name: "LiveAuthMainFlowTest"
        when: windowShown

        function init() {
            if (typeof liveSecureStorage !== "undefined" && liveSecureStorage) {
                liveSecureStorage.clearAllSecrets()
            }
            if (typeof liveAuthService !== "undefined" && liveAuthService) {
                liveAuthService.logout()
            }
        }

        function cleanup() {
            if (mainWindow) {
                mainWindow.close()
                mainWindow.destroy()
                mainWindow = null
            }
            if (typeof liveAuthService !== "undefined" && liveAuthService) {
                liveAuthService.logout()
            }
        }

        function test_real_auth_service_with_mock_server_through_main_qml() {
            var component = Qt.createComponent("../../qml/main.qml")
            compare(component.status, Component.Ready)

            mainWindow = component.createObject(testRoot, {
                authService: liveAuthService,
                permissionManager: livePermissionManager,
                shopModel: liveShopModel,
                productModel: liveProductModel,
                cartManager: liveCartManager,
                orderModel: liveOrderModel,
                networkManager: liveNetworkManager,
                appConfig: liveAppConfig,
                themeManager: liveThemeManager,
                secureStorage: liveSecureStorage
            })
            verify(mainWindow !== null)
            compare(liveAuthService.isLoggedIn, false)

            // Initial window is ready and visible
            tryVerify(function() {
                return mainWindow !== null && mainWindow.visible
            }, 3000)

            // Trigger real network authentication against MockApiServer
            liveAuthService.login("customer@quickcart.com", "Password123!")

            // Verify live network response resolves and session state updates
            tryVerify(function() {
                return liveAuthService.isLoggedIn === true && liveAuthService.userName === "Alice Customer"
            }, 5000)
            compare(liveAuthService.userRole, "customer")

            // Real logout cleans up session
            liveAuthService.logout()
            tryVerify(function() {
                return liveAuthService.isLoggedIn === false && liveAuthService.userRole === ""
            }, 3000)
        }
    }
}
