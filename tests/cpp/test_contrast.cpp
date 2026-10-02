/**
 * @file test_contrast.cpp
 * @brief Automated WCAG 2.1 relative luminance and contrast ratio validation test.
 *        Parses token definitions directly from qml/theme/Theme.qml and enforces
 *        text/UI contrast and typography luminance hierarchies.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include <QtGui/QColor>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QMap>
#include <cmath>

class TestContrast : public QObject
{
    Q_OBJECT

private:
    QMap<QString, QColor> m_darkTokens;
    QMap<QString, QColor> m_lightTokens;

    static double sRGBtoLin(double c)
    {
        return (c <= 0.04045) ? (c / 12.92) : std::pow((c + 0.055) / 1.055, 2.4);
    }

    static double relativeLuminance(const QColor &color)
    {
        return 0.2126 * sRGBtoLin(color.redF()) +
               0.7152 * sRGBtoLin(color.greenF()) +
               0.0722 * sRGBtoLin(color.blueF());
    }

    static double contrastRatio(const QColor &c1, const QColor &c2)
    {
        double l1 = relativeLuminance(c1);
        double l2 = relativeLuminance(c2);
        return (std::max(l1, l2) + 0.05) / (std::min(l1, l2) + 0.05);
    }

    static void logTokenPair(const char* theme, const char* fgName, const QColor& fg,
                             const char* bgName, const QColor& bg, double minRequired)
    {
        double ratio = contrastRatio(fg, bg);
        const char* status = (ratio >= minRequired) ? "PASS" : "FAIL";
        printf("| %-6s | %-12s (%s) | %-12s (%s) | %6.2f:1 | %4.1f:1 | %-4s |\n",
               theme, fgName, fg.name().toLatin1().constData(),
               bgName, bg.name().toLatin1().constData(),
               ratio, minRequired, status);
    }

    void parseThemeQml()
    {
        QString path = QStringLiteral(QUICKCART_SOURCE_DIR) + QStringLiteral("/qml/theme/Theme.qml");
        QFile file(path);
        QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text),
                 qPrintable(QStringLiteral("Failed to open Theme.qml at: ") + path));

        QString content = QString::fromUtf8(file.readAll());
        file.close();

        // Matches: readonly property color <tokenName>: root.isDark ? "<darkVal>" : "<lightVal>"
        QRegularExpression re(QStringLiteral("readonly\\s+property\\s+color\\s+(\\w+)\\s*:\\s*root\\.isDark\\s*\\?\\s*\"([^\"]+)\"\\s*:\\s*\"([^\"]+)\""));
        QRegularExpressionMatchIterator it = re.globalMatch(content);

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString name = match.captured(1);
            QString darkHex = match.captured(2);
            QString lightHex = match.captured(3);

            m_darkTokens.insert(name, QColor(darkHex));
            m_lightTokens.insert(name, QColor(lightHex));
        }

        printf("\nParsed %d color tokens dynamically from %s\n",
               static_cast<int>(m_darkTokens.size()), path.toLatin1().constData());
    }

private slots:
    void initTestCase()
    {
        parseThemeQml();

        // Verify that critical tokens were successfully parsed from Theme.qml
        const QStringList requiredTokens = {
            QStringLiteral("background"),
            QStringLiteral("surface"),
            QStringLiteral("primary"),
            QStringLiteral("onPrimary"),
            QStringLiteral("danger"),
            QStringLiteral("onDanger"),
            QStringLiteral("warning"),
            QStringLiteral("onWarning"),
            QStringLiteral("textPrimary"),
            QStringLiteral("textSecondary"),
            QStringLiteral("textMuted"),
            QStringLiteral("border"),
            QStringLiteral("inputBorder")
        };

        for (const QString &token : requiredTokens) {
            QVERIFY2(m_darkTokens.contains(token), qPrintable(QString("Theme.qml missing dark token: ") + token));
            QVERIFY2(m_lightTokens.contains(token), qPrintable(QString("Theme.qml missing light token: ") + token));
        }

        printf("\n| Theme  | Foreground Token             | Background Token             | Ratio    | Target | Status |\n");
        printf("|--------|------------------------------|------------------------------|----------|--------|--------|\n");
    }

    void testDarkThemeTextContrastAndHierarchy()
    {
        QColor darkBg = m_darkTokens[QStringLiteral("background")];
        QColor darkSurface = m_darkTokens[QStringLiteral("surface")];

        QColor textPrimary = m_darkTokens[QStringLiteral("textPrimary")];
        QColor textSecondary = m_darkTokens[QStringLiteral("textSecondary")];
        QColor textMuted = m_darkTokens[QStringLiteral("textMuted")];

        logTokenPair("Dark", "textPrimary", textPrimary, "background", darkBg, 4.5);
        logTokenPair("Dark", "textPrimary", textPrimary, "surface", darkSurface, 4.5);
        logTokenPair("Dark", "textSecondary", textSecondary, "background", darkBg, 4.5);
        logTokenPair("Dark", "textSecondary", textSecondary, "surface", darkSurface, 4.5);
        logTokenPair("Dark", "textMuted", textMuted, "background", darkBg, 4.5);
        logTokenPair("Dark", "textMuted", textMuted, "surface", darkSurface, 4.5);

        // WCAG AA >= 4.5:1 text contrast checks
        QVERIFY2(contrastRatio(textPrimary, darkBg) >= 4.5, "Dark textPrimary on bg fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textPrimary, darkSurface) >= 4.5, "Dark textPrimary on surface fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textSecondary, darkBg) >= 4.5, "Dark textSecondary on bg fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textSecondary, darkSurface) >= 4.5, "Dark textSecondary on surface fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textMuted, darkBg) >= 4.5, "Dark textMuted on bg fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textMuted, darkSurface) >= 4.5, "Dark textMuted on surface fails WCAG AA 4.5:1");

        // Enforce strict perceptual hierarchy in Dark theme:
        // textPrimary (brightest) > textSecondary > textMuted (dimmest)
        double lumPrimary = relativeLuminance(textPrimary);
        double lumSecondary = relativeLuminance(textSecondary);
        double lumMuted = relativeLuminance(textMuted);

        QVERIFY2(lumPrimary > lumSecondary,
                 qPrintable(QString("Hierarchy violation: textPrimary lum (%1) <= textSecondary lum (%2)")
                            .arg(lumPrimary).arg(lumSecondary)));
        QVERIFY2(lumSecondary > lumMuted,
                 qPrintable(QString("Hierarchy violation: textSecondary lum (%1) <= textMuted lum (%2)")
                            .arg(lumSecondary).arg(lumMuted)));
    }

    void testLightThemeTextContrastAndHierarchy()
    {
        QColor lightBg = m_lightTokens[QStringLiteral("background")];
        QColor lightSurface = m_lightTokens[QStringLiteral("surface")];

        QColor textPrimary = m_lightTokens[QStringLiteral("textPrimary")];
        QColor textSecondary = m_lightTokens[QStringLiteral("textSecondary")];
        QColor textMuted = m_lightTokens[QStringLiteral("textMuted")];

        logTokenPair("Light", "textPrimary", textPrimary, "background", lightBg, 4.5);
        logTokenPair("Light", "textPrimary", textPrimary, "surface", lightSurface, 4.5);
        logTokenPair("Light", "textSecondary", textSecondary, "background", lightBg, 4.5);
        logTokenPair("Light", "textSecondary", textSecondary, "surface", lightSurface, 4.5);
        logTokenPair("Light", "textMuted", textMuted, "background", lightBg, 4.5);
        logTokenPair("Light", "textMuted", textMuted, "surface", lightSurface, 4.5);

        // WCAG AA >= 4.5:1 text contrast checks
        QVERIFY2(contrastRatio(textPrimary, lightBg) >= 4.5, "Light textPrimary on bg fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textPrimary, lightSurface) >= 4.5, "Light textPrimary on surface fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textSecondary, lightBg) >= 4.5, "Light textSecondary on bg fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textSecondary, lightSurface) >= 4.5, "Light textSecondary on surface fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textMuted, lightBg) >= 4.5, "Light textMuted on bg fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textMuted, lightSurface) >= 4.5, "Light textMuted on surface fails WCAG AA 4.5:1");

        // Enforce strict perceptual hierarchy in Light theme:
        // textPrimary (darkest) < textSecondary < textMuted (lightest)
        double lumPrimary = relativeLuminance(textPrimary);
        double lumSecondary = relativeLuminance(textSecondary);
        double lumMuted = relativeLuminance(textMuted);

        QVERIFY2(lumPrimary < lumSecondary,
                 qPrintable(QString("Hierarchy violation: textPrimary lum (%1) >= textSecondary lum (%2)")
                            .arg(lumPrimary).arg(lumSecondary)));
        QVERIFY2(lumSecondary < lumMuted,
                 qPrintable(QString("Hierarchy violation: textSecondary lum (%1) >= textMuted lum (%2)")
                            .arg(lumSecondary).arg(lumMuted)));
    }

    void testButtonAndActionContrast()
    {
        // Dark theme buttons
        QColor darkPrimary = m_darkTokens[QStringLiteral("primary")];
        QColor darkOnPrimary = m_darkTokens[QStringLiteral("onPrimary")];
        logTokenPair("Dark", "onPrimary", darkOnPrimary, "primary", darkPrimary, 4.5);
        QVERIFY2(contrastRatio(darkOnPrimary, darkPrimary) >= 4.5, "Dark onPrimary on primary fails 4.5:1");

        QColor darkDanger = m_darkTokens[QStringLiteral("danger")];
        QColor darkOnDanger = m_darkTokens[QStringLiteral("onDanger")];
        logTokenPair("Dark", "onDanger", darkOnDanger, "danger", darkDanger, 4.5);
        QVERIFY2(contrastRatio(darkOnDanger, darkDanger) >= 4.5, "Dark onDanger on danger fails 4.5:1");

        QColor darkWarning = m_darkTokens[QStringLiteral("warning")];
        QColor darkOnWarning = m_darkTokens[QStringLiteral("onWarning")];
        logTokenPair("Dark", "onWarning", darkOnWarning, "warning", darkWarning, 4.5);
        QVERIFY2(contrastRatio(darkOnWarning, darkWarning) >= 4.5, "Dark onWarning on warning fails 4.5:1");

        // Light theme buttons
        QColor lightPrimary = m_lightTokens[QStringLiteral("primary")];
        QColor lightOnPrimary = m_lightTokens[QStringLiteral("onPrimary")];
        logTokenPair("Light", "onPrimary", lightOnPrimary, "primary", lightPrimary, 4.5);
        QVERIFY2(contrastRatio(lightOnPrimary, lightPrimary) >= 4.5, "Light onPrimary on primary fails 4.5:1");

        QColor lightDanger = m_lightTokens[QStringLiteral("danger")];
        QColor lightOnDanger = m_lightTokens[QStringLiteral("onDanger")];
        logTokenPair("Light", "onDanger", lightOnDanger, "danger", lightDanger, 4.5);
        QVERIFY2(contrastRatio(lightOnDanger, lightDanger) >= 4.5, "Light onDanger on danger fails 4.5:1");

        QColor lightWarning = m_lightTokens[QStringLiteral("warning")];
        QColor lightOnWarning = m_lightTokens[QStringLiteral("onWarning")];
        logTokenPair("Light", "onWarning", lightOnWarning, "warning", lightWarning, 4.5);
        QVERIFY2(contrastRatio(lightOnWarning, lightWarning) >= 4.5, "Light onWarning on warning fails 4.5:1");
    }

    void testUIComponentBorders()
    {
        QColor darkBg = m_darkTokens[QStringLiteral("background")];
        QColor darkBorder = m_darkTokens[QStringLiteral("inputBorder")];
        logTokenPair("Dark", "border/input", darkBorder, "background", darkBg, 3.0);
        QVERIFY2(contrastRatio(darkBorder, darkBg) >= 3.0, "Dark border on bg fails WCAG UI 3.0:1");

        QColor lightBg = m_lightTokens[QStringLiteral("background")];
        QColor lightBorder = m_lightTokens[QStringLiteral("inputBorder")];
        logTokenPair("Light", "border/input", lightBorder, "background", lightBg, 3.0);
        QVERIFY2(contrastRatio(lightBorder, lightBg) >= 3.0, "Light border on bg fails WCAG UI 3.0:1");
        printf("\n");
    }
};

QTEST_MAIN(TestContrast)
#include "test_contrast.moc"
