#include <QtTest/QtTest>
#include "core/moneyformatter.h"
#include "core/appconfig.h"

class TestMoneyFormatter : public QObject {
    Q_OBJECT

private slots:
    void testZeroAndSmallPaise() {
        QCOMPARE(MoneyFormatter::formatPaise(0, true), QStringLiteral("₹0.00"));
        QCOMPARE(MoneyFormatter::formatPaise(0, false), QStringLiteral("0.00"));

        QCOMPARE(MoneyFormatter::formatPaise(5, true), QStringLiteral("₹0.05"));
        QCOMPARE(MoneyFormatter::formatPaise(99, true), QStringLiteral("₹0.99"));
        QCOMPARE(MoneyFormatter::formatPaise(100, true), QStringLiteral("₹1.00"));
    }

    void testDeliveryConstants() {
        // Base delivery fee: 4900 paise (₹49.00)
        QCOMPARE(MoneyFormatter::formatPaise(4900, true), QStringLiteral("₹49.00"));
        QCOMPARE(MoneyFormatter::formatPaise(4900, false), QStringLiteral("49.00"));

        // Free delivery threshold: 49900 paise (₹499.00)
        QCOMPARE(MoneyFormatter::formatPaise(49900, true), QStringLiteral("₹499.00"));
        QCOMPARE(MoneyFormatter::formatPaise(49900, false), QStringLiteral("499.00"));
    }

    void testIndianNumberSystemGrouping() {
        // Thousands (3 digits grouped)
        QCOMPARE(MoneyFormatter::formatPaise(100000, true), QStringLiteral("₹1,000.00"));
        QCOMPARE(MoneyFormatter::formatPaise(1000000, true), QStringLiteral("₹10,000.00"));

        // 1 Lakh (1,00,000)
        QCOMPARE(MoneyFormatter::formatPaise(10000000, true), QStringLiteral("₹1,00,000.00"));

        // 10 Lakhs (10,00,000)
        QCOMPARE(MoneyFormatter::formatPaise(100000000, true), QStringLiteral("₹10,00,000.00"));

        // Complex multi-tier grouping (Lakhs and thousands)
        // 123456789 paise = ₹12,34,567.89
        QCOMPARE(MoneyFormatter::formatPaise(123456789, true), QStringLiteral("₹12,34,567.89"));
        QCOMPARE(MoneyFormatter::formatPaise(123456789, false), QStringLiteral("12,34,567.89"));

        // 1 Crore (1,00,00,000)
        QCOMPARE(MoneyFormatter::formatPaise(1000000000LL, true), QStringLiteral("₹1,00,00,000.00"));

        // 10 Crores (10,00,00,000)
        QCOMPARE(MoneyFormatter::formatPaise(10000000000LL, true), QStringLiteral("₹10,00,00,000.00"));
    }

    void testNegativeAmounts() {
        QCOMPARE(MoneyFormatter::formatPaise(-4900, true), QStringLiteral("-₹49.00"));
        QCOMPARE(MoneyFormatter::formatPaise(-4900, false), QStringLiteral("-49.00"));
        QCOMPARE(MoneyFormatter::formatPaise(-123456789, true), QStringLiteral("-₹12,34,567.89"));
    }

    void testAppConfigParity() {
        AppConfig config;
        QCOMPARE(config.formatPaise(4900), QStringLiteral("₹49.00"));
        QCOMPARE(config.formatMoney(4900), QStringLiteral("49.00"));
        QCOMPARE(config.formatPaise(123456789), QStringLiteral("₹12,34,567.89"));
        QCOMPARE(config.formatMoney(123456789), QStringLiteral("12,34,567.89"));
    }
};

QTEST_MAIN(TestMoneyFormatter)
#include "test_money_formatter.moc"
