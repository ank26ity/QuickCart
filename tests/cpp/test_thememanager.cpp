/**
 * @file test_thememanager.cpp
 * @brief Automated unit test suite for ThemeManager persistence, toggling, and motion settings.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include <QtGui/QGuiApplication>
#include "../../core/thememanager.h"

class TestThemeManager : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        ThemeManager::instance()->resetForTesting();
    }

    void cleanup()
    {
        ThemeManager::instance()->resetForTesting();
    }

    void testDefaultMode()
    {
        ThemeManager *tm = ThemeManager::instance();
        QCOMPARE(tm->mode(), QStringLiteral("system"));
        QCOMPARE(tm->reducedMotion(), false);
    }

    void testSetLightMode()
    {
        ThemeManager *tm = ThemeManager::instance();
        QSignalSpy modeSpy(tm, &ThemeManager::modeChanged);
        QSignalSpy darkSpy(tm, &ThemeManager::isDarkChanged);

        tm->setMode(QStringLiteral("light"));

        QCOMPARE(tm->mode(), QStringLiteral("light"));
        QCOMPARE(tm->isDark(), false);
        QCOMPARE(modeSpy.count(), 1);
        QVERIFY(darkSpy.count() >= 1);
    }

    void testSetDarkMode()
    {
        ThemeManager *tm = ThemeManager::instance();
        tm->setMode(QStringLiteral("light")); // First set light

        QSignalSpy modeSpy(tm, &ThemeManager::modeChanged);
        QSignalSpy darkSpy(tm, &ThemeManager::isDarkChanged);

        tm->setMode(QStringLiteral("dark"));

        QCOMPARE(tm->mode(), QStringLiteral("dark"));
        QCOMPARE(tm->isDark(), true);
        QCOMPARE(modeSpy.count(), 1);
        QCOMPARE(darkSpy.count(), 1);
    }

    void testToggleTheme()
    {
        ThemeManager *tm = ThemeManager::instance();
        tm->setMode(QStringLiteral("dark"));
        QVERIFY(tm->isDark());

        tm->toggleTheme();
        QCOMPARE(tm->mode(), QStringLiteral("light"));
        QVERIFY(!tm->isDark());

        tm->toggleTheme();
        QCOMPARE(tm->mode(), QStringLiteral("dark"));
        QVERIFY(tm->isDark());
    }

    void testReducedMotion()
    {
        ThemeManager *tm = ThemeManager::instance();
        QSignalSpy motionSpy(tm, &ThemeManager::reducedMotionChanged);

        tm->setReducedMotion(true);
        QCOMPARE(tm->reducedMotion(), true);
        QCOMPARE(motionSpy.count(), 1);

        tm->setReducedMotion(false);
        QCOMPARE(tm->reducedMotion(), false);
        QCOMPARE(motionSpy.count(), 2);
    }

    void testPersistence()
    {
        ThemeManager::instance()->setMode(QStringLiteral("light"));
        ThemeManager::instance()->setReducedMotion(true);

        // Verify settings were written
        QSettings settings("QuickCart", "Theme");
        QCOMPARE(settings.value("themeMode").toString(), QStringLiteral("light"));
        QCOMPARE(settings.value("reducedMotion").toBool(), true);
    }

    void testLiveColorSchemeChange()
    {
        ThemeManager *tm = ThemeManager::instance();
        tm->setMode(QStringLiteral("system"));

        QSignalSpy darkSpy(tm, &ThemeManager::isDarkChanged);

        // Simulate live system color scheme changes if style hints are available
        if (QGuiApplication::styleHints()) {
            emit QGuiApplication::styleHints()->colorSchemeChanged(Qt::ColorScheme::Light);
            emit QGuiApplication::styleHints()->colorSchemeChanged(Qt::ColorScheme::Dark);
        }

        // System mode is preserved across live changes
        QCOMPARE(tm->mode(), QStringLiteral("system"));
    }
};

QTEST_MAIN(TestThemeManager)
#include "test_thememanager.moc"
