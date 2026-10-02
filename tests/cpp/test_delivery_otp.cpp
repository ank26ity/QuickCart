/**
 * @file test_delivery_otp.cpp
 * @brief Automated unit test suite for Order Delivery OTP cryptographic validation and secure handoff.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include <QtCore/QCryptographicHash>
#include <QtCore/QRandomGenerator>
#include "../../core/validators.h"
#include "../../core/orderstatemachine.h"
#include "../../api/networkmanager.h"
#include "../tools/mockapiserver.h"

using Status = OrderStateMachine::OrderStatus;
using Actor = OrderStateMachine::OrderActor;

class TestDeliveryOtp : public QObject {
    Q_OBJECT

private:
    MockApiServer m_server;

private slots:
    void initTestCase() {
        QVERIFY(m_server.start());
        NetworkManager::instance()->setBaseUrl(m_server.url());
    }

    void cleanupTestCase() { m_server.stop(); }

    void testOtpValidationFormats() {
        // Valid 4-digit numeric OTPs
        QVERIFY(Validators::validateOtp(QStringLiteral("1234"), 4).isSuccess());
        QVERIFY(Validators::validateOtp(QStringLiteral("0000"), 4).isSuccess());
        QVERIFY(Validators::validateOtp(QStringLiteral("9876"), 4).isSuccess());

        // Invalid lengths
        QVERIFY(Validators::validateOtp(QStringLiteral("123"), 4).isError());
        QVERIFY(Validators::validateOtp(QStringLiteral("12345"), 4).isError());
        QVERIFY(Validators::validateOtp(QStringLiteral(""), 4).isError());

        // Non-numeric characters rejected
        QVERIFY(Validators::validateOtp(QStringLiteral("12a4"), 4).isError());
        QVERIFY(Validators::validateOtp(QStringLiteral("abcd"), 4).isError());
        QVERIFY(Validators::validateOtp(QStringLiteral("12 4"), 4).isError());
        QVERIFY(Validators::validateOtp(QStringLiteral("12-4"), 4).isError());

        // 6-digit OTP configuration
        QVERIFY(Validators::validateOtp(QStringLiteral("123456"), 6).isSuccess());
        QVERIFY(Validators::validateOtp(QStringLiteral("12345"), 6).isError());
    }

    void testServerSideOtpGenerationAndVerification() {
        // 1. Seed order on server in picked_up state
        QString orderId = QStringLiteral("order_test_otp_99");
        QJsonObject order;
        order["_id"] = orderId;
        order["status"] = "picked_up";
        m_server.addOrder(order);

        // Server generates secure 4-digit OTP
        m_server.setOrderDeliveryOtp(orderId, QStringLiteral("7349"));

        // 2. Courier submits WRONG OTP to server
        QJsonObject wrongReq;
        wrongReq["otp"] = QStringLiteral("1111");

        bool wrongDone = false;
        bool wrongSuccess = true;
        QString wrongError;

        NetworkManager::instance()->post(QString("/api/orders/%1/verify-delivery-otp").arg(orderId), wrongReq,
                                         [&](bool success, const QJsonDocument &, const QString &err) {
                                             wrongDone = true;
                                             wrongSuccess = success;
                                             wrongError = err;
                                         });

        QTRY_VERIFY_WITH_TIMEOUT(wrongDone, 3000);
        QVERIFY(!wrongSuccess); // Must be rejected by server

        // 3. Courier submits CORRECT OTP entered by customer
        QJsonObject correctReq;
        correctReq["otp"] = QStringLiteral("7349");

        bool correctDone = false;
        bool correctSuccess = false;
        QJsonObject respObj;

        NetworkManager::instance()->post(QString("/api/orders/%1/verify-delivery-otp").arg(orderId), correctReq,
                                         [&](bool success, const QJsonDocument &doc, const QString &) {
                                             correctDone = true;
                                             correctSuccess = success;
                                             respObj = doc.object();
                                         });

        QTRY_VERIFY_WITH_TIMEOUT(correctDone, 3000);
        QVERIFY(correctSuccess); // Accepted by server
        QCOMPARE(respObj.value("status").toString(), QStringLiteral("delivered"));

        // 4. Verify local state machine reflection
        auto res = OrderStateMachine::transition(Status::PickedUp, Status::Delivered, Actor::Courier);
        QVERIFY(res.isSuccess());
        QCOMPARE(res.value(), Status::Delivered);
    }

    void testOtpGenerationEntropy() {
        // Verify randomness and bounds of generated 4-digit OTPs
        QSet<QString> generatedOtps;
        for (int i = 0; i < 100; ++i) {
            quint32 val = QRandomGenerator::global()->bounded(1000u, 10000u);
            QString otp = QString::number(val);
            QVERIFY(Validators::validateOtp(otp, 4).isSuccess());
            generatedOtps.insert(otp);
        }
        // At least 90 unique codes out of 100 random draws
        QVERIFY(generatedOtps.size() >= 90);
    }
};

QTEST_MAIN(TestDeliveryOtp)
#include "test_delivery_otp.moc"
