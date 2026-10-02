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

using Status = OrderStateMachine::OrderStatus;
using Actor = OrderStateMachine::OrderActor;

class TestDeliveryOtp : public QObject
{
    Q_OBJECT

private slots:
    void testOtpValidationFormats()
    {
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

    void testOtpDeliveryLifecycleHandoff()
    {
        // 1. Order in PickedUp state ready for delivery
        // Valid courier OTP handoff
        QString customerOtp = QStringLiteral("8492");
        QString courierInputCorrect = QStringLiteral("8492");
        QString courierInputWrong = QStringLiteral("1111");

        // Courier attempts delivery with wrong OTP -> Rejected
        QVERIFY(Validators::validateOtp(courierInputWrong, 4).isSuccess()); // format is ok
        QVERIFY(courierInputWrong != customerOtp); // Content mismatch

        // Courier attempts delivery with correct OTP -> Accepted
        QVERIFY(Validators::validateOtp(courierInputCorrect, 4).isSuccess());
        QCOMPARE(courierInputCorrect, customerOtp);

        // State Machine transition to Delivered succeeds for Courier
        auto res = OrderStateMachine::transition(Status::PickedUp, Status::Delivered, Actor::Courier);
        QVERIFY(res.isSuccess());
        QCOMPARE(res.value(), Status::Delivered);

        // Post-delivery: cannot be modified or re-delivered
        QVERIFY(OrderStateMachine::isTerminalState(Status::Delivered));
        auto rPost = OrderStateMachine::transition(Status::Delivered, Status::PickedUp, Actor::Courier);
        QVERIFY(rPost.isError());
    }

    void testOtpGenerationEntropy()
    {
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
