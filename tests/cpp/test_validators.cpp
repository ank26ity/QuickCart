/**
 * @file test_validators.cpp
 * @brief Automated unit test suite for business rule and input validators.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include "../../core/validators.h"

class TestValidators : public QObject {
    Q_OBJECT

private slots:
    void testEmailValidation() {
        QVERIFY(Validators::validateEmail(QStringLiteral("customer@quickcart.com")).isSuccess());
        QVERIFY(Validators::validateEmail(QStringLiteral("rider.fast+1@delivery.co.in")).isSuccess());

        // Invalid emails
        QVERIFY(Validators::validateEmail(QStringLiteral("")).isError());
        QVERIFY(Validators::validateEmail(QStringLiteral("no_at_sign.com")).isError());
        QVERIFY(Validators::validateEmail(QStringLiteral("user@domain")).isError());
        QVERIFY(Validators::validateEmail(QStringLiteral("@emptyusername.com")).isError());
    }

    void testPhoneValidation() {
        QVERIFY(Validators::validatePhone(QStringLiteral("+919876543210")).isSuccess());
        QVERIFY(Validators::validatePhone(QStringLiteral("9876543210")).isSuccess());
        QVERIFY(Validators::validatePhone(QStringLiteral("+1-800-555-0199")).isSuccess());

        // Invalid phones
        QVERIFY(Validators::validatePhone(QStringLiteral("")).isError());
        QVERIFY(Validators::validatePhone(QStringLiteral("123")).isError());
        QVERIFY(Validators::validatePhone(QStringLiteral("abc-def-ghij")).isError());
    }

    void testPasswordValidation() {
        QVERIFY(Validators::validatePassword(QStringLiteral("SecurePass123")).isSuccess());
        QVERIFY(Validators::validatePassword(QStringLiteral("QuickCart@2026")).isSuccess());

        // Too short (<8)
        QVERIFY(Validators::validatePassword(QStringLiteral("Pass1")).isError());
        // Missing digit
        QVERIFY(Validators::validatePassword(QStringLiteral("PasswordOnly")).isError());
        // Missing uppercase
        QVERIFY(Validators::validatePassword(QStringLiteral("password123")).isError());
        // Missing lowercase
        QVERIFY(Validators::validatePassword(QStringLiteral("PASSWORD123")).isError());
    }

    void testPriceValidation() {
        // Integer paise validation tests
        QVERIFY(Validators::validatePricePaise(2999).isSuccess());
        QVERIFY(Validators::validatePricePaise(1).isSuccess());
        QVERIFY(Validators::validatePricePaise(50000000).isSuccess());

        QVERIFY(Validators::validatePricePaise(0).isError());
        QVERIFY(Validators::validatePricePaise(-100).isError());
        QVERIFY(Validators::validatePricePaise(50000001).isError());
    }

    void testQuantityValidation() {
        // Valid quantity within stock
        QVERIFY(Validators::validateQuantity(1, 10).isSuccess());
        QVERIFY(Validators::validateQuantity(5, 5).isSuccess());

        // Zero or negative quantity
        QVERIFY(Validators::validateQuantity(0, 10).isError());
        QVERIFY(Validators::validateQuantity(-2, 10).isError());

        // Out of stock
        QVERIFY(Validators::validateQuantity(1, 0).isError());

        // Quantity exceeding available inventory
        auto excessRes = Validators::validateQuantity(6, 5);
        QVERIFY(excessRes.isError());
        QCOMPARE(excessRes.error().category, ErrorCategory::Conflict);
    }

    void testCoordinatesValidation() {
        QVERIFY(Validators::validateCoordinates(12.9716, 77.5946).isSuccess()); // Bangalore
        QVERIFY(Validators::validateCoordinates(-90.0, 0.0).isSuccess());
        QVERIFY(Validators::validateCoordinates(0.0, 180.0).isSuccess());

        QVERIFY(Validators::validateCoordinates(91.0, 77.0).isError());
        QVERIFY(Validators::validateCoordinates(12.0, 185.0).isError());
        QVERIFY(Validators::validateCoordinates(-95.0, -200.0).isError());
    }

    void testHaversineDistanceAndRadiusBoundary() {
        // Center: 12.9715987, 77.5945627 (MG Road, Bangalore)
        double centerLat = 12.9715987;
        double centerLng = 77.5945627;

        // Point ~1.2 km away
        double nearLat = 12.9800000;
        double nearLng = 77.6000000;
        double dNear = Validators::calculateHaversineDistanceKm(centerLat, centerLng, nearLat, nearLng);
        QVERIFY(dNear > 1.0 && dNear < 1.6);

        auto nearCheck = Validators::validateWithinDeliveryRadius(centerLat, centerLng, nearLat, nearLng, 3.0);
        QVERIFY(nearCheck.isSuccess());

        // Point ~15 km away (Electronic City)
        double farLat = 12.8399;
        double farLng = 77.6770;
        double dFar = Validators::calculateHaversineDistanceKm(centerLat, centerLng, farLat, farLng);
        QVERIFY(dFar > 12.0);

        auto farCheck = Validators::validateWithinDeliveryRadius(centerLat, centerLng, farLat, farLng, 3.0);
        QVERIFY(farCheck.isError());
        QCOMPARE(farCheck.error().category, ErrorCategory::Validation);
    }

    void testOtpValidation() {
        QVERIFY(Validators::validateOtp(QStringLiteral("1234"), 4).isSuccess());
        QVERIFY(Validators::validateOtp(QStringLiteral("839210"), 6).isSuccess());

        QVERIFY(Validators::validateOtp(QStringLiteral("123"), 4).isError());
        QVERIFY(Validators::validateOtp(QStringLiteral("12a4"), 4).isError());
        QVERIFY(Validators::validateOtp(QStringLiteral(""), 4).isError());
    }

    void testOpeningHoursValidation() {
        // Open 9:00 AM (540m) to 10:00 PM (1320m)
        QVERIFY(Validators::validateOpeningHours(540, 1320).isSuccess());

        // Open after close
        QVERIFY(Validators::validateOpeningHours(1320, 540).isError());
        // Invalid bounds
        QVERIFY(Validators::validateOpeningHours(-10, 100).isError());
        QVERIFY(Validators::validateOpeningHours(500, 1500).isError());
    }
};

QTEST_MAIN(TestValidators)
#include "test_validators.moc"
