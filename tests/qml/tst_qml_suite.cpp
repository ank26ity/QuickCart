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
#include "../../core/thememanager.h"

class Setup : public QObject
{
    Q_OBJECT
public:
    Setup() {
        qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    }

public slots:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        engine->addImportPath(QStringLiteral(QUICKCART_IMPORT_DIR));
        engine->addImportPath(QStringLiteral(QUICKCART_SOURCE_DIR "/qml"));
        engine->addImportPath(QStringLiteral(QUICKCART_SOURCE_DIR));

        ThemeManager *tm = ThemeManager::instance();
        engine->rootContext()->setContextProperty(QStringLiteral("themeManager"), tm);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(QuickTests, Setup)
#include "tst_qml_suite.moc"
