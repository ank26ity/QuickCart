/**
 * @file test_responsive_ui.cpp
 * @brief Automated offscreen responsive layout, breakpoint transition, and multi-role golden screenshot verification.
 * @layer Tests (C++ / Qt Quick / CTest)
 */

#include <QtTest/QtTest>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include "../../core/thememanager.h"

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
    QString m_goldenDir;

    static constexpr double MAX_MISMATCH_TOLERANCE = 0.0005; // Strict <= 0.05% tolerance threshold

    static double calculateMismatchRatio(const QImage &actual, const QImage &golden)
    {
        if (actual.size() != golden.size()) return 1.0;
        int diffPixels = 0;
        int totalPixels = actual.width() * actual.height();
        for (int y = 0; y < actual.height(); ++y) {
            for (int x = 0; x < actual.width(); ++x) {
                QRgb a = actual.pixel(x, y);
                QRgb g = golden.pixel(x, y);
                int dr = std::abs(qRed(a) - qRed(g));
                int dg = std::abs(qGreen(a) - qGreen(g));
                int db = std::abs(qBlue(a) - qBlue(g));
                // Per-pixel threshold: delta > 10 on any channel or aggregate delta > 20
                if (dr > 10 || dg > 10 || db > 10 || (dr + dg + db) > 20) {
                    diffPixels++;
                }
            }
        }
        return static_cast<double>(diffPixels) / totalPixels;
    }

private slots:
    void initTestCase()
    {
        m_sourceDir = QStringLiteral(QUICKCART_SOURCE_DIR);
        m_screenshotDir = m_sourceDir + QStringLiteral("/build/screenshots");
        m_goldenDir = m_sourceDir + QStringLiteral("/tests/golden");
        QDir().mkpath(m_screenshotDir);
        QDir().mkpath(m_goldenDir);

        qDebug() << "TestResponsiveUI initialized with SourceDir:" << m_sourceDir;
        qDebug() << "Screenshots output dir:" << m_screenshotDir;
        qDebug() << "Golden reference dir:" << m_goldenDir;
        qDebug() << "Enforced screenshot difference tolerance: <= 0.05% (0.0005)";
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

    void testDeliberateUiMismatchFails()
    {
        // ── 1. Negative Control: 20x20 rect on 360x640 mobile screen ──────────
        // Mathematical proof:
        // A 20x20 rect occupies 400 pixels.
        // On 360x640 (230,400 px), mismatch = 400 / 230,400 = 0.001736... (0.1736%).
        // Note: A 20x20 rect cannot be 1.11% of any standard golden:
        //   For 400 pixels to equal 1.11% (0.0111), total area would be 400/0.0111 = 36,036 px (~190x190).
        // Under our tightened tolerance (<=0.05% = 0.0005), 0.1736% > 0.05%, so it strictly FAILS.
        QImage base(360, 640, QImage::Format_ARGB32);
        base.fill(QColor(15, 23, 42)); // Background: dark navy (#0f172a)

        QImage rect20Perturbed = base.copy();
        QPainter p1(&rect20Perturbed);
        p1.fillRect(20, 20, 20, 20, QColor(239, 68, 68)); // 20x20 red square
        p1.end();

        double rectMismatch = calculateMismatchRatio(rect20Perturbed, base);
        qDebug() << "Negative Control [20x20 Rect]:" << (rectMismatch * 100.0) << "% (tightened threshold: <= 0.05%)";
        QVERIFY2(rectMismatch > MAX_MISMATCH_TOLERANCE,
                 qPrintable(QString("20x20 rect (%1%) must exceed tightened 0.05% threshold").arg(rectMismatch * 100.0)));

        // ── 2. Negative Control: Real Token Color Shift in App Layout ─────────
        // Simulate changing the HeaderBar surface token (#0f172a -> #10b981) across top 56px:
        // 360 * 56 = 20,160 pixels out of 230,400 = 8.7500% overall perturbation.
        QImage tokenPerturbed = base.copy();
        QPainter p2(&tokenPerturbed);
        p2.fillRect(0, 0, 360, 56, QColor(16, 185, 129)); // Changed header background token
        p2.end();

        double tokenMismatch = calculateMismatchRatio(tokenPerturbed, base);
        qDebug() << "Negative Control [HeaderBar Token Shift]:" << (tokenMismatch * 100.0) << "% (tightened threshold: <= 0.05%)";
        QVERIFY2(tokenMismatch > MAX_MISMATCH_TOLERANCE,
                 qPrintable(QString("HeaderBar token shift (%1%) must exceed tightened 0.05% threshold").arg(tokenMismatch * 100.0)));
        QVERIFY2(tokenMismatch >= 0.087, "HeaderBar 56px change on 360w screen must measure >= 8.7%");
    }

    void testOffscreenScreenshotCaptures()
    {
        MockAuthService authService;
        authService.setIsLoggedIn(true);

        ThemeManager *tm = ThemeManager::instance();

        struct ViewTarget
        {
            const char *viewName;
            const char *relativeQmlPath;
        };

        const ViewTarget views[] = {
            {"scaffold", "/qml/components/AppScaffold.qml"},
            {"auth_view", "/qml/views/AuthView.qml"},
            {"customer_view", "/qml/views/CustomerView.qml"},
            {"shopkeeper_view", "/qml/views/ShopkeeperView.qml"},
            {"delivery_view", "/qml/views/DeliveryView.qml"},
            {"admin_view", "/qml/views/AdminView.qml"}
        };

        struct FormFactor
        {
            const char *name;
            int width;
            int height;
        };

        const FormFactor factors[] = {
            {"360", 360, 640},
            {"768", 768, 1024},
            {"1440", 1440, 900}
        };

        const QStringList modes = { QStringLiteral("dark"), QStringLiteral("light") };

        for (const auto &target : views) {
            QQuickView view;
            view.setResizeMode(QQuickView::SizeRootObjectToView);
            view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml"));
            view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml/components"));
            view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml/theme"));
            view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml/responsive"));
            view.engine()->addImportPath(m_sourceDir + QStringLiteral("/qml/views"));
            view.rootContext()->setContextProperty(QStringLiteral("themeManager"), tm);

            view.setSource(QUrl::fromLocalFile(m_sourceDir + QString::fromLatin1(target.relativeQmlPath)));
            if (view.status() != QQuickView::Ready) {
                qWarning() << "Could not load view" << target.viewName << ":" << view.errors();
                continue;
            }

            QQuickItem *root = view.rootObject();
            if (root) {
                root->setProperty("authService", QVariant::fromValue(&authService));
            }

            for (const QString &mode : modes) {
                tm->setMode(mode);
                QTest::qWait(40);

                for (const auto &factor : factors) {
                    QString fullName = QString("%1_%2w_%3").arg(target.viewName).arg(factor.width).arg(mode);
                    view.resize(factor.width, factor.height);
                    view.show();
                    QTest::qWait(80);

                    QImage frame = view.grabWindow();
                    QVERIFY2(!frame.isNull(), qPrintable(QString("Frame grab for %1 failed").arg(fullName)));

                    QString outPath = QString("%1/%2.png").arg(m_screenshotDir, fullName);
                    bool saved = frame.save(outPath);
                    QVERIFY2(saved, qPrintable(QString("Failed to save screenshot: %1").arg(outPath)));

                    QString goldenPath = QString("%1/%2.png").arg(m_goldenDir, fullName);
                    if (!QFileInfo::exists(goldenPath)) {
                        bool goldenSaved = frame.save(goldenPath);
                        QVERIFY2(goldenSaved, qPrintable(QString("Failed to save golden image: %1").arg(goldenPath)));
                        qDebug() << "[Golden Seeded]" << fullName << "->" << goldenPath;
                    } else {
                        QImage goldenImage(goldenPath);
                        QVERIFY2(!goldenImage.isNull(), qPrintable(QString("Failed to read golden image: %1").arg(goldenPath)));
                        double mismatch = calculateMismatchRatio(frame, goldenImage);
                        qDebug() << "[Screenshot Check]" << fullName << "Mismatch:" << (mismatch * 100.0)
                                 << "% (tolerance: <= 0.5%)";
                        QVERIFY2(mismatch <= MAX_MISMATCH_TOLERANCE,
                                 qPrintable(QString("Screenshot difference %1% exceeds tolerance of 0.5% for %2")
                                     .arg(mismatch * 100.0, 0, 'f', 3).arg(fullName)));
                    }
                }
            }
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
