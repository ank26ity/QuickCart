/**
 * @file test_customer_cart_flow.cpp
 * @brief Integration tests for Customer Flow (4b): shop discovery 3km boundary, single-store rule, stock limits, checkout.
 * @layer Tests / Integration (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include "../../models/cartmanager.h"
#include "../../models/shopmodel.h"
#include "../../api/networkmanager.h"
#include "../../core/appconfig.h"
#include "../tools/mockapiserver.h"

class TestCustomerCartFlow : public QObject
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

    void init()
    {
        CartManager::instance()->resetForTesting();
        m_server.resetData();
    }

    void testShopDiscovery3KmBoundary()
    {
        ShopModel shops;
        QSignalSpy loadSpy(&shops, &ShopModel::shopsLoaded);

        // Fetch shops near Indiranagar, Bangalore (12.9716, 77.5946)
        shops.fetchShops("all", 12.9716, 77.5946);

        QVERIFY(loadSpy.wait(3000));
        // Expect 2 shops within 3km (Fresh Mart & Corner Pharmacy); Faraway Hypermarket (15km) must be filtered out!
        QCOMPARE(shops.count(), 2);

        QVariantMap s1 = shops.getShopAt(0);
        QVariantMap s2 = shops.getShopAt(1);
        QVERIFY(s1.value("name").toString() == "Fresh Mart Daily" || s2.value("name").toString() == "Fresh Mart Daily");
        QVERIFY(s1.value("name").toString() == "Corner Pharmacy" || s2.value("name").toString() == "Corner Pharmacy");
    }

    void testCategoryFilter()
    {
        ShopModel shops;
        QSignalSpy loadSpy(&shops, &ShopModel::shopsLoaded);

        shops.fetchShops("pharmacy", 12.9716, 77.5946);
        QVERIFY(loadSpy.wait(3000));

        QCOMPARE(shops.count(), 1);
        QCOMPARE(shops.getShopAt(0).value("name").toString(), QStringLiteral("Corner Pharmacy"));
    }

    void testSingleStoreRuleEnforcement()
    {
        CartManager *cart = CartManager::instance();
        QSignalSpy mismatchSpy(cart, &CartManager::promptStoreMismatch);

        // Add product from Shop 1
        QVariantMap prod1;
        prod1["id"] = "prod_1";
        prod1["shopId"] = "shop_1";
        prod1["name"] = "Apples";
        prod1["price"] = 120.0;
        QVERIFY(cart->addItem(prod1, 10));
        QCOMPARE(cart->itemCount(), 1);
        QCOMPARE(cart->shopId(), QStringLiteral("shop_1"));

        // Attempt adding product from Shop 2 (Different Store)
        QVariantMap prod2;
        prod2["id"] = "prod_3";
        prod2["shopId"] = "shop_2";
        prod2["name"] = "First Aid Kit";
        prod2["price"] = 250.0;

        bool added = cart->addItem(prod2, 5);
        QVERIFY(!added);
        QCOMPARE(mismatchSpy.count(), 1);
        QCOMPARE(cart->itemCount(), 1); // Cart unchanged
    }

    void testStockLimitsEnforcement()
    {
        CartManager *cart = CartManager::instance();

        QVariantMap prod;
        prod["id"] = "prod_2";
        prod["shopId"] = "shop_1";
        prod["name"] = "Almond Milk";
        prod["price"] = 80.0;

        // Stock limit is 2
        QVERIFY(cart->addItem(prod, 2)); // Qty 1
        QVERIFY(cart->addItem(prod, 2)); // Qty 2
        QCOMPARE(cart->itemCount(), 2);

        // Exceeding stock limit
        QVERIFY(!cart->addItem(prod, 2)); // Should reject
        QCOMPARE(cart->itemCount(), 2);
        QVERIFY(!cart->errorMessage().isEmpty());
    }

    void testDeliveryFeeCalculationAndOrderPlacement()
    {
        CartManager *cart = CartManager::instance();
        QCOMPARE(cart->deliveryFee(), 0.0);
        QCOMPARE(cart->total(), 0.0);

        QVariantMap prod;
        prod["id"] = "prod_1";
        prod["shopId"] = "shop_1";
        prod["name"] = "Apples";
        prod["price"] = 120.0;
        cart->addItem(prod, 10);

        QCOMPARE(cart->subtotal(), 120.0);
        QCOMPARE(cart->deliveryFee(), 50.0);
        QCOMPARE(cart->total(), 170.0);

        // Verify integer paise calculations
        QCOMPARE(cart->subtotalPaise(), 12000LL);
        QCOMPARE(cart->deliveryFeePaise(), 5000LL);
        QCOMPARE(cart->totalPaise(), 17000LL);

        // Place order
        QSignalSpy orderPlacedSpy(cart, &CartManager::orderPlacedSuccess);
        cart->placeOrder("123 MG Road, Apt 4B");

        QVERIFY(orderPlacedSpy.wait(3000));
        QVERIFY(!orderPlacedSpy.first().at(0).toString().isEmpty());
        // Cart must be cleared upon successful placement
        QCOMPARE(cart->itemCount(), 0);
        QCOMPARE(cart->total(), 0.0);
    }

    void testCartItemModificationsAndClear()
    {
        CartManager *cart = CartManager::instance();
        cart->clearCart();
        QCOMPARE(cart->itemCount(), 0);

        QVariantMap p1;
        p1["id"] = "prod_1";
        p1["shopId"] = "shop_1";
        p1["name"] = "Apples";
        p1["price"] = 120.0;
        cart->addItem(p1, 10);

        QVariantMap p2;
        p2["id"] = "prod_2";
        p2["shopId"] = "shop_1";
        p2["name"] = "Milk";
        p2["price"] = 80.0;
        cart->addItem(p2, 5);

        QCOMPARE(cart->itemCount(), 2);
        QCOMPARE(cart->items().size(), 2);

        // Update quantity (+1)
        cart->updateQuantity("prod_1", 1);
        QCOMPARE(cart->itemCount(), 3); // 2 + 1

        // Decrementing until 0 removes item
        cart->updateQuantity("prod_1", -1);
        cart->updateQuantity("prod_1", -1);
        QCOMPARE(cart->items().size(), 1);

        // Decrementing remaining item to 0 clears it
        cart->updateQuantity("prod_2", -1);
        QCOMPARE(cart->itemCount(), 0);
        QCOMPARE(cart->items().size(), 0);

        // Attempting to place order with empty cart fails gracefully
        cart->placeOrder("Some address");
        QCOMPARE(cart->errorMessage(), QStringLiteral("Cart is empty."));

        cart->clearCart();
        QCOMPARE(cart->itemCount(), 0);
    }

    void testServerConfigAndPaiseRecomputation()
    {
        // 1. Fetch server delivery fee and threshold config
        AppConfig *config = AppConfig::instance();
        QSignalSpy configSpy(config, &AppConfig::serverConfigFetched);
        config->fetchServerConfig();

        QVERIFY(configSpy.wait(3000));
        QCOMPARE(config->baseDeliveryFeePaise(), 4900LL);
        QCOMPARE(config->freeDeliveryThresholdPaise(), 49900LL);

        // 2. Add items using integer paise
        CartManager *cart = CartManager::instance();
        cart->clearCart();

        QVariantMap p1;
        p1["id"] = "prod_1";
        p1["shopId"] = "shop_1";
        p1["name"] = "Premium Basmati Rice";
        p1["pricePaise"] = 15000LL; // ₹150.00
        p1["price"] = 150.0;
        cart->addItem(p1, 5); // qty 1

        cart->addItem(p1, 5); // qty 2 -> subtotal = 30000 paise (₹300.00)

        // 3. Request server-side recomputation of totals
        QSignalSpy serverCalcSpy(cart, &CartManager::serverCalculated);
        cart->syncServerCalculation();

        QVERIFY(serverCalcSpy.wait(3000));

        // Subtotal < 49900 threshold -> 4900 paise delivery fee applied by server
        QCOMPARE(cart->subtotalPaise(), 30000LL);
        QCOMPARE(cart->deliveryFeePaise(), 4900LL);
        QCOMPARE(cart->totalPaise(), 34900LL);

        // Verify currency formatting string
        QCOMPARE(config->formatMoney(cart->totalPaise()), QStringLiteral("349.00"));
    }
};

QTEST_MAIN(TestCustomerCartFlow)
#include "test_customer_cart_flow.moc"
