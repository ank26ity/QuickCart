/**
 * @file test_responsive_ui.cpp
 * @brief Automated offscreen responsive layout, breakpoint transition, and screenshot verification test.
 * @layer Tests (C++ / Qt Quick / CTest)
 */

#include <QtTest/QtTest>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

class MockAuthService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn WRITE setIsLoggedIn NOTIFY authChanged)
    Q_PROPERTY(QString userName READ userName WRITE setUserName NOTIFY authChanged)
    Q_PROPERTY(QString userRole READ userRole WRITE setUserRole NOTIFY authChanged)

public:
    explicit MockAuthService(QObject *parent = nullptr)
        : QObject(parent)
        , m_isLoggedIn(false)
        , m_userName(QStringLiteral("Alex Test"))
        , m_userRole(QStringLiteral("customer"))
    {
    }

    bool isLoggedIn() const { return m_isLoggedIn; }
    void setIsLoggedIn(bool val)
    {
        if (m_isLoggedIn != val) {
            m_isLoggedIn = val;
            emit authChanged();
        }
    }

    QString userName() const { return m_userName; }
    void setUserName(const QString &val)
    {
        if (m_userName != val) {
            m_userName = val;
            emit authChanged();
        }
    }

    QString userRole() const { return m_userRole; }
    void setUserRole(const QString &val)
    {
        if (m_userRole != val) {
            m_userRole = val;
            emit authChanged();
        }
    }

signals:
    void authChanged();

private:
    bool m_isLoggedIn;
    QString m_userName;
    QString m_userRole;
};

class TestResponsiveUI : public QObject
{
    Q_OBJECT

private:
    QString m_sourceDir;
    QString m_screenshotDir;

private slots:
    void initTestCase()
    {
        m_sourceDir = QStringLiteral(QUICKCART_SOURCE_DIR);
        m_screenshotDir = m_sourceDir + QStringLiteral("/build/screenshots");
        QDir().mkpath(m_screenshotDir);

        qDebug() << "TestResponsiveUI initialized with SourceDir:" << m_sourceDir;
        qDebug() << "Screenshots output dir:" << m_screenshotDir;
    }

    void testResponsiveBreakpoints()
    {
        QQmlEngine engine;
        engine.addImportPath(m_sourceDir + QStringLiteral("/qml"));
        engine.addImportPath(m_sourceDir + QStringLiteral("/qml/responsive"));

        QQmlComponent component(&engine, QUrl::fromLocalFile(m_sourceDir + QStringLiteral("/qml/responsive/Responsive.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));

        QObject *responsive = component.create();
        QVERIFY(responsive != nullptr);

        // 1. Mobile Phone (Compact: < 600)
        responsive->setProperty("windowWidth", 360);
        responsive->setProperty("windowHeight", 640);
        QCOMPARE(responsive->property("isCompact").toBool(), true);
        QCOMPARE(responsive->property("isMedium").toBool(), false);
        QCOMPARE(responsive->property("isExpanded").toBool(), false);
        QCOMPARE(responsive->property("isExtraLarge").toBool(), false);
        QCOMPARE(responsive->property("isMobile").toBool(), true);
        QCOMPARE(responsive->property("isTablet").toBool(), false);
        QCOMPARE(responsive->property("isDesktop").toBool(), false);
        QCOMPARE(responsive->property("columns").toInt(), 1);
        QCOMPARE(responsive->property("gutter").toDouble(), 12.0);

        // Compact boundary
        responsive->setProperty("windowWidth", 599);
        QCOMPARE(responsive->property("isCompact").toBool(), true);
        QCOMPARE(responsive->property("columns").toInt(), 1);

        // 2. Tablet (Medium: 600 <= width < 840)
        responsive->setProperty("windowWidth", 600);
        QCOMPARE(responsive->property("isCompact").toBool(), false);
        QCOMPARE(responsive->property("isMedium").toBool(), true);
        QCOMPARE(responsive->property("isExpanded").toBool(), false);
        QCOMPARE(responsive->property("isExtraLarge").toBool(), false);
        QCOMPARE(responsive->property("isMobile").toBool(), false);
        QCOMPARE(responsive->property("isTablet").toBool(), true);
        QCOMPARE(responsive->property("isDesktop").toBool(), false);
        QCOMPARE(responsive->property("columns").toInt(), 2);
        QCOMPARE(responsive->property("gutter").toDouble(), 16.0);

        // Medium upper boundary
        responsive->setProperty("windowWidth", 839);
        QCOMPARE(responsive->property("isMedium").toBool(), true);
        QCOMPARE(responsive->property("columns").toInt(), 2);

        // 3. Laptop / Expanded Desktop (Expanded: 840 <= width < 1200)
        responsive->setProperty("windowWidth", 840);
        QCOMPARE(responsive->property("isCompact").toBool(), false);
        QCOMPARE(responsive->property("isMedium").toBool(), false);
        QCOMPARE(responsive->property("isExpanded").toBool(), true);
        QCOMPARE(responsive->property("isExtraLarge").toBool(), false);
        QCOMPARE(responsive->property("isMobile").toBool(), false);
        QCOMPARE(responsive->property("isTablet").toBool(), false);
        QCOMPARE(responsive->property("isDesktop").toBool(), true);
        QCOMPARE(responsive->property("columns").toInt(), 3);
        QCOMPARE(responsive->property("gutter").toDouble(), 24.0);

        // Expanded upper boundary
        responsive->setProperty("windowWidth", 1199);
        QCOMPARE(responsive->property("isExpanded").toBool(), true);
        QCOMPARE(responsive->property("columns").toInt(), 3);

        // 4. Large Desktop (ExtraLarge: >= 1200)
        responsive->setProperty("windowWidth", 1200);
        QCOMPARE(responsive->property("isCompact").toBool(), false);
        QCOMPARE(responsive->property("isMedium").toBool(), false);
        QCOMPARE(responsive->property("isExpanded").toBool(), false);
        QCOMPARE(responsive->property("isExtraLarge").toBool(), true);
        QCOMPARE(responsive->property("isDesktop").toBool(), true);
        QCOMPARE(responsive->property("columns").toInt(), 4);
        QCOMPARE(responsive->property("gutter").toDouble(), 24.0);

        // 5. WCAG Minimum Touch Target Rule
        QVERIFY2(responsive->property("minTouchTarget").toDouble() >= 48.0,
                 "WCAG 2.1 touch target must be at least 48dp");

        delete responsive;
    }

    void testSafeAreaAndKeyboard()
    {
        QQmlEngine engine;
        engine.addImportPath(m_sourceDir + QStringLiteral("/qml"));
        engine.addImportPath(m_sourceDir + QStringLiteral("/qml/responsive"));

        QQmlComponent component(&engine, QUrl::fromLocalFile(m_sourceDir + QStringLiteral("/qml/responsive/Responsive.qml")));
        QVERIFY(component.isReady());

        QObject *responsive = component.create();
        QVERIFY(responsive != nullptr);

        // Initially no insets
        QCOMPARE(responsive->property("safeAreaTop").toDouble(), 0.0);
        QCOMPARE(responsive->property("safeAreaBottom").toDouble(), 0.0);
        QCOMPARE(responsive->property("keyboardHeight").toDouble(), 0.0);
        QCOMPARE(responsive->property("isKeyboardVisible").toBool(), false);

        // Simulate mobile notch and navigation bar insets
        responsive->setProperty("safeAreaTop", 44.0);
        responsive->setProperty("safeAreaBottom", 34.0);
        responsive->setProperty("safeAreaLeft", 10.0);
        responsive->setProperty("safeAreaRight", 10.0);
        QCOMPARE(responsive->property("safeAreaTop").toDouble(), 44.0);
        QCOMPARE(responsive->property("safeAreaBottom").toDouble(), 34.0);

        // Simulate keyboard popup
        responsive->setProperty("keyboardHeight", 280.0);
        QCOMPARE(responsive->property("isKeyboardVisible").toBool(), true);

        // Simulate keyboard dismissal
        responsive->setProperty("keyboardHeight", 0.0);
        QCOMPARE(responsive->property("isKeyboardVisible").toBool(), false);

        delete responsive;
    }

    void testAppScaffoldUnauthenticatedState()
    {
        MockAuthService authService;
        authService.setIsLoggedIn(false);

        QQuickView view;
        view.setResizeMode(QQuickView::SizeRootObjectToView);
        view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml"));
        view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml/components"));
        view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml/theme"));
        view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml/responsive"));

        view.setSource(QUrl::fromLocalFile(m_sourceDir + QStringLiteral("/qml/components/AppScaffold.qml")));
        QCOMPARE(view.status(), QQuickView::Ready);
        view.show();

        QQuickItem *root = view.rootObject();
        QVERIFY(root != nullptr);
        root->setProperty("authService", QVariant::fromValue(&authService));

        QQuickItem *sidebar = root->findChild<QQuickItem *>(QStringLiteral("sidebar"));
        QQuickItem *navRail = root->findChild<QQuickItem *>(QStringLiteral("navRail"));
        QQuickItem *topBar = root->findChild<QQuickItem *>(QStringLiteral("topBar"));
        QQuickItem *bottomNav = root->findChild<QQuickItem *>(QStringLiteral("bottomNav"));

        QVERIFY(sidebar != nullptr);
        QVERIFY(navRail != nullptr);
        QVERIFY(topBar != nullptr);
        QVERIFY(bottomNav != nullptr);

        // At Desktop resolution (1280x800) when unauthenticated:
        view.resize(1280, 800);
        QTest::qWait(50);
        QCOMPARE(sidebar->property("visible").toBool(), false);
        QCOMPARE(navRail->property("visible").toBool(), false);
        QCOMPARE(bottomNav->property("visible").toBool(), false);
        QCOMPARE(topBar->property("visible").toBool(), true);

        // At Mobile resolution (360x640) when unauthenticated:
        view.resize(360, 640);
        QTest::qWait(50);
        QCOMPARE(sidebar->property("visible").toBool(), false);
        QCOMPARE(navRail->property("visible").toBool(), false);
        QCOMPARE(bottomNav->property("visible").toBool(), false);
        QCOMPARE(topBar->property("visible").toBool(), true);
    }

    void testAppScaffoldAuthenticatedAdaptiveLayout()
    {
        MockAuthService authService;
        authService.setIsLoggedIn(true);

        QQuickView view;
        view.setResizeMode(QQuickView::SizeRootObjectToView);
        view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml"));
        view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml/components"));
        view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml/theme"));
        view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml/responsive"));

        view.setSource(QUrl::fromLocalFile(m_sourceDir + QStringLiteral("/qml/components/AppScaffold.qml")));
        QCOMPARE(view.status(), QQuickView::Ready);
        view.show();

        QQuickItem *root = view.rootObject();
        QVERIFY(root != nullptr);
        root->setProperty("authService", QVariant::fromValue(&authService));

        QQuickItem *sidebar = root->findChild<QQuickItem *>(QStringLiteral("sidebar"));
        QQuickItem *navRail = root->findChild<QQuickItem *>(QStringLiteral("navRail"));
        QQuickItem *topBar = root->findChild<QQuickItem *>(QStringLiteral("topBar"));
        QQuickItem *bottomNav = root->findChild<QQuickItem *>(QStringLiteral("bottomNav"));

        QVERIFY(sidebar != nullptr);
        QVERIFY(navRail != nullptr);
        QVERIFY(topBar != nullptr);
        QVERIFY(bottomNav != nullptr);

        // Case 1: Phone (360x640) -> Mobile Bottom Navigation Active
        view.resize(360, 640);
        QTest::qWait(50);
        QCOMPARE(sidebar->property("visible").toBool(), false);
        QCOMPARE(navRail->property("visible").toBool(), false);
        QCOMPARE(bottomNav->property("visible").toBool(), true);
        QCOMPARE(topBar->property("visible").toBool(), true);

        // Case 2: Tablet (720x1024) -> Navigation Rail Active
        view.resize(720, 1024);
        QTest::qWait(50);
        QCOMPARE(sidebar->property("visible").toBool(), false);
        QCOMPARE(navRail->property("visible").toBool(), true);
        QCOMPARE(bottomNav->property("visible").toBool(), false);
        QCOMPARE(topBar->property("visible").toBool(), false);

        // Case 3: Desktop (1280x800) -> Persistent Sidebar Active
        view.resize(1280, 800);
        QTest::qWait(50);
        QCOMPARE(sidebar->property("visible").toBool(), true);
        QCOMPARE(navRail->property("visible").toBool(), false);
        QCOMPARE(bottomNav->property("visible").toBool(), false);
        QCOMPARE(topBar->property("visible").toBool(), false);
    }

    void testOffscreenScreenshotCaptures()
    {
        MockAuthService authService;
        authService.setIsLoggedIn(true);

        QQuickView view;
        view.setResizeMode(QQuickView::SizeRootObjectToView);
        view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml"));
        view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml/components"));
        view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml/theme"));
        view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml/responsive"));

        view.setSource(QUrl::fromLocalFile(m_sourceDir + QStringLiteral("/qml/components/AppScaffold.qml")));
        QCOMPARE(view.status(), QQuickView::Ready);

        QQuickItem *root = view.rootObject();
        QVERIFY(root != nullptr);
        root->setProperty("authService", QVariant::fromValue(&authService));

        struct FormFactor
        {
            const char *name;
            int width;
            int height;
        };

        const FormFactor factors[] = {
            {"mobile_360x640", 360, 640},
            {"tablet_768x1024", 768, 1024},
            {"laptop_1024x768", 1024, 768},
            {"desktop_1440x900", 1440, 900}
        };

        for (const auto &factor : factors) {
            view.resize(factor.width, factor.height);
            view.show();
            QTest::qWait(100);

            QImage frame = view.grabWindow();
            QVERIFY2(!frame.isNull(), qPrintable(QString("Frame grab for %1 failed").arg(factor.name)));
            QCOMPARE(frame.width(), factor.width);
            QCOMPARE(frame.height(), factor.height);

            QString outPath = QString("%1/%2.png").arg(m_screenshotDir, factor.name);
            bool saved = frame.save(outPath);
            QVERIFY2(saved, qPrintable(QString("Failed to save screenshot: %1").arg(outPath)));

            qDebug() << "Captured offscreen frame for" << factor.name
                     << "Dimensions:" << frame.width() << "x" << frame.height()
                     << "Saved to:" << outPath;
        }
    }
};

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    qputenv("QT_QUICK_CONTROLS_STYLE", QByteArray("Basic"));
    QGuiApplication app(argc, argv);
    TestResponsiveUI tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_responsive_ui.moc"
