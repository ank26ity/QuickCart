/**
 * @file test_polling_sync.cpp
 * @brief Automated unit test suite for OrderModel background polling synchronization (4g).
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include "../../models/ordermodel.h"

class TestPollingSync : public QObject {
    Q_OBJECT

private slots:
    void testPollingStartAndStop() {
        OrderModel model;
        // Verify initial state
        QCOMPARE(model.count(), 0);

        // Start polling with short interval (100ms)
        model.startPolling(100);

        // Calling startPolling again when already running is safe
        model.startPolling(100);

        // Stop polling
        model.stopPolling();

        // Calling stopPolling again when stopped is safe
        model.stopPolling();
    }

    void testOrderQueueSyncUpdate() {
        OrderModel model;

        QJsonArray initialOrders;
        QJsonObject o1;
        o1[QStringLiteral("id")] = QStringLiteral("ord_101");
        o1[QStringLiteral("status")] = QStringLiteral("pending");
        o1[QStringLiteral("address")] = QStringLiteral("Connaught Place 12");
        o1[QStringLiteral("total")] = 250.0;
        initialOrders.append(o1);

        model.populateFromJson(initialOrders);
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.getOrderAt(0)[QStringLiteral("status")].toString(), QStringLiteral("pending"));

        // Simulate server push / polling sync update where order moves to "preparing"
        QJsonArray syncedOrders;
        QJsonObject o1Updated = o1;
        o1Updated[QStringLiteral("status")] = QStringLiteral("preparing");
        syncedOrders.append(o1Updated);

        QJsonObject o2;
        o2[QStringLiteral("id")] = QStringLiteral("ord_102");
        o2[QStringLiteral("status")] = QStringLiteral("ready");
        o2[QStringLiteral("address")] = QStringLiteral("Sector 18 Noida");
        o2[QStringLiteral("total")] = 450.0;
        syncedOrders.append(o2);

        QSignalSpy countSpy(&model, &OrderModel::countChanged);
        model.populateFromJson(syncedOrders);

        // Model synchronizes seamlessly
        QCOMPARE(model.count(), 2);
        QCOMPARE(countSpy.count(), 1);
        QCOMPARE(model.getOrderAt(0)[QStringLiteral("status")].toString(), QStringLiteral("preparing"));
        QCOMPARE(model.getOrderAt(1)[QStringLiteral("status")].toString(), QStringLiteral("ready"));
    }
};

QTEST_MAIN(TestPollingSync)
#include "test_polling_sync.moc"
