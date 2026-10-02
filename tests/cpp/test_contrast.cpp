/**
 * @file test_contrast.cpp
 * @brief Automated WCAG 2.1 relative luminance and contrast ratio validation test.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include <QtGui/QColor>
#include <cmath>

class TestContrast : public QObject
{
    Q_OBJECT

private:
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

private slots:
    void initTestCase()
    {
        printf("\n| Theme  | Foreground Token             | Background Token             | Ratio    | Target | Status |\n");
        printf("|--------|------------------------------|------------------------------|----------|--------|--------|\n");
    }

    void testDarkThemeTextContrast()
    {
        QColor darkBg("#0b0f19");
        QColor darkSurface("#0f172a");

        QColor textPrimary("#f8fafc");
        QColor textSecondary("#94a3b8");
        QColor textMuted("#9ca3af");

        logTokenPair("Dark", "textPrimary", textPrimary, "background", darkBg, 4.5);
        logTokenPair("Dark", "textPrimary", textPrimary, "surface", darkSurface, 4.5);
        logTokenPair("Dark", "textSecondary", textSecondary, "background", darkBg, 4.5);
        logTokenPair("Dark", "textSecondary", textSecondary, "surface", darkSurface, 4.5);
        logTokenPair("Dark", "textMuted", textMuted, "background", darkBg, 4.5);
        logTokenPair("Dark", "textMuted", textMuted, "surface", darkSurface, 4.5);

        QVERIFY2(contrastRatio(textPrimary, darkBg) >= 4.5, "Dark textPrimary on bg fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textPrimary, darkSurface) >= 4.5, "Dark textPrimary on surface fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textSecondary, darkBg) >= 4.5, "Dark textSecondary on bg fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textSecondary, darkSurface) >= 4.5, "Dark textSecondary on surface fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textMuted, darkBg) >= 4.5, "Dark textMuted on bg fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textMuted, darkSurface) >= 4.5, "Dark textMuted on surface fails WCAG AA 4.5:1");
    }

    void testLightThemeTextContrast()
    {
        QColor lightBg("#f8fafc");
        QColor lightSurface("#ffffff");

        QColor textPrimary("#0f172a");
        QColor textSecondary("#334155");
        QColor textMuted("#475569");

        logTokenPair("Light", "textPrimary", textPrimary, "background", lightBg, 4.5);
        logTokenPair("Light", "textPrimary", textPrimary, "surface", lightSurface, 4.5);
        logTokenPair("Light", "textSecondary", textSecondary, "background", lightBg, 4.5);
        logTokenPair("Light", "textSecondary", textSecondary, "surface", lightSurface, 4.5);
        logTokenPair("Light", "textMuted", textMuted, "background", lightBg, 4.5);
        logTokenPair("Light", "textMuted", textMuted, "surface", lightSurface, 4.5);

        QVERIFY2(contrastRatio(textPrimary, lightBg) >= 4.5, "Light textPrimary on bg fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textPrimary, lightSurface) >= 4.5, "Light textPrimary on surface fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textSecondary, lightBg) >= 4.5, "Light textSecondary on bg fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textSecondary, lightSurface) >= 4.5, "Light textSecondary on surface fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textMuted, lightBg) >= 4.5, "Light textMuted on bg fails WCAG AA 4.5:1");
        QVERIFY2(contrastRatio(textMuted, lightSurface) >= 4.5, "Light textMuted on surface fails WCAG AA 4.5:1");
    }

    void testButtonAndActionContrast()
    {
        // Dark theme buttons
        QColor darkPrimary("#10b981");
        QColor darkOnPrimary("#022c22");
        logTokenPair("Dark", "onPrimary", darkOnPrimary, "primary", darkPrimary, 4.5);
        QVERIFY2(contrastRatio(darkOnPrimary, darkPrimary) >= 4.5, "Dark onPrimary on primary fails 4.5:1");

        QColor darkDanger("#ef4444");
        QColor darkOnDanger("#1a0303");
        logTokenPair("Dark", "onDanger", darkOnDanger, "danger", darkDanger, 4.5);
        QVERIFY2(contrastRatio(darkOnDanger, darkDanger) >= 4.5, "Dark onDanger on danger fails 4.5:1");

        QColor darkWarning("#f59e0b");
        QColor darkOnWarning("#451a03");
        logTokenPair("Dark", "onWarning", darkOnWarning, "warning", darkWarning, 4.5);
        QVERIFY2(contrastRatio(darkOnWarning, darkWarning) >= 4.5, "Dark onWarning on warning fails 4.5:1");

        // Light theme buttons
        QColor lightPrimary("#047857");
        QColor lightOnPrimary("#ffffff");
        logTokenPair("Light", "onPrimary", lightOnPrimary, "primary", lightPrimary, 4.5);
        QVERIFY2(contrastRatio(lightOnPrimary, lightPrimary) >= 4.5, "Light onPrimary on primary fails 4.5:1");

        QColor lightDanger("#b91c1c");
        QColor lightOnDanger("#ffffff");
        logTokenPair("Light", "onDanger", lightOnDanger, "danger", lightDanger, 4.5);
        QVERIFY2(contrastRatio(lightOnDanger, lightDanger) >= 4.5, "Light onDanger on danger fails 4.5:1");

        QColor lightWarning("#b45309");
        QColor lightOnWarning("#ffffff");
        logTokenPair("Light", "onWarning", lightOnWarning, "warning", lightWarning, 4.5);
        QVERIFY2(contrastRatio(lightOnWarning, lightWarning) >= 4.5, "Light onWarning on warning fails 4.5:1");
    }

    void testUIComponentBorders()
    {
        QColor darkBg("#0b0f19");
        QColor darkBorder("#64748b");
        logTokenPair("Dark", "border/input", darkBorder, "background", darkBg, 3.0);
        QVERIFY2(contrastRatio(darkBorder, darkBg) >= 3.0, "Dark border on bg fails WCAG UI 3.0:1");

        QColor lightBg("#f8fafc");
        QColor lightBorder("#64748b");
        logTokenPair("Light", "border/input", lightBorder, "background", lightBg, 3.0);
        QVERIFY2(contrastRatio(lightBorder, lightBg) >= 3.0, "Light border on bg fails WCAG UI 3.0:1");
        printf("\n");
    }
};

QTEST_MAIN(TestContrast)
#include "test_contrast.moc"
