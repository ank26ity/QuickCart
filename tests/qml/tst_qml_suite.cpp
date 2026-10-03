/**
 * @file tst_qml_suite.cpp
 * @brief Qt Quick Test test harness driving TestCase suites for Theme tokens, Router guards, and Role login flows.
 * @layer Tests (Qt Quick Test)
 */

#include <QtQuickTest/quicktest.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include "../../core/logging.h"
#include "../../core/appconfig.h"
#include "../../core/thememanager.h"
#include "../../security/securestorage.h"
#include "../../security/permissionmanager.h"
#include "../../api/networkmanager.h"
#include "../../models/authservice.h"
#include "../../models/shopmodel.h"
#include "../../models/productmodel.h"
#include "../../models/cartmanager.h"
#include "../../models/ordermodel.h"
#include "../tools/mockapiserver.h"

class Setup : public QObject {
    Q_OBJECT
    std::unique_ptr<MockApiServer> m_mockServer;
    AuthService *m_authService{nullptr};
    PermissionManager *m_permissionManager{nullptr};
    ShopModel *m_shopModel{nullptr};
    ProductModel *m_productModel{nullptr};
    CartManager *m_cartManager{nullptr};
    OrderModel *m_orderModel{nullptr};

public:
    Setup() {
        qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
        StructuredLogger::init();
    }

    ~Setup() override {
        if (m_mockServer) {
            m_mockServer->stop();
        }
    }

public slots:
    void qmlEngineAvailable(QQmlEngine *engine) {
        if (!m_mockServer) {
            m_mockServer = std::make_unique<MockApiServer>();
            if (m_mockServer->start()) {
                NetworkManager::instance()->setBaseUrl(m_mockServer->url());
                AppConfig::instance()->setApiBaseUrl(m_mockServer->url());
            }
        }

        engine->addImportPath(QStringLiteral(QUICKCART_IMPORT_DIR));
        engine->addImportPath(QStringLiteral(QUICKCART_SOURCE_DIR "/qml"));
        engine->addImportPath(QStringLiteral(QUICKCART_SOURCE_DIR));

        ThemeManager *tm = ThemeManager::instance();
        engine->rootContext()->setContextProperty(QStringLiteral("themeManager"), tm);
        engine->rootContext()->setContextProperty(QStringLiteral("liveThemeManager"), tm);

        m_authService = new AuthService(this);
        m_permissionManager = new PermissionManager(this);
        m_shopModel = new ShopModel(this);
        m_productModel = new ProductModel(this);
        m_cartManager = CartManager::instance();
        m_orderModel = new OrderModel(this);

        engine->rootContext()->setContextProperty(QStringLiteral("liveAuthService"), m_authService);
        engine->rootContext()->setContextProperty(QStringLiteral("livePermissionManager"), m_permissionManager);
        engine->rootContext()->setContextProperty(QStringLiteral("liveShopModel"), m_shopModel);
        engine->rootContext()->setContextProperty(QStringLiteral("liveProductModel"), m_productModel);
        engine->rootContext()->setContextProperty(QStringLiteral("liveCartManager"), m_cartManager);
        engine->rootContext()->setContextProperty(QStringLiteral("liveOrderModel"), m_orderModel);
        engine->rootContext()->setContextProperty(QStringLiteral("liveNetworkManager"), NetworkManager::instance());
        engine->rootContext()->setContextProperty(QStringLiteral("liveAppConfig"), AppConfig::instance());
        engine->rootContext()->setContextProperty(QStringLiteral("liveSecureStorage"), SecureStorage::instance());

        // Standard context properties matching main.cpp
        engine->rootContext()->setContextProperty(QStringLiteral("authService"), m_authService);
        engine->rootContext()->setContextProperty(QStringLiteral("permissionManager"), m_permissionManager);
        engine->rootContext()->setContextProperty(QStringLiteral("shopModel"), m_shopModel);
        engine->rootContext()->setContextProperty(QStringLiteral("productModel"), m_productModel);
        engine->rootContext()->setContextProperty(QStringLiteral("cartManager"), m_cartManager);
        engine->rootContext()->setContextProperty(QStringLiteral("orderModel"), m_orderModel);
        engine->rootContext()->setContextProperty(QStringLiteral("networkManager"), NetworkManager::instance());
        engine->rootContext()->setContextProperty(QStringLiteral("appConfig"), AppConfig::instance());
        engine->rootContext()->setContextProperty(QStringLiteral("secureStorage"), SecureStorage::instance());
    }
};

QUICK_TEST_MAIN_WITH_SETUP(QuickTests, Setup)
#include "tst_qml_suite.moc"
