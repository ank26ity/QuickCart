/**
 * @file test_unit_models_coverage.cpp
 * @brief Comprehensive branch and line coverage test suite for Core & Domain Models:
 * CartManager, AuthService, ApiClient, SecureStorage, OrderModel, ShopModel, ProductModel, NetworkManager.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

#include "../../models/cartmanager.h"
#include "../../models/authservice.h"
#include "../../models/ordermodel.h"
#include "../../models/shopmodel.h"
#include "../../models/productmodel.h"
#include "../../api/apiclient.h"
#include "../../api/networkmanager.h"
#include "../../security/securestorage.h"
#include "../../core/appconfig.h"
#include "../tools/mockapiserver.h"

class DummyInterceptor : public INetworkInterceptor {
public:
    int interceptedCount{0};
    void onRequest(QNetworkRequest &request, const QByteArray &body) override {
        Q_UNUSED(request);
        Q_UNUSED(body);
        interceptedCount++;
    }
    void onResponse(QNetworkReply *reply, const QByteArray &responseBody) override {
        Q_UNUSED(reply);
        Q_UNUSED(responseBody);
    }
};

class TestUnitModelsCoverage : public QObject {
    Q_OBJECT

private:
    MockApiServer m_server;

private slots:
    void initTestCase() {
        QVERIFY(m_server.start());
        NetworkManager::instance()->setBaseUrl(m_server.url());
    }

    void cleanupTestCase() { m_server.stop(); }

    void init() { m_server.resetData(); }

    // ── 1. CartManager Deep Coverage ─────────────────────────────────────────
    void testCartManagerFullCoverage() {
        CartManager *cart = CartManager::instance();
        cart->resetForTesting();

        // Accessors on empty cart
        QCOMPARE(cart->itemCount(), 0);
        QCOMPARE(cart->subtotalPaise(), 0);
        QCOMPARE(cart->deliveryFeePaise(), 0);
        QCOMPARE(cart->totalPaise(), 0);
        QCOMPARE(cart->isSubmitting(), false);
        QVERIFY(cart->items().isEmpty());
        QVERIFY(cart->shopId().isEmpty());

        // Add invalid item
        QVariantMap invalidItem;
        QVERIFY(!cart->addItem(invalidItem, 1));
        QVERIFY(!cart->addItem(invalidItem, -1));

        // Add valid item 1
        QVariantMap item1;
        item1[QStringLiteral("id")] = QStringLiteral("p1");
        item1[QStringLiteral("shopId")] = QStringLiteral("shop_1");
        item1[QStringLiteral("name")] = QStringLiteral("Organic Apples");
        item1[QStringLiteral("pricePaise")] = 15000;
        item1[QStringLiteral("quantity")] = 10;

        QVERIFY(cart->addItem(item1, 5));
        QCOMPARE(cart->itemCount(), 1);
        QCOMPARE(cart->shopId(), QStringLiteral("shop_1"));
        QCOMPARE(cart->subtotalPaise(), 15000);
        QVERIFY(cart->deliveryFeePaise() > 0);
        QCOMPARE(cart->totalPaise(), cart->subtotalPaise() + cart->deliveryFeePaise());

        // Update quantity
        cart->updateQuantity(QStringLiteral("p1"), 1); // 1 + 1 = 2
        QCOMPARE(cart->itemCount(), 2);
        cart->updateQuantity(QStringLiteral("p1"), -1); // 2 - 1 = 1
        QCOMPARE(cart->itemCount(), 1);
        cart->updateQuantity(QStringLiteral("p1"), -1); // Decrementing to 0 removes item
        QCOMPARE(cart->itemCount(), 0);

        // Add again and test clearCart
        QVERIFY(cart->addItem(item1, 5));
        cart->clearCart();
        QCOMPARE(cart->itemCount(), 0);
        QVERIFY(cart->shopId().isEmpty());

        // Distance & server calculation sync
        cart->setDeliveryDistanceKm(5.5);
        QCOMPARE(cart->deliveryDistanceKm(), 5.5);

        QVERIFY(cart->addItem(item1, 5));
        cart->syncServerCalculation();

        // Place order with invalid address fails
        QSignalSpy errSpy(cart, &CartManager::errorChanged);
        cart->placeOrder(QStringLiteral(""));
        QCOMPARE(errSpy.count(), 1);

        // Clear and place order on empty cart fails
        cart->clearCart();
        cart->placeOrder(QStringLiteral("123 MG Road"));
        QCOMPARE(errSpy.count(), 3);
    }

    // ── 2. ProductModel Deep Coverage ────────────────────────────────────────
    void testProductModelFullCoverage() {
        ProductModel model;
        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(model.isLoading(), false);
        QVERIFY(!model.roleNames().isEmpty());

        // Invalid data access
        QCOMPARE(model.data(QModelIndex(), ProductModel::NameRole), QVariant());
        QCOMPARE(model.data(model.index(-1, 0), ProductModel::NameRole), QVariant());
        QCOMPARE(model.data(model.index(10, 0), ProductModel::NameRole), QVariant());

        // Populate
        QJsonArray arr;
        QJsonObject p;
        p[QStringLiteral("id")] = QStringLiteral("p100");
        p[QStringLiteral("shop_id")] = QStringLiteral("s1");
        p[QStringLiteral("name")] = QStringLiteral("Fresh Milk");
        p[QStringLiteral("description")] = QStringLiteral("1 Liter Pack");
        p[QStringLiteral("price_paise")] = 6500;
        p[QStringLiteral("quantity")] = 8;
        p[QStringLiteral("image")] = QStringLiteral("milk.png");
        arr.append(p);

        model.populateFromJson(arr);
        QCOMPARE(model.count(), 1);

        QModelIndex idx = model.index(0, 0);
        QCOMPARE(model.data(idx, ProductModel::IdRole).toString(), QStringLiteral("p100"));
        QCOMPARE(model.data(idx, ProductModel::ShopIdRole).toString(), QStringLiteral("s1"));
        QCOMPARE(model.data(idx, ProductModel::NameRole).toString(), QStringLiteral("Fresh Milk"));
        QCOMPARE(model.data(idx, ProductModel::DescriptionRole).toString(), QStringLiteral("1 Liter Pack"));
        QCOMPARE(model.data(idx, ProductModel::PriceRole).toLongLong(), 6500LL);
        QCOMPARE(model.data(idx, ProductModel::QuantityRole).toInt(), 8);
        QCOMPARE(model.data(idx, ProductModel::ImageRole).toString(), QStringLiteral("milk.png"));
        QCOMPARE(model.data(idx, ProductModel::IsLowStockRole).toBool(), false);
        QCOMPARE(model.data(idx, ProductModel::IsOutOfStockRole).toBool(), false);
        QCOMPARE(model.data(idx, 99999), QVariant()); // Invalid role

        // getProductAt
        QVERIFY(model.getProductAt(-1).isEmpty());
        QVERIFY(model.getProductAt(5).isEmpty());
        QCOMPARE(model.getProductAt(0)[QStringLiteral("name")].toString(), QStringLiteral("Fresh Milk"));

        // Fetch products for shop via API
        model.fetchProductsForShop(QStringLiteral("shop_1"));
        QTRY_VERIFY_WITH_TIMEOUT(!model.isLoading(), 2000);

        // Product CRUD
        QJsonObject newP;
        newP[QStringLiteral("id")] = QStringLiteral("prod_test_crud");
        newP[QStringLiteral("name")] = QStringLiteral("Organic Bread");
        newP[QStringLiteral("price_paise")] = 4500;
        newP[QStringLiteral("quantity")] = 12;
        model.addProduct(newP);
        QVERIFY(model.count() >= 2);

        QJsonObject updP;
        updP[QStringLiteral("price_paise")] = 5000;
        QVERIFY(model.updateProduct(QStringLiteral("prod_test_crud"), updP));
        QVERIFY(!model.updateProduct(QStringLiteral("nonexistent"), updP));

        QVERIFY(model.deleteProduct(QStringLiteral("prod_test_crud")));
        QVERIFY(!model.deleteProduct(QStringLiteral("nonexistent")));
    }

    // ── 3. ShopModel Deep Coverage ───────────────────────────────────────────
    void testShopModelFullCoverage() {
        ShopModel model;
        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(model.isLoading(), false);
        QVERIFY(!model.roleNames().isEmpty());

        // Invalid indices
        QCOMPARE(model.data(QModelIndex(), ShopModel::NameRole), QVariant());
        QCOMPARE(model.data(model.index(-1, 0), ShopModel::NameRole), QVariant());
        QCOMPARE(model.data(model.index(10, 0), ShopModel::NameRole), QVariant());

        QJsonArray arr;
        QJsonObject s;
        s[QStringLiteral("id")] = QStringLiteral("s100");
        s[QStringLiteral("name")] = QStringLiteral("Apollo Pharmacy");
        s[QStringLiteral("category")] = QStringLiteral("pharmacy");
        s[QStringLiteral("description")] = QStringLiteral("24/7 Chemist");
        s[QStringLiteral("image")] = QStringLiteral("apollo.png");
        s[QStringLiteral("rating")] = 4.8;
        s[QStringLiteral("address")] = QStringLiteral("MG Road, Bangalore");
        s[QStringLiteral("owner_id")] = QStringLiteral("owner_1");
        s[QStringLiteral("lat")] = 12.9716;
        s[QStringLiteral("lng")] = 77.5946;
        arr.append(s);

        model.populateFromJson(arr, 12.9716, 77.5946);
        QCOMPARE(model.count(), 1);

        QModelIndex idx = model.index(0, 0);
        QCOMPARE(model.data(idx, ShopModel::IdRole).toString(), QStringLiteral("s100"));
        QCOMPARE(model.data(idx, ShopModel::NameRole).toString(), QStringLiteral("Apollo Pharmacy"));
        QCOMPARE(model.data(idx, ShopModel::CategoryRole).toString(), QStringLiteral("pharmacy"));
        QCOMPARE(model.data(idx, ShopModel::DescriptionRole).toString(), QStringLiteral("24/7 Chemist"));
        QCOMPARE(model.data(idx, ShopModel::ImageRole).toString(), QStringLiteral("apollo.png"));
        QCOMPARE(model.data(idx, ShopModel::RatingRole).toDouble(), 4.8);
        QCOMPARE(model.data(idx, ShopModel::AddressRole).toString(), QStringLiteral("MG Road, Bangalore"));
        QCOMPARE(model.data(idx, ShopModel::OwnerIdRole).toString(), QStringLiteral("owner_1"));
        QCOMPARE(model.data(idx, ShopModel::LatRole).toDouble(), 12.9716);
        QCOMPARE(model.data(idx, ShopModel::LngRole).toDouble(), 77.5946);
        QCOMPARE(model.data(idx, 99999), QVariant());

        QVERIFY(model.getShopAt(-1).isEmpty());
        QVERIFY(model.getShopAt(10).isEmpty());
        QCOMPARE(model.getShopAt(0)[QStringLiteral("name")].toString(), QStringLiteral("Apollo Pharmacy"));
    }

    // ── 4. OrderModel Deep Coverage ──────────────────────────────────────────
    void testOrderModelFullCoverage() {
        OrderModel model;
        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(model.isLoading(), false);
        QCOMPARE(model.errorMessage(), QString());
        QVERIFY(!model.roleNames().isEmpty());

        QCOMPARE(model.data(QModelIndex(), OrderModel::StatusRole), QVariant());
        QCOMPARE(model.data(model.index(-1, 0), OrderModel::StatusRole), QVariant());
        QCOMPARE(model.data(model.index(5, 0), OrderModel::StatusRole), QVariant());

        QJsonArray arr;
        QJsonObject o;
        o[QStringLiteral("id")] = QStringLiteral("ord_999");
        o[QStringLiteral("customer_id")] = QStringLiteral("cust_1");
        o[QStringLiteral("shop_id")] = QStringLiteral("shop_1");
        o[QStringLiteral("shop_name")] = QStringLiteral("Fresh Mart");
        o[QStringLiteral("delivery_boy_id")] = QStringLiteral("rider_1");
        o[QStringLiteral("address")] = QStringLiteral("MG Road");
        o[QStringLiteral("status")] = QStringLiteral("pending");
        o[QStringLiteral("subtotal_paise")] = 20000;
        o[QStringLiteral("delivery_fee_paise")] = 4000;
        o[QStringLiteral("total_paise")] = 24000;
        o[QStringLiteral("created_at")] = QStringLiteral("2026-10-02T12:00:00Z");
        o[QStringLiteral("items")] = QJsonArray();
        arr.append(o);
        m_server.addOrder(o);

        model.populateFromJson(arr);
        QCOMPARE(model.count(), 1);

        QModelIndex idx = model.index(0, 0);
        QCOMPARE(model.data(idx, OrderModel::IdRole).toString(), QStringLiteral("ord_999"));
        QCOMPARE(model.data(idx, OrderModel::CustomerIdRole).toString(), QStringLiteral("cust_1"));
        QCOMPARE(model.data(idx, OrderModel::ShopIdRole).toString(), QStringLiteral("shop_1"));
        QCOMPARE(model.data(idx, OrderModel::ShopNameRole).toString(), QStringLiteral("Fresh Mart"));
        QCOMPARE(model.data(idx, OrderModel::DeliveryBoyIdRole).toString(), QStringLiteral("rider_1"));
        QCOMPARE(model.data(idx, OrderModel::AddressRole).toString(), QStringLiteral("MG Road"));
        QCOMPARE(model.data(idx, OrderModel::StatusRole).toString(), QStringLiteral("pending"));
        QCOMPARE(model.data(idx, OrderModel::SubtotalRole).toLongLong(), 20000LL);
        QCOMPARE(model.data(idx, OrderModel::DeliveryFeeRole).toLongLong(), 4000LL);
        QCOMPARE(model.data(idx, OrderModel::TotalRole).toLongLong(), 24000LL);
        QCOMPARE(model.data(idx, OrderModel::CreatedAtRole).toString(), QStringLiteral("2026-10-02T12:00:00Z"));
        QCOMPARE(model.data(idx, 99999), QVariant());

        QVERIFY(model.getOrderAt(-1).isEmpty());
        QVERIFY(model.getOrderAt(5).isEmpty());
        QCOMPARE(model.getOrderAt(0)[QStringLiteral("id")].toString(), QStringLiteral("ord_999"));

        // Polling controls
        model.startPolling(500);
        model.stopPolling();

        // Assign rider
        model.assignRiderToOrder(QStringLiteral("ord_999"), QStringLiteral("rider_2"));
        QTest::qWait(300);

        // Valid status transition
        model.updateOrderStatus(QStringLiteral("ord_999"), QStringLiteral("preparing"), QStringLiteral("merchant"));
        QTest::qWait(200);

        // Invalid transition sets error
        model.updateOrderStatus(QStringLiteral("ord_999"), QStringLiteral("delivered"), QStringLiteral("merchant"));
        QVERIFY(!model.errorMessage().isEmpty());

        // Direct fetch
        model.fetchOrders(true);
        QTest::qWait(200);
    }

    // ── 5. ApiClient Deep Coverage ───────────────────────────────────────────
    void testApiClientErrorMappings() {
        ApiClient *client = ApiClient::instance();
        client->resetForTesting();

        // Verify all HTTP status mappings
        QCOMPARE(ApiClient::mapHttpStatusToError(400, "Bad Request").category, ErrorCategory::Validation);
        QCOMPARE(ApiClient::mapHttpStatusToError(401, "Unauthorized").category, ErrorCategory::Authentication);
        QCOMPARE(ApiClient::mapHttpStatusToError(403, "Forbidden").category, ErrorCategory::Authorization);
        QCOMPARE(ApiClient::mapHttpStatusToError(404, "Not Found").category, ErrorCategory::NotFound);
        QCOMPARE(ApiClient::mapHttpStatusToError(409, "Conflict").category, ErrorCategory::Conflict);
        QCOMPARE(ApiClient::mapHttpStatusToError(429, "Rate limited").category, ErrorCategory::RateLimited);
        QCOMPARE(ApiClient::mapHttpStatusToError(500, "Internal error").category, ErrorCategory::Server);
        QCOMPARE(ApiClient::mapHttpStatusToError(503, "Service Unavailable").category, ErrorCategory::Server);
        QCOMPARE(ApiClient::mapHttpStatusToError(599, "Other error").category, ErrorCategory::Server);

        // Idempotency Key
        QString key1 = ApiClient::generateIdempotencyKey();
        QString key2 = ApiClient::generateIdempotencyKey();
        QVERIFY(!key1.isEmpty());
        QVERIFY(!key2.isEmpty());
        QVERIFY(key1 != key2);

        // Fetch Shops typed API
        bool shopsDone = false;
        client->fetchShops(12.9716, 77.5946, [&](const Result<QList<ShopDto>> &res) {
            QVERIFY(res.isSuccess());
            shopsDone = true;
        });
        QTRY_VERIFY_WITH_TIMEOUT(shopsDone, 2000);

        // Create Order typed API
        OrderCreateDto dto;
        dto.shopId = QStringLiteral("shop_1");
        dto.deliveryAddress = QStringLiteral("Indiranagar, Bangalore");
        dto.subtotalPaise = 30000;
        dto.deliveryFeePaise = 5000;
        dto.totalPaise = 35000;
        dto.idempotencyKey = ApiClient::generateIdempotencyKey();

        bool orderDone = false;
        QString createdOrderId;
        client->createOrder(dto, [&](const Result<OrderDto> &res) {
            QVERIFY(res.isSuccess());
            createdOrderId = res.value().id;
            orderDone = true;
        });
        QTRY_VERIFY_WITH_TIMEOUT(orderDone, 2000);
        QVERIFY(!createdOrderId.isEmpty());

        // Update Order Status typed API
        bool updateDone = false;
        client->updateOrderStatus(createdOrderId, QStringLiteral("preparing"), [&](const Result<void> &res) {
            QVERIFY(res.isSuccess());
            updateDone = true;
        });
        QTRY_VERIFY_WITH_TIMEOUT(updateDone, 2000);
    }

    // ── 6. AuthService Deep Coverage ─────────────────────────────────────────
    void testAuthServiceFullCoverage() {
        AuthService *auth = AuthService::instance();
        auth->resetForTesting();

        // Phone login flow
        auth->login(QStringLiteral("+919876543210"), QStringLiteral("Password123"));
        QTest::qWait(200);

        // OTP flow
        QCOMPARE(auth->isOtpSent(), false);
        auth->requestOtp(QStringLiteral("+919876543210"));
        QTest::qWait(200);
        auth->verifyOtp(QStringLiteral("+919876543210"), QStringLiteral("123456"));
        QTest::qWait(200);

        // Signup customer
        QVariantMap signupData;
        signupData[QStringLiteral("name")] = QStringLiteral("Kavya Patel");
        signupData[QStringLiteral("email")] = QStringLiteral("kavya@quickcart.com");
        signupData[QStringLiteral("password")] = QStringLiteral("Password123");
        signupData[QStringLiteral("role")] = QStringLiteral("customer");
        auth->signup(signupData);
        QTest::qWait(200);

        // Accessors
        QCOMPARE(auth->userPhone(), QString());
        QCOMPARE(auth->userShopId(), QString());
        QVERIFY(auth->currentUserData().isEmpty() || auth->currentUserData().contains(QStringLiteral("name")));

        // Courier compliance submission
        auth->submitCourierCompliance(QStringLiteral(""), QStringLiteral(""));
        QVERIFY(!auth->errorMessage().isEmpty());

        auth->submitCourierCompliance(QStringLiteral("DL-1234567890"), QStringLiteral("KA-01-AB-1234"));
        QTest::qWait(200);

        // Check session restoration from SecureStorage
        QJsonObject sessionUser;
        sessionUser[QStringLiteral("_id")] = QStringLiteral("user_sess_1");
        sessionUser[QStringLiteral("name")] = QStringLiteral("Session User");
        sessionUser[QStringLiteral("role")] = QStringLiteral("shopkeeper");
        sessionUser[QStringLiteral("shopId")] = QStringLiteral("shop_1");
        SecureStorage::instance()->saveSecret(
            QStringLiteral("session_user_data"),
            QString::fromUtf8(QJsonDocument(sessionUser).toJson(QJsonDocument::Compact)));
        auth->checkSession();
        QCOMPARE(auth->userName(), QStringLiteral("Session User"));
        QCOMPARE(auth->userRole(), QStringLiteral("shopkeeper"));

        // Signup validation checks
        QVariantMap invalidSignup;
        invalidSignup[QStringLiteral("email")] = QStringLiteral("invalid_email");
        invalidSignup[QStringLiteral("password")] = QStringLiteral("short");
        auth->signup(invalidSignup);
        QVERIFY(!auth->errorMessage().isEmpty());
    }

    // ── 7. NetworkManager Deep Coverage ──────────────────────────────────────
    void testNetworkManagerFullCoverage() {
        NetworkManager *nm = NetworkManager::instance();
        nm->resetForTesting();

        QCOMPARE(nm->isOnline(), true);
        QCOMPARE(nm->baseUrl(), m_server.url());

        // Interceptor
        auto interceptor = std::make_shared<DummyInterceptor>();
        nm->addInterceptor(interceptor);

        // Legacy GET
        bool legacyGetDone = false;
        nm->get(QStringLiteral("/api/health"), [&](bool ok, const QJsonDocument &doc, const QString &err) {
            Q_UNUSED(doc);
            Q_UNUSED(err);
            legacyGetDone = ok;
        });
        QTRY_VERIFY_WITH_TIMEOUT(legacyGetDone, 2000);
        QVERIFY(interceptor->interceptedCount >= 1);

        // Legacy POST
        bool legacyPostDone = false;
        QJsonObject b;
        b[QStringLiteral("test")] = true;
        nm->post(QStringLiteral("/api/health"), b, [&](bool ok, const QJsonDocument &doc, const QString &err) {
            Q_UNUSED(doc);
            Q_UNUSED(err);
            legacyPostDone = ok;
        });
        QTRY_VERIFY_WITH_TIMEOUT(legacyPostDone, 2000);

        // Legacy PATCH
        bool legacyPatchDone = false;
        nm->patch(QStringLiteral("/api/health"), b, [&](bool ok, const QJsonDocument &doc, const QString &err) {
            Q_UNUSED(doc);
            Q_UNUSED(err);
            legacyPatchDone = ok;
        });
        QTRY_VERIFY_WITH_TIMEOUT(legacyPatchDone, 2000);

        // Legacy DELETE
        bool legacyDeleteDone = false;
        nm->remove(QStringLiteral("/api/health"), [&](bool ok, const QJsonDocument &doc, const QString &err) {
            Q_UNUSED(doc);
            Q_UNUSED(err);
            legacyDeleteDone = ok;
        });
        QTRY_VERIFY_WITH_TIMEOUT(legacyDeleteDone, 2000);

        // Typed PATCH and DELETE
        bool typedPatchDone = false;
        nm->executePatch(QStringLiteral("/api/health"), b,
                         [&](const Result<QJsonDocument> &res) { typedPatchDone = res.isSuccess(); });
        QTRY_VERIFY_WITH_TIMEOUT(typedPatchDone, 2000);

        bool typedDeleteDone = false;
        nm->executeDelete(QStringLiteral("/api/health"),
                          [&](const Result<QJsonDocument> &res) { typedDeleteDone = res.isSuccess(); });
        QTRY_VERIFY_WITH_TIMEOUT(typedDeleteDone, 2000);

        // 401 token refresh loop
        SecureStorage::instance()->saveTokens(QStringLiteral("mock_old_access"), QStringLiteral("mock_valid_refresh"));
        m_server.setFailNextRequests(1, 401);

        bool refreshedReqDone = false;
        nm->executeGet(QStringLiteral("/api/health"),
                       [&](const Result<QJsonDocument> &res) { refreshedReqDone = res.isSuccess(); });
        QTRY_VERIFY_WITH_TIMEOUT(refreshedReqDone, 3000);

        // retryRequest
        bool retryDone = false;
        nm->retryRequest(QStringLiteral("GET"), QStringLiteral("/api/health"), QByteArray(), 1,
                         [&](const Result<QJsonDocument> &res) { retryDone = res.isSuccess(); });
        QTRY_VERIFY_WITH_TIMEOUT(retryDone, 2000);

        // 401 with expired refresh token emits tokenRefreshRequired
        SecureStorage::instance()->saveTokens(QStringLiteral("old_access"), QStringLiteral("expired_refresh"));
        m_server.setFailNextRequests(1, 401);
        QSignalSpy refreshRequiredSpy(nm, &NetworkManager::tokenRefreshRequired);
        nm->executeGet(QStringLiteral("/api/health"), [](const Result<QJsonDocument> &) {});
        QTRY_VERIFY_WITH_TIMEOUT(refreshRequiredSpy.count() >= 1, 2000);
    }

    // ── 8. SecureStorage Extended Deep Coverage ──────────────────────────────
    void testSecureStorageExtended() {
        SecureStorage *storage = SecureStorage::instance();
        storage->resetForTesting();

        // Token operations
        QVERIFY(storage->accessToken().isEmpty());
        QVERIFY(storage->refreshToken().isEmpty());

        storage->saveTokens(QStringLiteral("access_xyz"), QStringLiteral("refresh_abc"));
        QCOMPARE(storage->accessToken(), QStringLiteral("access_xyz"));
        QCOMPARE(storage->refreshToken(), QStringLiteral("refresh_abc"));

        storage->clearTokens();
        QVERIFY(storage->accessToken().isEmpty());
        QVERIFY(storage->refreshToken().isEmpty());

        // Key encryption with custom IV
        QByteArray key = SecureStorage::generateRandomKey(32);
        QByteArray iv = SecureStorage::generateRandomKey(12);
        QByteArray plain = "Extended Confidential Payload";
        QByteArray cipher = SecureStorage::encryptAesGcm(plain, key, iv);
        QVERIFY(!cipher.isEmpty());
        QCOMPARE(SecureStorage::decryptAesGcm(cipher, key), plain);

        // Decrypt short cipher error
        QCOMPARE(SecureStorage::decryptAesGcm(QByteArray("short"), key), QByteArray());

        // Save and delete
        storage->saveSecret(QStringLiteral("temp_secret_key"), QStringLiteral("secret_val"));
        QVERIFY(storage->deleteSecret(QStringLiteral("temp_secret_key")));

        // EncryptedVault Backend explicit path
        storage->setBackendForTesting(SecureStorage::Backend::EncryptedVault);
        QCOMPARE(storage->activeBackend(), SecureStorage::Backend::EncryptedVault);
        QVERIFY(storage->saveSecret(QStringLiteral("vault_key"), QStringLiteral("vault_secret_val")));
        QCOMPARE(storage->getSecret(QStringLiteral("vault_key")), QStringLiteral("vault_secret_val"));
        QVERIFY(storage->deleteSecret(QStringLiteral("vault_key")));
        QVERIFY(storage->getSecret(QStringLiteral("vault_key")).isEmpty());
        storage->setBackendForTesting(SecureStorage::Backend::PlatformDefault);

        storage->clearAllSecrets();
    }
};

QTEST_MAIN(TestUnitModelsCoverage)
#include "test_unit_models_coverage.moc"
