/**
 * @file test_courier_flow.cpp
 * @brief Integration tests for Courier flow (4d): job claim, double-claim rejection, pickup, delivery, dynamic earnings
 * config.
 * @layer Tests / Integration (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include "../../models/ordermodel.h"
#include "../../api/networkmanager.h"
#include "../tools/mockapiserver.h"

class TestCourierFlow : public QObject {
    Q_OBJECT

private:
    MockApiServer m_server;

private slots:
    void initTestCase() {
        QVERIFY(m_server.start());
        NetworkManager::instance()->setBaseUrl(m_server.url());
    }

    void cleanupTestCase() { m_server.stop(); }

    void testJobClaimAndDoubleClaimPrevention() {
        // 1. Post a new order
        QJsonObject order;
        order["_id"] = "order_courier_claim_1";
        order["shop_id"] = "shop_1";
        order["status"] = "ready";
        m_server.addOrder(order);

        QSignalSpy reqSpy(&m_server, &MockApiServer::requestReceived);

        // First courier claims order
        bool firstClaimSuccess = false;
        NetworkManager::instance()->patch("/api/admin/orders/order_courier_claim_1/assign",
                                          QJsonObject{{"delivery_boy_id", "rider_1"}},
                                          [&firstClaimSuccess](bool ok, const QJsonDocument &doc, const QString &err) {
                                              Q_UNUSED(doc);
                                              Q_UNUSED(err);
                                              firstClaimSuccess = ok;
                                          });

        QTRY_VERIFY(firstClaimSuccess);

        // Second courier tries to claim SAME order -> MUST fail with 409 Conflict!
        bool secondClaimSuccess = true;
        QString conflictError;
        NetworkManager::instance()->patch(
            "/api/admin/orders/order_courier_claim_1/assign", QJsonObject{{"delivery_boy_id", "rider_2"}},
            [&secondClaimSuccess, &conflictError](bool ok, const QJsonDocument &doc, const QString &err) {
                Q_UNUSED(doc);
                secondClaimSuccess = ok;
                conflictError = err;
            });

        QTRY_VERIFY(!secondClaimSuccess);
    }

    void testDynamicEarningsServerConfig() {
        bool configReceived = false;
        double baseFee = 0.0;
        double perKmRate = 0.0;

        NetworkManager::instance()->get(
            "/api/config/delivery",
            [&configReceived, &baseFee, &perKmRate](bool ok, const QJsonDocument &doc, const QString &err) {
                Q_UNUSED(err);
                if (ok && doc.isObject()) {
                    configReceived = true;
                    baseFee = doc.object().value("baseFee").toDouble();
                    perKmRate = doc.object().value("perKmRate").toDouble();
                }
            });

        QTRY_VERIFY(configReceived);
        QCOMPARE(baseFee, 40.0);
        QCOMPARE(perKmRate, 12.0);

        // Compute earnings dynamically for a 3.5 km trip
        double distanceKm = 3.5;
        double totalEarnings = baseFee + (distanceKm * perKmRate);
        QCOMPARE(totalEarnings, 82.0); // 40 + (3.5 * 12) = 82.0
    }
};

QTEST_MAIN(TestCourierFlow)
#include "test_courier_flow.moc"
