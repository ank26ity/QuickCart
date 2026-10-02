/**
 * @file test_network_resilience.cpp
 * @brief Integration tests for Network resilience (4h): retry with backoff, circuit breaker, error mapping.
 * @layer Tests / Integration (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include "../../api/networkmanager.h"
#include "../../api/circuitbreaker.h"
#include "../tools/mockapiserver.h"

class TestNetworkResilience : public QObject {
    Q_OBJECT

private:
    MockApiServer m_server;

private slots:
    void initTestCase() {
        QVERIFY(m_server.start());
        NetworkManager::instance()->setBaseUrl(m_server.url());
    }

    void cleanupTestCase() { m_server.stop(); }

    void testTransientFailureRetrySuccess() {
        // Simulate 1 transient 503 error, next request succeeds
        m_server.setFailNextRequests(1, 503);

        bool requestDone = false;
        bool wasSuccessful = false;

        NetworkManager::instance()->get(
            "/api/shops", [&requestDone, &wasSuccessful](bool ok, const QJsonDocument &doc, const QString &err) {
                Q_UNUSED(doc);
                Q_UNUSED(err);
                requestDone = true;
                wasSuccessful = ok;
            });

        // NetworkManager handles transient 503 by automatically retrying with backoff
        QTRY_VERIFY_WITH_TIMEOUT(requestDone, 5000);
        QVERIFY(wasSuccessful);
    }

    void testCircuitBreakerTripping() {
        CircuitBreaker cb(3, 500); // 3 failures trips, 500ms recovery

        QString endpoint = "/api/fragile-service";
        QVERIFY(cb.canExecute(endpoint));

        cb.recordFailure(endpoint);
        cb.recordFailure(endpoint);
        QVERIFY(cb.canExecute(endpoint));

        // Third failure trips breaker
        cb.recordFailure(endpoint);
        QCOMPARE(cb.getState(endpoint), CircuitBreaker::State::Open);
        QVERIFY(!cb.canExecute(endpoint)); // Fast fails without dispatching!

        // Wait recovery timeout
        QTest::qWait(600);
        QCOMPARE(cb.getState(endpoint), CircuitBreaker::State::HalfOpen);
        QVERIFY(cb.canExecute(endpoint));

        // Two consecutive successes in HalfOpen closes breaker
        cb.recordSuccess(endpoint);
        cb.recordSuccess(endpoint);
        QCOMPARE(cb.getState(endpoint), CircuitBreaker::State::Closed);
    }
};

QTEST_MAIN(TestNetworkResilience)
#include "test_network_resilience.moc"
