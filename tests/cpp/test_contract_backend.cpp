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
#include "../tools/mockapiserver.h"

class TestContractBackend : public QObject {
    Q_OBJECT

private:
    MockApiServer m_mockServer;
    QString m_realBackendUrl;
    bool m_hasRealBackend{false};

    bool checkServerHealth(const QString &url) {
        QNetworkAccessManager nam;
        QNetworkRequest req(QUrl(url + QStringLiteral("/health")));
        req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        QNetworkReply *reply = nam.get(req);

        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(2000, &loop, &QEventLoop::quit);
        loop.exec();

        bool ok = (reply->isFinished() && reply->error() == QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200);
        reply->deleteLater();
        return ok;
    }

    void executeContractFlowAgainstServer(const QString &serverBaseUrl, const QString &serverName) {
        qInfo() << "=== Executing Contract Flow against:" << serverName << "at" << serverBaseUrl << "===";
        NetworkManager::instance()->setBaseUrl(serverBaseUrl);

        // 1. Config Contract: integer paise fields
        {
            QNetworkAccessManager nam;
            QNetworkRequest req(QUrl(serverBaseUrl + QStringLiteral("/config")));
            QNetworkReply *reply = nam.get(req);
            QEventLoop loop;
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            QCOMPARE(reply->error(), QNetworkReply::NoError);
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = doc.object();
            QVERIFY(obj.contains(QStringLiteral("deliveryFeePaise")));
            QVERIFY(obj.contains(QStringLiteral("freeDeliveryThresholdPaise")));
            QCOMPARE(obj.value(QStringLiteral("deliveryFeePaise")).toVariant().toLongLong(), 4900LL);
            QCOMPARE(obj.value(QStringLiteral("freeDeliveryThresholdPaise")).toVariant().toLongLong(), 49900LL);
            reply->deleteLater();
        }

        // 2. Shop Discovery: 3km boundary
        {
            ShopModel shops;
            QSignalSpy loadSpy(&shops, &ShopModel::shopsLoaded);
            shops.fetchShops(QStringLiteral("all"), 12.9716, 77.5946);
            QVERIFY(loadSpy.wait(4000));
            QVERIFY(shops.count() >= 2);

            bool foundFreshMart = false;
            bool foundCornerPharmacy = false;
            for (int i = 0; i < shops.count(); ++i) {
                QString name = shops.getShopAt(i).value(QStringLiteral("name")).toString();
                if (name == QStringLiteral("Fresh Mart Daily")) foundFreshMart = true;
                if (name == QStringLiteral("Corner Pharmacy")) foundCornerPharmacy = true;
            }
            QVERIFY(foundFreshMart);
            QVERIFY(foundCornerPharmacy);
        }

        // 3. Cart Calculation: Server integer paise parity
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

        // 4. Admin RBAC Contract: 401 & 403 enforcement
        {
            QNetworkAccessManager nam;
            // 4a. Unauthenticated request -> 401
            QNetworkRequest reqNoAuth(QUrl(serverBaseUrl + QStringLiteral("/admin/metrics")));
            QNetworkReply *repNoAuth = nam.get(reqNoAuth);
            QEventLoop loop1;
            connect(repNoAuth, &QNetworkReply::finished, &loop1, &QEventLoop::quit);
            loop1.exec();
            QCOMPARE(repNoAuth->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 401);
            repNoAuth->deleteLater();

            // 4b. Customer role -> 403
            QNetworkRequest reqCustomer(QUrl(serverBaseUrl + QStringLiteral("/admin/metrics")));
            reqCustomer.setRawHeader("Authorization", "Bearer mock_jwt_customer_1");
            reqCustomer.setRawHeader("x-user-role", "customer");
            QNetworkReply *repCust = nam.get(reqCustomer);
            QEventLoop loop2;
            connect(repCust, &QNetworkReply::finished, &loop2, &QEventLoop::quit);
            loop2.exec();
            QCOMPARE(repCust->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 403);
            repCust->deleteLater();

            // 4c. Admin role -> 200
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
        qInfo() << "TestContractBackend initialized. Mock URL:" << m_mockServer.url()
                << "| Real Backend URL:" << m_realBackendUrl
                << "| Real Backend Accessible:" << m_hasRealBackend;
    }

    void cleanupTestCase() {
        m_mockServer.stop();
    }

    void testContractAgainstMockApiServer() {
        executeContractFlowAgainstServer(m_mockServer.url() + QStringLiteral("/api"), QStringLiteral("MockApiServer"));
    }

    void testContractAgainstRealBackend() {
        if (!m_hasRealBackend) {
            QSKIP("Real backend is not running at QUICKCART_BACKEND_URL. Skipping live backend stage.");
        }
        executeContractFlowAgainstServer(m_realBackendUrl, QStringLiteral("Real Node.js/MongoDB Backend"));
    }
};

QTEST_MAIN(TestContractBackend)
#include "test_contract_backend.moc"
