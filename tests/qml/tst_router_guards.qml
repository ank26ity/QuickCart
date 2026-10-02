import QtQuick
import QtTest
import QuickCart

Item {
    id: root
    width: 400
    height: 300

    TestCase {
        name: "RouterGuardsTestCase"
        when: windowShown

        function init() {
            Router.reset("auth")
        }

        function test_customer_guards() {
            verify(Router.isRouteAllowed("customer", "customer"))
            verify(!Router.isRouteAllowed("shopkeeper", "customer"))
            verify(!Router.isRouteAllowed("delivery", "customer"))
            verify(!Router.isRouteAllowed("admin", "customer"))
        }

        function test_shopkeeper_guards() {
            verify(Router.isRouteAllowed("shopkeeper", "shopkeeper"))
            verify(!Router.isRouteAllowed("delivery", "shopkeeper"))
            verify(!Router.isRouteAllowed("admin", "shopkeeper"))
        }

        function test_delivery_guards() {
            verify(Router.isRouteAllowed("delivery", "delivery"))
            verify(!Router.isRouteAllowed("shopkeeper", "delivery"))
            verify(!Router.isRouteAllowed("admin", "delivery"))
        }

        function test_admin_superuser() {
            var routes = ["auth", "customer", "shopkeeper", "delivery", "admin"]
            for (var i = 0; i < routes.length; ++i) {
                verify(Router.isRouteAllowed(routes[i], "admin"))
            }
        }

        function test_navigation_stack_transitions() {
            compare(Router.currentRoute, "auth")
            verify(Router.navigate("customer", "customer"))
            compare(Router.currentRoute, "customer")

            verify(!Router.navigate("admin", "customer"))
            compare(Router.currentRoute, "customer")

            verify(Router.popRoute())
            compare(Router.currentRoute, "auth")
        }
    }
}
