/**
 * @file test_merchant_flow.cpp
 * @brief Integration tests for Merchant flow (4c & 4f): order queue, state transitions (Pending->Accepted->Preparing->Ready).
 * @layer Tests / Integration (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include "../../models/ordermodel.h"
#include "../../api/networkmanager.h"
#include "../tools/mockapiserver.h"

class TestMerchantFlow : public QObject
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

    void testMerchantLifecycleTransitions()
    {
        // 1. Seed a pending order into mock server directly
        QJsonObject pendingOrder;
        pendingOrder["_id"] = "order_merch_test_1";
        pendingOrder["shop_id"] = "shop_1";
        pendingOrder["status"] = "pending";
        pendingOrder["subtotal"] = 240.0;
        pendingOrder["total"] = 290.0;
        m_server.addOrder(pendingOrder);

        QJsonArray orderList;
        orderList.append(pendingOrder);

        OrderModel model;
        model.populateFromJson(orderList);
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.getOrderAt(0).value("status").toString(), QStringLiteral("pending"));

        // 2. Merchant rejects illegal transition directly to delivered
        model.updateOrderStatus("order_merch_test_1", "delivered", "merchant");
        QVERIFY(!model.errorMessage().isEmpty());

        // 3. Valid transition: pending -> accepted
        model.updateOrderStatus("order_merch_test_1", "accepted", "merchant");
        QVERIFY(model.errorMessage().isEmpty());
        QTRY_COMPARE(model.getOrderAt(0).value("status").toString(), QStringLiteral("accepted"));

        // 4. Valid transition: accepted -> preparing
        model.updateOrderStatus("order_merch_test_1", "preparing", "merchant");
        QVERIFY(model.errorMessage().isEmpty());
        QTRY_COMPARE(model.getOrderAt(0).value("status").toString(), QStringLiteral("preparing"));

        // 5. Valid transition: preparing -> ready
        model.updateOrderStatus("order_merch_test_1", "ready", "merchant");
        QVERIFY(model.errorMessage().isEmpty());
        QTRY_COMPARE(model.getOrderAt(0).value("status").toString(), QStringLiteral("ready"));
    }
};

QTEST_MAIN(TestMerchantFlow)
#include "test_merchant_flow.moc"
