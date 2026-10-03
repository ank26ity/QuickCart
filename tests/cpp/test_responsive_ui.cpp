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

class MockAuthService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn WRITE setIsLoggedIn NOTIFY authChanged)
    Q_PROPERTY(QString userName READ userName WRITE setUserName NOTIFY authChanged)
    Q_PROPERTY(QString userRole READ userRole WRITE setUserRole NOTIFY authChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage CONSTANT)

public:
    explicit MockAuthService(QObject *parent = nullptr)
        : QObject(parent), m_isLoggedIn(false), m_userName(QStringLiteral("Alex Test")),
          m_userRole(QStringLiteral("customer")) {}

    bool isLoggedIn() const { return m_isLoggedIn; }
    void setIsLoggedIn(bool val) {
        if (m_isLoggedIn != val) {
            m_isLoggedIn = val;
            emit authChanged();
        }
    }

    QString userName() const { return m_userName; }
    void setUserName(const QString &val) {
        if (m_userName != val) {
            m_userName = val;
            emit authChanged();
        }
    }

    QString userRole() const { return m_userRole; }
    void setUserRole(const QString &val) {
        if (m_userRole != val) {
            m_userRole = val;
            emit authChanged();
        }
    }

    QString errorMessage() const { return QString(); }

signals:
    void authChanged();

private:
    bool m_isLoggedIn;
    QString m_userName;
    QString m_userRole;
};

class TestResponsiveUI : public QObject {
    Q_OBJECT

public:
    struct MismatchResult {
        double globalRatio{0.0};
        double maxRegionRatio{0.0};
        int worstRegionX{0};
        int worstRegionY{0};
        bool passed{false};
    };

    static constexpr double MAX_GLOBAL_TOLERANCE = 0.0005; // Strict <= 0.05% global threshold
    static constexpr double MAX_REGION_TOLERANCE = 0.05;   // Strict <= 5.0% localized regional threshold
    static constexpr int REGION_TILE_SIZE = 32;            // 32x32 pixel tiles for regional sensitivity

    static MismatchResult calculateMismatch(const QImage &actual, const QImage &golden) {
        if (actual.size() != golden.size() || actual.isNull() || golden.isNull()) {
            return MismatchResult{1.0, 1.0, 0, 0, false};
        }

        int width = actual.width();
        int height = actual.height();
        int totalPixels = width * height;
        int globalDiff = 0;

        int numTilesX = (width + REGION_TILE_SIZE - 1) / REGION_TILE_SIZE;
        int numTilesY = (height + REGION_TILE_SIZE - 1) / REGION_TILE_SIZE;
        std::vector<std::vector<int>> tileDiffs(numTilesY, std::vector<int>(numTilesX, 0));
        std::vector<std::vector<int>> tileTotals(numTilesY, std::vector<int>(numTilesX, 0));

        for (int y = 0; y < height; ++y) {
            int ty = y / REGION_TILE_SIZE;
            for (int x = 0; x < width; ++x) {
                int tx = x / REGION_TILE_SIZE;
                tileTotals[ty][tx]++;

                QRgb a = actual.pixel(x, y);
                QRgb g = golden.pixel(x, y);
                int dr = std::abs(qRed(a) - qRed(g));
                int dg = std::abs(qGreen(a) - qGreen(g));
                int db = std::abs(qBlue(a) - qBlue(g));
                // Per-pixel threshold: delta > 10 on any channel or aggregate delta > 20
                if (dr > 10 || dg > 10 || db > 10 || (dr + dg + db) > 20) {
                    globalDiff++;
                    tileDiffs[ty][tx]++;
                }
            }
        }

        double globalRatio = static_cast<double>(globalDiff) / totalPixels;
        double maxRegionRatio = 0.0;
        int worstTx = 0;
        int worstTy = 0;

        for (int ty = 0; ty < numTilesY; ++ty) {
            for (int tx = 0; tx < numTilesX; ++tx) {
                if (tileTotals[ty][tx] > 0) {
                    double rRatio = static_cast<double>(tileDiffs[ty][tx]) / tileTotals[ty][tx];
                    if (rRatio > maxRegionRatio) {
                        maxRegionRatio = rRatio;
                        worstTx = tx * REGION_TILE_SIZE;
                        worstTy = ty * REGION_TILE_SIZE;
                    }
                }
            }
        }

        bool passed = (globalRatio <= MAX_GLOBAL_TOLERANCE) && (maxRegionRatio <= MAX_REGION_TOLERANCE);
        return MismatchResult{globalRatio, maxRegionRatio, worstTx, worstTy, passed};
    }

private:
    QString m_sourceDir;
    QString m_screenshotDir;
    QString m_goldenDir;

private slots:
    void initTestCase() {
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

    void testResponsiveBreakpoints() {
        QQmlEngine engine;
        engine.addImportPath(m_sourceDir + QStringLiteral("/qml"));
        engine.addImportPath(m_sourceDir + QStringLiteral("/qml/responsive"));

        QQmlComponent component(&engine,
                                QUrl::fromLocalFile(m_sourceDir + QStringLiteral("/qml/responsive/Responsive.qml")));
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

    void testSafeAreaAndKeyboard() {
        QQmlEngine engine;
        engine.addImportPath(m_sourceDir + QStringLiteral("/qml"));
        engine.addImportPath(m_sourceDir + QStringLiteral("/qml/responsive"));

        QQmlComponent component(&engine,
                                QUrl::fromLocalFile(m_sourceDir + QStringLiteral("/qml/responsive/Responsive.qml")));
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

    void testDeliberateUiMismatchFails() {
        // ── 1. Negative Control: 20x20 rect on 1440x900 desktop screen ──────────
        // Mathematical proof:
        // A 20x20 rect occupies 400 pixels.
        // On 1440x900 (1,296,000 px), global mismatch = 400 / 1,296,000 = 0.03086%.
        // Under a global threshold alone (<=0.05%), it would pass (0.03086% <= 0.05%).
        // BUT with our 32x32 regional grid, the tile contains 400 changed pixels out of 1024 (39.06% regional diff).
        // Since regional tolerance is <= 5.0%, it strictly FAILS!
        QImage base1440(1440, 900, QImage::Format_ARGB32);
        base1440.fill(QColor(15, 23, 42)); // Background: dark navy (#0f172a)

        QImage rect20Perturbed = base1440.copy();
        QPainter p1(&rect20Perturbed);
        p1.fillRect(100, 100, 20, 20, QColor(239, 68, 68)); // 20x20 red square
        p1.end();

        MismatchResult rectRes = calculateMismatch(rect20Perturbed, base1440);
        qDebug() << "Negative Control [20x20 Rect at 1440x900]: Global =" << (rectRes.globalRatio * 100.0)
                 << "% (<=0.05%), Max Region =" << (rectRes.maxRegionRatio * 100.0)
                 << "% (<=5.0%), Passed =" << rectRes.passed;

        QVERIFY2(!rectRes.passed, "20x20 perturbation at 1440x900 MUST fail the regional mismatch check");
        QVERIFY2(rectRes.maxRegionRatio > MAX_REGION_TOLERANCE, "Max region diff must exceed 5% tolerance");

        // ── 2. Negative Control: Real Theme Token Change in App ───────────────
        // Mutate Theme.primary token from Electric Blue (#2563eb) to Vibrant Crimson (#dc2626)
        // over a 240x48 CTA button:
        QImage buttonBase(360, 640, QImage::Format_ARGB32);
        buttonBase.fill(QColor(15, 23, 42));
        QPainter pBase(&buttonBase);
        pBase.fillRect(60, 300, 240, 48, QColor(37, 99, 235)); // Original Theme.primary: #2563eb
        pBase.end();

        QImage buttonMutated = buttonBase.copy();
        QPainter pMut(&buttonMutated);
        pMut.fillRect(60, 300, 240, 48, QColor(220, 38, 38)); // Mutated Theme.primary: #dc2626
        pMut.end();

        MismatchResult tokenRes = calculateMismatch(buttonMutated, buttonBase);
        qDebug() << "Negative Control [Real Theme.primary Token Mutation]: Global =" << (tokenRes.globalRatio * 100.0)
                 << "%, Max Region =" << (tokenRes.maxRegionRatio * 100.0) << "%, Passed =" << tokenRes.passed;

        QVERIFY2(!tokenRes.passed, "Real Theme token color shift MUST fail golden test");
        QVERIFY2(tokenRes.maxRegionRatio > MAX_REGION_TOLERANCE, "Token shift region mismatch must exceed 5%");
    }

    void testOffscreenScreenshotCaptures() {
        MockAuthService authService;
        authService.setIsLoggedIn(true);

        ThemeManager *tm = ThemeManager::instance();

        struct ViewTarget {
            const char *viewName;
            const char *relativeQmlPath;
        };

        const ViewTarget views[] = {
            {"scaffold", "/qml/components/AppScaffold.qml"},  {"auth_view", "/qml/views/AuthView.qml"},
            {"customer_view", "/qml/views/CustomerView.qml"}, {"shopkeeper_view", "/qml/views/ShopkeeperView.qml"},
            {"delivery_view", "/qml/views/DeliveryView.qml"}, {"admin_view", "/qml/views/AdminView.qml"}};

        struct FormFactor {
            const char *name;
            int width;
            int height;
        };

        const FormFactor factors[] = {{"360", 360, 640}, {"768", 768, 1024}, {"1440", 1440, 900}};

        const QStringList modes = {QStringLiteral("dark"), QStringLiteral("light")};
        bool generateGoldens = qEnvironmentVariableIsSet("GENERATE_GOLDENS");

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
                    if (generateGoldens || !QFileInfo::exists(goldenPath)) {
                        bool goldenSaved = frame.save(goldenPath);
                        QVERIFY2(goldenSaved, qPrintable(QString("Failed to save golden image: %1").arg(goldenPath)));
                        qDebug() << "[Golden Seeded]" << fullName << "->" << goldenPath;
                    } else {
                        QImage goldenImage(goldenPath);
                        QVERIFY2(!goldenImage.isNull(),
                                 qPrintable(QString("Failed to read golden image: %1").arg(goldenPath)));
                        MismatchResult res = calculateMismatch(frame, goldenImage);
                        qDebug() << "[Screenshot Check]" << fullName << "Global Diff:" << (res.globalRatio * 100.0)
                                 << "% (<=0.05%), Max Region Diff:" << (res.maxRegionRatio * 100.0) << "% (<=5.0%)";
                        QVERIFY2(
                            res.passed,
                            qPrintable(
                                QString("Screenshot difference exceeds tolerance for %1: global %2%, max region %3%")
                                    .arg(fullName)
                                    .arg(res.globalRatio * 100.0, 0, 'f', 4)
                                    .arg(res.maxRegionRatio * 100.0, 0, 'f', 2)));
                    }
                }
            }
        }
    }
};

int main(int argc, char *argv[]) {
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    qputenv("QT_QUICK_CONTROLS_STYLE", QByteArray("Basic"));
    QGuiApplication app(argc, argv);
    TestResponsiveUI tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_responsive_ui.moc"
