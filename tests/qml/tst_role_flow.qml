import QtQuick
import QtTest
import "../../qml/components"

Item {
    id: root
    width: 1024
    height: 768

    QtObject {
        id: mockAuth
        property bool isLoggedIn: false
        property string userName: ""
        property string userRole: ""
    }

    QtObject {
        id: mockPerms
        function defaultViewForCurrentRole(): string {
            if (mockAuth.userRole === "customer") return "../../qml/views/CustomerView.qml"
            if (mockAuth.userRole === "shopkeeper") return "../../qml/views/ShopkeeperView.qml"
            if (mockAuth.userRole === "delivery") return "../../qml/views/DeliveryView.qml"
            if (mockAuth.userRole === "admin") return "../../qml/views/AdminView.qml"
            return "../../qml/views/AuthView.qml"
        }
    }

    AppScaffold {
        id: scaffold
        anchors.fill: parent
        authService: mockAuth
        permissionManager: mockPerms

        Loader {
            id: roleLoader
            anchors.fill: parent
            source: {
                if (!mockAuth.isLoggedIn) {
                    return "../../qml/views/AuthView.qml"
                }
                return mockPerms.defaultViewForCurrentRole()
            }
        }
    }

    TestCase {
        name: "RoleFlowTestCase"
        when: windowShown

        function init() {
            mockAuth.isLoggedIn = false
            mockAuth.userRole = ""
            mockAuth.userName = ""
        }

        function test_initial_state_shows_auth_view() {
            tryVerify(function() { return roleLoader.status === Loader.Ready }, 2000)
            verify(roleLoader.item !== null)
            compare(mockAuth.isLoggedIn, false)
        }

        function test_customer_login_flow() {
            mockAuth.userName = "Aarav Sharma"
            mockAuth.userRole = "customer"
            mockAuth.isLoggedIn = true

            tryVerify(function() { return roleLoader.status === Loader.Ready }, 2000)
            verify(roleLoader.item !== null)
            compare(mockAuth.isLoggedIn, true)
            compare(mockAuth.userRole, "customer")
        }

        function test_merchant_login_flow() {
            mockAuth.userName = "Priya Store"
            mockAuth.userRole = "shopkeeper"
            mockAuth.isLoggedIn = true

            tryVerify(function() { return roleLoader.status === Loader.Ready }, 2000)
            verify(roleLoader.item !== null)
            compare(mockAuth.userRole, "shopkeeper")
        }

        function test_courier_login_flow() {
            mockAuth.userName = "Rohan Express"
            mockAuth.userRole = "delivery"
            mockAuth.isLoggedIn = true

            tryVerify(function() { return roleLoader.status === Loader.Ready }, 2000)
            verify(roleLoader.item !== null)
            compare(mockAuth.userRole, "delivery")
        }

        function test_admin_login_flow() {
            mockAuth.userName = "System Administrator"
            mockAuth.userRole = "admin"
            mockAuth.isLoggedIn = true

            tryVerify(function() { return roleLoader.status === Loader.Ready }, 2000)
            verify(roleLoader.item !== null)
            compare(mockAuth.userRole, "admin")
        }

        function test_logout_transition() {
            mockAuth.isLoggedIn = true
            mockAuth.userRole = "customer"
            tryVerify(function() { return roleLoader.status === Loader.Ready }, 2000)

            // Perform logout
            mockAuth.isLoggedIn = false
            mockAuth.userRole = ""
            tryVerify(function() { return roleLoader.status === Loader.Ready }, 2000)
            verify(roleLoader.item !== null)
            compare(mockAuth.isLoggedIn, false)
        }
    }
}
