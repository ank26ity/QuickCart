/**
 * @file test_admin_flow.cpp
 * @brief Automated integration test for Administrator Flow (4e) including RBAC provisioning and shop infrastructure.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include "../../security/permissionmanager.h"
#include "../../models/shopmodel.h"
#include "../../models/ordermodel.h"
#include "../../core/orderstatemachine.h"

using Status = OrderStateMachine::OrderStatus;
using Actor = OrderStateMachine::OrderActor;

class TestAdminFlow : public QObject
{
    Q_OBJECT

private slots:
    void testAdminRbacPermissions()
    {
        PermissionManager pm;
        pm.setCurrentRole(QStringLiteral("admin"));

        // Admin has superuser capabilities across all system modules
        QVERIFY(pm.hasPermission(QStringLiteral("Admin:ManageUsers")));
        QVERIFY(pm.hasPermission(QStringLiteral("Admin:RegisterShop")));
        QVERIFY(pm.hasPermission(QStringLiteral("Admin:Dashboard")));
        QVERIFY(pm.hasPermission(QStringLiteral("Order:ViewAll")));
        QVERIFY(pm.hasPermission(QStringLiteral("Catalog:Browse")));

        // Default view for admin
        QCOMPARE(pm.defaultViewForCurrentRole(), QStringLiteral("views/AdminView.qml"));
    }

    void testAdminSuperuserOrderIntervention()
    {
        // Admin can intervene in orders at any non-terminal stage
        const QVector<Status> activeStates = {
            Status::Pending,
            Status::Accepted,
            Status::Preparing,
            Status::Ready,
            Status::Assigned,
            Status::PickedUp
        };

        for (Status s : activeStates) {
            auto canCancel = OrderStateMachine::canTransition(s, Status::Cancelled, Actor::Admin);
            QVERIFY2(canCancel.isSuccess(),
                qPrintable(QString("Admin must be permitted to cancel order in state %1").arg(OrderStateMachine::statusToString(s))));
        }
    }

    void testAdminMetricsAggregation()
    {
        ShopModel shopModel;
        QJsonArray shops;
        for (int i = 0; i < 5; ++i) {
            QJsonObject s;
            s[QStringLiteral("id")] = QString("shop_%1").arg(i);
            s[QStringLiteral("name")] = QString("Shop %1").arg(i);
            shops.append(s);
        }
        shopModel.populateFromJson(shops);
        QCOMPARE(shopModel.count(), 5);

        OrderModel orderModel;
        QJsonArray orders;
        double expectedGmv = 0.0;
        for (int i = 0; i < 4; ++i) {
            QJsonObject o;
            o[QStringLiteral("id")] = QString("order_%1").arg(i);
            o[QStringLiteral("status")] = QStringLiteral("delivering");
            double amt = 250.0 + (i * 50.0);
            o[QStringLiteral("total")] = amt;
            expectedGmv += amt;
            orders.append(o);
        }
        orderModel.populateFromJson(orders);
        QCOMPARE(orderModel.count(), 4);

        double actualGmv = 0.0;
        for (int i = 0; i < orderModel.count(); ++i) {
            actualGmv += orderModel.getOrderAt(i)[QStringLiteral("total")].toDouble();
        }
        QCOMPARE(actualGmv, expectedGmv);
    }
};

QTEST_MAIN(TestAdminFlow)
#include "test_admin_flow.moc"
