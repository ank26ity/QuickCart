/**
 * @file test_router_guards.cpp
 * @brief Automated unit test suite for Router.qml route guards, role authorization, and navigation stack.
 * @layer Tests (C++ / QML / Qt Test)
 */

#include <QtTest/QtTest>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtCore/QDir>

class TestRouterGuards : public QObject {
    Q_OBJECT

private:
    QString m_sourceDir;

private slots:
    void initTestCase() { m_sourceDir = QStringLiteral(QUICKCART_SOURCE_DIR); }

    void testRouterRoleGuards() {
        QQmlEngine engine;
        engine.addImportPath(m_sourceDir + QStringLiteral("/qml"));
        engine.addImportPath(m_sourceDir + QStringLiteral("/qml/navigation"));

        QQmlComponent component(&engine,
                                QUrl::fromLocalFile(m_sourceDir + QStringLiteral("/qml/navigation/Router.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));

        QObject *router = component.create();
        QVERIFY(router != nullptr);

        // 1. Unauthenticated or Customer accessing Customer Route -> Allowed
        bool allowedCustomer = false;
        QMetaObject::invokeMethod(router, "isRouteAllowed", Q_RETURN_ARG(bool, allowedCustomer),
                                  Q_ARG(QString, QStringLiteral("customer")),
                                  Q_ARG(QString, QStringLiteral("customer")));
        QVERIFY(allowedCustomer);

        // 2. Customer accessing Shopkeeper Route -> Guarded / Rejected
        bool customerToShop = true;
        QMetaObject::invokeMethod(router, "isRouteAllowed", Q_RETURN_ARG(bool, customerToShop),
                                  Q_ARG(QString, QStringLiteral("shopkeeper")),
                                  Q_ARG(QString, QStringLiteral("customer")));
        QVERIFY(!customerToShop);

        // 3. Customer accessing Delivery Route -> Guarded / Rejected
        bool customerToDelivery = true;
        QMetaObject::invokeMethod(router, "isRouteAllowed", Q_RETURN_ARG(bool, customerToDelivery),
                                  Q_ARG(QString, QStringLiteral("delivery")),
                                  Q_ARG(QString, QStringLiteral("customer")));
        QVERIFY(!customerToDelivery);

        // 4. Customer accessing Admin Route -> Guarded / Rejected
        bool customerToAdmin = true;
        QMetaObject::invokeMethod(router, "isRouteAllowed", Q_RETURN_ARG(bool, customerToAdmin),
                                  Q_ARG(QString, QStringLiteral("admin")), Q_ARG(QString, QStringLiteral("customer")));
        QVERIFY(!customerToAdmin);

        // 5. Courier accessing Delivery Route -> Allowed
        bool courierToDelivery = false;
        QMetaObject::invokeMethod(router, "isRouteAllowed", Q_RETURN_ARG(bool, courierToDelivery),
                                  Q_ARG(QString, QStringLiteral("delivery")),
                                  Q_ARG(QString, QStringLiteral("delivery")));
        QVERIFY(courierToDelivery);

        // 6. Admin is superuser -> Allowed across all routes
        const QStringList allRoutes = {"auth", "customer", "shopkeeper", "delivery", "admin"};
        for (const QString &route : allRoutes) {
            bool adminAccess = false;
            QMetaObject::invokeMethod(router, "isRouteAllowed", Q_RETURN_ARG(bool, adminAccess), Q_ARG(QString, route),
                                      Q_ARG(QString, QStringLiteral("admin")));
            QVERIFY2(adminAccess, qPrintable(QString("Admin should have access to route: %1").arg(route)));
        }

        // 7. Navigation stack history and pop
        bool navResult = false;
        QMetaObject::invokeMethod(router, "navigate", Q_RETURN_ARG(bool, navResult),
                                  Q_ARG(QString, QStringLiteral("customer")),
                                  Q_ARG(QString, QStringLiteral("customer")));
        QVERIFY(navResult);
        QCOMPARE(router->property("currentRoute").toString(), QStringLiteral("customer"));

        // Pop route back to auth
        bool popResult = false;
        QMetaObject::invokeMethod(router, "popRoute", Q_RETURN_ARG(bool, popResult));
        QVERIFY(popResult);
        QCOMPARE(router->property("currentRoute").toString(), QStringLiteral("auth"));

        delete router;
    }
};

QTEST_MAIN(TestRouterGuards)
#include "test_router_guards.moc"
