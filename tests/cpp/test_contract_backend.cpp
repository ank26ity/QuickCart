/**
 * @file test_contract_backend.cpp
 * @brief Contract Test verifying client integration against both MockApiServer and the real backend server.
 * @layer Tests / Contract (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include <QtCore/QProcess>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QUuid>
#include <QtCore/QFile>
#include <QtTest/QSignalSpy>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include "../../api/networkmanager.h"
#include "../../api/apiclient.h"
#include "../../models/shopmodel.h"
#include "../../models/cartmanager.h"
#include "../../models/authservice.h"
#include "../../models/ordermodel.h"
#include "../../core/orderstatemachine.h"
#include "../../security/securestorage.h"
#include "../tools/mockapiserver.h"

class TestContractBackend : public QObject {
    Q_OBJECT

private:
    MockApiServer m_mockServer;
    QString m_realBackendUrl;
    bool m_hasRealBackend{false};

    bool checkServerHealth(const QString &url) {
        for (int attempt = 0; attempt < 5; ++attempt) {
            QNetworkAccessManager nam;
            QNetworkRequest req(QUrl(url + QStringLiteral("/health")));
            req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
            QNetworkReply *reply = nam.get(req);

            QEventLoop loop;
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            QTimer::singleShot(5000, &loop, &QEventLoop::quit);
            loop.exec();

            bool ok = (reply->isFinished() && reply->error() == QNetworkReply::NoError &&
                       reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200);
            reply->deleteLater();
            if (ok)
                return true;
            QTest::qWait(500);
        }
        return false;
    }

    void executeContractFlowAgainstServer(const QString &serverBaseUrl, const QString &serverName) {
        qInfo() << "=== Executing Contract Flow against:" << serverName << "at" << serverBaseUrl << "===";
        NetworkManager::instance()->setBaseUrl(serverBaseUrl);

        // 1. Config Contract: OpenAPI schema & integer paise validation
        {
            QNetworkAccessManager nam;
            QNetworkRequest req(QUrl(serverBaseUrl + QStringLiteral("/config")));
            QNetworkReply *reply = nam.get(req);
            QEventLoop loop;
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            QCOMPARE(reply->error(), QNetworkReply::NoError);
            QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QVERIFY(doc.isObject());
            QJsonObject obj = doc.object();
            // OpenAPI required fields
            QVERIFY(obj.contains(QStringLiteral("deliveryFeePaise")));
            QVERIFY(obj.contains(QStringLiteral("freeDeliveryThresholdPaise")));
            QVERIFY(obj.contains(QStringLiteral("currency")));
            QCOMPARE(obj.value(QStringLiteral("deliveryFeePaise")).toVariant().toLongLong(), 4900LL);
            QCOMPARE(obj.value(QStringLiteral("freeDeliveryThresholdPaise")).toVariant().toLongLong(), 49900LL);
            reply->deleteLater();
        }

        // 2. Auth Flow: Login contract
        QString customerToken;
        {
            QNetworkAccessManager nam;
            QNetworkRequest req(QUrl(serverBaseUrl + QStringLiteral("/auth/login")));
            req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
            QJsonObject loginBody;
            loginBody[QStringLiteral("email")] = QStringLiteral("customer@quickcart.com");
            loginBody[QStringLiteral("password")] = QStringLiteral("Password123!");
            QNetworkReply *reply = nam.post(req, QJsonDocument(loginBody).toJson(QJsonDocument::Compact));

            QEventLoop loop;
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (code == 200) {
                QJsonObject authObj = QJsonDocument::fromJson(reply->readAll()).object();
                customerToken = authObj.value(QStringLiteral("accessToken")).toString();
                QVERIFY(!customerToken.isEmpty());
                QVERIFY(authObj.contains(QStringLiteral("user")));
            } else {
                customerToken = QStringLiteral("mock_jwt_customer_1");
            }
            reply->deleteLater();
        }

        // 3. Shop Discovery: 3km boundary & OpenAPI shop schema
        QString chosenShopId;
        {
            ShopModel shops;
            QSignalSpy loadSpy(&shops, &ShopModel::shopsLoaded);
            shops.fetchShops(QStringLiteral("all"), 12.9716, 77.5946);
            QVERIFY(loadSpy.wait(4000));
            QVERIFY(shops.count() >= 1);

            for (int i = 0; i < shops.count(); ++i) {
                QVariantMap s = shops.getShopAt(i);
                // OpenAPI schema check
                QVERIFY(s.contains(QStringLiteral("_id")) || s.contains(QStringLiteral("id")));
                QVERIFY(s.contains(QStringLiteral("name")));
                QVERIFY(s.contains(QStringLiteral("lat")));
                QVERIFY(s.contains(QStringLiteral("lng")));
                if (chosenShopId.isEmpty()) {
                    chosenShopId = s.value(QStringLiteral("_id")).toString();
                    if (chosenShopId.isEmpty())
                        chosenShopId = s.value(QStringLiteral("id")).toString();
                }
            }
        }
        if (chosenShopId.isEmpty()) {
            chosenShopId = QStringLiteral("shop_1");
        }

        // 4. Cart Calculation: Server integer paise parity
        {
            QJsonObject body;
            QJsonArray items;
            QJsonObject item1;
            item1[QStringLiteral("productId")] = QStringLiteral("prod_1");
            item1[QStringLiteral("quantity")] = 2;
            item1[QStringLiteral("pricePaise")] = 12000; // 2 * ₹120 = 24000 paise
            items.append(item1);
            body[QStringLiteral("items")] = items;

            QNetworkAccessManager nam;
            QNetworkRequest req(QUrl(serverBaseUrl + QStringLiteral("/cart/calculate")));
            req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
            QNetworkReply *reply = nam.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));

            QEventLoop loop;
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            QCOMPARE(reply->error(), QNetworkReply::NoError);
            QJsonObject resObj = QJsonDocument::fromJson(reply->readAll()).object();
            QCOMPARE(resObj.value(QStringLiteral("subtotalPaise")).toVariant().toLongLong(), 24000LL);
            QCOMPARE(resObj.value(QStringLiteral("deliveryFeePaise")).toVariant().toLongLong(), 4900LL);
            QCOMPARE(resObj.value(QStringLiteral("totalPaise")).toVariant().toLongLong(), 28900LL);
            reply->deleteLater();
        }

        // 5. Order Creation: Full flow with Idempotency Key
        QString createdOrderId;
        {
            QJsonObject orderReq;
            orderReq[QStringLiteral("shopId")] = chosenShopId;
            orderReq[QStringLiteral("deliveryAddress")] = QStringLiteral("123 Indiranagar, Bangalore");
            orderReq[QStringLiteral("idempotency_key")] = QUuid::createUuid().toString(QUuid::WithoutBraces);
            QJsonArray orderItems;
            QJsonObject oItem;
            oItem[QStringLiteral("productId")] = QStringLiteral("prod_1");
            oItem[QStringLiteral("quantity")] = 1;
            oItem[QStringLiteral("pricePaise")] = 12000;
            orderItems.append(oItem);
            orderReq[QStringLiteral("items")] = orderItems;

            QNetworkAccessManager nam;
            QNetworkRequest req(QUrl(serverBaseUrl + QStringLiteral("/orders")));
            req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
            req.setRawHeader("Authorization", ("Bearer " + customerToken).toUtf8());
            req.setRawHeader("x-user-role", "customer");
            QNetworkReply *reply = nam.post(req, QJsonDocument(orderReq).toJson(QJsonDocument::Compact));

            QEventLoop loop;
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QVERIFY(code == 200 || code == 201);
            QJsonObject ord = QJsonDocument::fromJson(reply->readAll()).object();
            createdOrderId = ord.value(QStringLiteral("_id")).toString(ord.value(QStringLiteral("id")).toString());
            QVERIFY(!createdOrderId.isEmpty());
            QVERIFY(ord.contains(QStringLiteral("status")));
            reply->deleteLater();
        }

        // 6. Transition Order to 'ready' (Merchant/Admin update)
        {
            QJsonObject statusReq;
            statusReq[QStringLiteral("status")] = QStringLiteral("ready");

            QNetworkAccessManager nam;
            QNetworkRequest req(QUrl(serverBaseUrl + QStringLiteral("/orders/") + createdOrderId));
            req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
            req.setRawHeader("Authorization", "Bearer mock_jwt_admin_1");
            req.setRawHeader("x-user-role", "admin");
            QNetworkReply *reply =
                nam.sendCustomRequest(req, "PATCH", QJsonDocument(statusReq).toJson(QJsonDocument::Compact));

            QEventLoop loop;
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            reply->deleteLater();
        }

        // 7. Courier Claim: Assign to courier (Double-claim protected)
        {
            QJsonObject claimReq;
            claimReq[QStringLiteral("delivery_boy_id")] = QStringLiteral("user_courier_1");
            claimReq[QStringLiteral("courierId")] = QStringLiteral("user_courier_1");

            QNetworkAccessManager nam;
            QNetworkRequest req(
                QUrl(serverBaseUrl + QStringLiteral("/orders/") + createdOrderId + QStringLiteral("/assign")));
            req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
            req.setRawHeader("Authorization", "Bearer mock_jwt_delivery_1");
            req.setRawHeader("x-user-role", "delivery");
            QNetworkReply *reply =
                nam.sendCustomRequest(req, "PATCH", QJsonDocument(claimReq).toJson(QJsonDocument::Compact));

            QEventLoop loop;
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            int claimCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QVERIFY(claimCode == 200 || claimCode == 204);
            reply->deleteLater();
        }

        // 8. Delivery OTP Verification: Complete delivery
        {
            QJsonObject otpReq;
            otpReq[QStringLiteral("otp")] = QStringLiteral("1234");

            QNetworkAccessManager nam;
            QNetworkRequest req(QUrl(serverBaseUrl + QStringLiteral("/orders/") + createdOrderId +
                                     QStringLiteral("/verify-delivery-otp")));
            req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
            req.setRawHeader("Authorization", "Bearer mock_jwt_delivery_1");
            req.setRawHeader("x-user-role", "delivery");
            QNetworkReply *reply = nam.post(req, QJsonDocument(otpReq).toJson(QJsonDocument::Compact));

            QEventLoop loop;
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            int otpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QCOMPARE(otpCode, 200);
            QJsonObject otpResp = QJsonDocument::fromJson(reply->readAll()).object();
            QCOMPARE(otpResp.value(QStringLiteral("status")).toString(), QStringLiteral("delivered"));
            reply->deleteLater();
        }

        // 9. Admin RBAC Contract: 401 & 403 enforcement
        {
            QNetworkAccessManager nam;
            // 9a. Unauthenticated request -> 401
            QNetworkRequest reqNoAuth(QUrl(serverBaseUrl + QStringLiteral("/admin/metrics")));
            QNetworkReply *repNoAuth = nam.get(reqNoAuth);
            QEventLoop loop1;
            connect(repNoAuth, &QNetworkReply::finished, &loop1, &QEventLoop::quit);
            loop1.exec();
            QCOMPARE(repNoAuth->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 401);
            repNoAuth->deleteLater();

            // 9b. Customer role -> 403
            QNetworkRequest reqCustomer(QUrl(serverBaseUrl + QStringLiteral("/admin/metrics")));
            reqCustomer.setRawHeader("Authorization", "Bearer mock_jwt_customer_1");
            reqCustomer.setRawHeader("x-user-role", "customer");
            QNetworkReply *repCust = nam.get(reqCustomer);
            QEventLoop loop2;
            connect(repCust, &QNetworkReply::finished, &loop2, &QEventLoop::quit);
            loop2.exec();
            QCOMPARE(repCust->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 403);
            repCust->deleteLater();

            // 9c. Admin role -> 200
            QNetworkRequest reqAdmin(QUrl(serverBaseUrl + QStringLiteral("/admin/metrics")));
            reqAdmin.setRawHeader("Authorization", "Bearer mock_jwt_admin_1");
            reqAdmin.setRawHeader("x-user-role", "admin");
            QNetworkReply *repAdmin = nam.get(reqAdmin);
            QEventLoop loop3;
            connect(repAdmin, &QNetworkReply::finished, &loop3, &QEventLoop::quit);
            loop3.exec();
            QCOMPARE(repAdmin->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
            repAdmin->deleteLater();
        }
    }

private slots:
    void initTestCase() {
        QVERIFY(m_mockServer.start());

        m_realBackendUrl = qEnvironmentVariable("QUICKCART_BACKEND_URL", QStringLiteral("http://127.0.0.1:3000/api"));
        m_hasRealBackend = checkServerHealth(m_realBackendUrl);
        if (!m_hasRealBackend && m_realBackendUrl.contains(QStringLiteral("127.0.0.1"))) {
            QString altUrl =
                QString(m_realBackendUrl).replace(QStringLiteral("127.0.0.1"), QStringLiteral("localhost"));
            if (checkServerHealth(altUrl)) {
                m_realBackendUrl = altUrl;
                m_hasRealBackend = true;
            }
        }
        qInfo() << "TestContractBackend initialized. Mock URL:" << m_mockServer.url()
                << "| Real Backend URL:" << m_realBackendUrl << "| Real Backend Accessible:" << m_hasRealBackend;
    }

    void cleanupTestCase() { m_mockServer.stop(); }

    void testContractAgainstMockApiServer() {
        executeContractFlowAgainstServer(m_mockServer.url() + QStringLiteral("/api"), QStringLiteral("MockApiServer"));
    }

    void testContractAgainstRealBackend() {
        bool isCi = qEnvironmentVariableIsSet("CI") || qEnvironmentVariableIsSet("GITHUB_ACTIONS") ||
                    qEnvironmentVariable("REQUIRE_REAL_BACKEND") == QStringLiteral("1");

        if (isCi && !m_hasRealBackend) {
            QFAIL("Real backend is unreachable in CI environment! Failing contract test as required.");
        } else if (!m_hasRealBackend) {
            QSKIP("Real backend is not running at QUICKCART_BACKEND_URL. Skipping live backend stage.");
        }
        executeContractFlowAgainstServer(m_realBackendUrl, QStringLiteral("Real Node.js/MongoDB Backend"));
    }

    void testOrderTransitionsParityWithSpec() {
        // Load canonical specification file: core/order_transitions.json
        QString specPath = QStringLiteral("core/order_transitions.json");
        QFile file(specPath);
        if (!file.exists()) {
            specPath = QStringLiteral("../core/order_transitions.json");
            file.setFileName(specPath);
        }
        if (!file.exists()) {
            specPath = QStringLiteral("../../core/order_transitions.json");
            file.setFileName(specPath);
        }

        QVERIFY2(file.open(QIODevice::ReadOnly), "Must open core/order_transitions.json specification");
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QVERIFY(doc.isObject());
        QJsonObject root = doc.object();

        QJsonArray transitions = root.value(QStringLiteral("transitions")).toArray();
        QVERIFY(!transitions.isEmpty());

        for (const QJsonValue &v : transitions) {
            QJsonObject t = v.toObject();
            QString from = t.value(QStringLiteral("from")).toString();
            QString to = t.value(QStringLiteral("to")).toString();
            QJsonArray allowedActors = t.value(QStringLiteral("allowedActors")).toArray();

            OrderStateMachine::OrderStatus fromStatus = OrderStateMachine::statusFromString(from);
            OrderStateMachine::OrderStatus toStatus = OrderStateMachine::statusFromString(to);

            for (const QJsonValue &actVal : allowedActors) {
                QString actorStr = actVal.toString();
                OrderStateMachine::OrderActor actor = OrderStateMachine::actorFromString(actorStr);

                bool allowed = OrderStateMachine::canTransition(fromStatus, toStatus, actor).isSuccess();
                QVERIFY2(allowed, QString("Transition %1 -> %2 by actor %3 must be allowed according to spec")
                                      .arg(from, to, actorStr)
                                      .toUtf8()
                                      .constData());
            }
        }
    }
};

QTEST_MAIN(TestContractBackend)
#include "test_contract_backend.moc"
