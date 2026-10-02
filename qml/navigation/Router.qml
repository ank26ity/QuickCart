/**
 * @file Router.qml
 * @brief Centralized navigation router with role-based route guards and history stack.
 * @layer Presentation (QML Singleton)
 * @tests Covered by tst_router.qml
 */

pragma Singleton
pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: root

    // ── Public Properties ──────────────────────────────────────────────────
    property string currentRoute: "auth"
    property var routeHistory: ["auth"]
    readonly property bool canGoBack: routeHistory.length > 1

    // ── Route Definitions ──────────────────────────────────────────────────
    readonly property string routeAuth: "auth"
    readonly property string routeCustomer: "customer"
    readonly property string routeShopkeeper: "shopkeeper"
    readonly property string routeDelivery: "delivery"
    readonly property string routeAdmin: "admin"

    // ── Navigation Methods ─────────────────────────────────────────────────
    /**
     * Navigates to target route verifying role-based authorization.
     * @param targetRoute The route identifier to navigate to.
     * @param userRole The role of the currently authenticated user.
     * @return true if navigation was allowed, false if guarded.
     */
    function navigate(targetRoute: string, userRole: string): bool {
        // Enforce route guard
        if (!isRouteAllowed(targetRoute, userRole)) {
            console.warn("[Router] Navigation guarded: Role '" + userRole + "' cannot access route '" + targetRoute + "'");
            return false;
        }

        if (root.currentRoute !== targetRoute) {
            root.routeHistory.push(targetRoute);
            root.currentRoute = targetRoute;
        }
        return true;
    }

    /**
     * Pops the active route and navigates to the previous one in the stack.
     */
    function popRoute(): bool {
        if (root.routeHistory.length > 1) {
            root.routeHistory.pop();
            root.currentRoute = root.routeHistory[root.routeHistory.length - 1];
            return true;
        }
        return false;
    }

    /**
     * Resets the navigation stack to the specified route.
     */
    function reset(defaultRoute: string) {
        root.routeHistory = [defaultRoute];
        root.currentRoute = defaultRoute;
    }

    /**
     * Maps route identifier to the corresponding QML file source path.
     */
    function sourceForRoute(route: string): string {
        switch (route) {
        case root.routeCustomer: return "views/CustomerView.qml";
        case root.routeShopkeeper: return "views/ShopkeeperView.qml";
        case root.routeDelivery: return "views/DeliveryView.qml";
        case root.routeAdmin: return "views/AdminView.qml";
        default: return "views/AuthView.qml";
        }
    }

    /**
     * Route guard: validates whether a role can access a target route.
     */
    function isRouteAllowed(route: string, role: string): bool {
        if (route === root.routeAuth) return true;
        if (role === "admin") return true; // Superuser access
        if (route === root.routeCustomer) return role === "customer";
        if (route === root.routeShopkeeper) return role === "shopkeeper";
        if (route === root.routeDelivery) return role === "delivery";
        return false;
    }
}
