#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtGui/QIcon>
#include <QtQuickControls2/QQuickStyle>
#include <QtCore/QThreadPool>
#include <QtCore/QThread>
#include <QtCore/QUrl>
#include <QtCore/QCoreApplication>
#include <QtCore/QObject>

#include "core/logging.h"
#include "core/appconfig.h"
#include "core/thememanager.h"
#include "security/securestorage.h"
#include "security/permissionmanager.h"
#include "api/networkmanager.h"
#include "models/authservice.h"
#include "models/shopmodel.h"
#include "models/productmodel.h"
#include "models/cartmanager.h"
#include "models/ordermodel.h"

int main(int argc, char *argv[])
{
    // Initialize structured logging categories and PII filter
    StructuredLogger::init();

    QGuiApplication app(argc, argv);

    app.setOrganizationName("QuickShopp");
    app.setOrganizationDomain("quickshopp.app");
    app.setApplicationName("QuickCart");

    // Configure ThreadPool limits for maximum multi-threading throughput
    QThreadPool::globalInstance()->setMaxThreadCount(qMax(4, QThread::idealThreadCount() * 2));

    QQuickStyle::setStyle("Basic");

    // Foundation Singletons
    AppConfig appConfig;
    ThemeManager themeManager;
    SecureStorage secureStorage;
    PermissionManager permissionManager;
    NetworkManager networkManager;
    AuthService authService;

    // Domain Models
    ShopModel shopModel;
    ProductModel productModel;
    CartManager cartManager;
    OrderModel orderModel;

    QQmlApplicationEngine engine;

    // Register Foundation & Theme Singletons into QML
    engine.rootContext()->setContextProperty("appConfig", &appConfig);
    engine.rootContext()->setContextProperty("themeManager", &themeManager);
    engine.rootContext()->setContextProperty("secureStorage", &secureStorage);
    engine.rootContext()->setContextProperty("permissionManager", &permissionManager);

    // Register Network & Domain Models into QML
    engine.rootContext()->setContextProperty("networkManager", &networkManager);
    engine.rootContext()->setContextProperty("authService", &authService);
    engine.rootContext()->setContextProperty("shopModel", &shopModel);
    engine.rootContext()->setContextProperty("productModel", &productModel);
    engine.rootContext()->setContextProperty("cartManager", &cartManager);
    engine.rootContext()->setContextProperty("orderModel", &orderModel);

    const QUrl url(QStringLiteral("qrc:/qt/qml/QuickCart/qml/main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
