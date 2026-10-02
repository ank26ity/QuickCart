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
#include "../../api/networkmanager.h"
#include "../tools/mockapiserver.h"

using Status = OrderStateMachine::OrderStatus;
using Actor = OrderStateMachine::OrderActor;

class TestAdminFlow : public QObject
{
    Q_OBJECT

private:
    MockApiServer m_server;

private slots:
    void initTestCase()
    {
        QVERIFY(m_server.start());
        NetworkManager::instance()->setBaseUrl(m_server.url());
    }

    void cleanupTestCase()
    {
        m_server.stop();
    }

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

    void testCourierDocumentApprovalFlow()
    {
        // 1. Approve courier document via admin endpoint
        QString courierId = QStringLiteral("user_courier_1");
        bool done = false;
        bool ok = false;
        QJsonObject resp;

        NetworkManager::instance()->post(
            QString("/api/admin/courier/%1/approve").arg(courierId),
            QJsonObject(),
            [&](bool success, const QJsonDocument &doc, const QString &) {
                done = true;
                ok = success;
                resp = doc.object();
            }
        );

        QTRY_VERIFY_WITH_TIMEOUT(done, 3000);
        QVERIFY(ok);
        QCOMPARE(resp.value("status").toString(), QStringLiteral("approved"));
    }

    void testUserAndShopSuspensionFlow()
    {
        // 2. Suspend malicious user via admin endpoint
        QString badUserId = QStringLiteral("user_cust_1");
        bool done = false;
        bool ok = false;
        QJsonObject resp;

        NetworkManager::instance()->post(
            QString("/api/admin/users/%1/suspend").arg(badUserId),
            QJsonObject(),
            [&](bool success, const QJsonDocument &doc, const QString &) {
                done = true;
                ok = success;
                resp = doc.object();
            }
        );

        QTRY_VERIFY_WITH_TIMEOUT(done, 3000);
        QVERIFY(ok);
        QCOMPARE(resp.value("status").toString(), QStringLiteral("suspended"));
    }

    void testManualDispatchFlow()
    {
        // 3. Admin manually dispatches ready order to courier
        QString orderId = QStringLiteral("order_dispatch_101");
        QJsonObject order;
        order["_id"] = orderId;
        order["status"] = "ready";
        m_server.addOrder(order);

        QJsonObject req;
        req["delivery_boy_id"] = QStringLiteral("user_courier_1");

        bool done = false;
        bool ok = false;
        QJsonObject resp;

        NetworkManager::instance()->post(
            QString("/api/admin/orders/%1/dispatch").arg(orderId),
            req,
            [&](bool success, const QJsonDocument &doc, const QString &) {
                done = true;
                ok = success;
                resp = doc.object();
            }
        );

        QTRY_VERIFY_WITH_TIMEOUT(done, 3000);
        QVERIFY(ok);
        QCOMPARE(resp.value("status").toString(), QStringLiteral("assigned"));
    }

    void testAuditLogVerification()
    {
        // 4. Verify admin actions generated audit logs
        bool done = false;
        bool ok = false;
        QJsonArray logs;

        NetworkManager::instance()->get(
            QStringLiteral("/api/admin/audit-logs"),
            [&](bool success, const QJsonDocument &doc, const QString &) {
                done = true;
                ok = success;
                logs = doc.array();
            }
        );

        QTRY_VERIFY_WITH_TIMEOUT(done, 3000);
        QVERIFY(ok);
        QVERIFY(logs.size() >= 3); // courier_approved, user_suspended, manual_dispatch

        bool foundApproval = false;
        bool foundSuspension = false;
        bool foundDispatch = false;

        for (const auto &v : logs) {
            QString action = v.toObject().value("action").toString();
            if (action == QStringLiteral("courier_approved")) foundApproval = true;
            if (action == QStringLiteral("user_suspended")) foundSuspension = true;
            if (action == QStringLiteral("manual_dispatch")) foundDispatch = true;
        }

        QVERIFY(foundApproval);
        QVERIFY(foundSuspension);
        QVERIFY(foundDispatch);
    }
};

QTEST_MAIN(TestAdminFlow)
#include "test_admin_flow.moc"
