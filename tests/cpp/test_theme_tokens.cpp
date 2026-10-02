/**
 * @file test_theme_tokens.cpp
 * @brief Automated unit test suite verifying completeness, validity, and contrast of Theme.qml tokens.
 * @layer Tests (C++ / QML / Qt Test)
 */

#include <QtTest/QtTest>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtGui/QColor>
#include <QtCore/QDir>
#include "../../core/thememanager.h"

class TestThemeTokens : public QObject {
    Q_OBJECT

private:
    QString m_sourceDir;

private slots:
    void initTestCase() { m_sourceDir = QStringLiteral(QUICKCART_SOURCE_DIR); }

    void testTokenCompletenessAndValidity() {
        QQmlEngine engine;
        engine.addImportPath(m_sourceDir + QStringLiteral("/qml"));
        engine.addImportPath(m_sourceDir + QStringLiteral("/qml/theme"));

        QQmlComponent component(&engine, QUrl::fromLocalFile(m_sourceDir + QStringLiteral("/qml/theme/Theme.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));

        QObject *theme = component.create();
        QVERIFY(theme != nullptr);

        ThemeManager *tm = ThemeManager::instance();
        theme->setProperty("themeManager", QVariant::fromValue(tm));

        const QStringList requiredColorTokens = {
            "background",      "surface",     "surfaceVariant",  "surfaceGlass",   "surfaceHeader",
            "surfaceBorder",   "border",      "divider",         "overlay",        "primary",
            "primaryHover",    "onPrimary",   "secondary",       "secondaryHover", "onSecondary",
            "danger",          "dangerHover", "onDanger",        "warning",        "warningHover",
            "onWarning",       "success",     "onSuccess",       "textPrimary",    "textSecondary",
            "textMuted",       "textInverse", "inputBackground", "inputBorder",    "inputBorderFocus",
            "inputPlaceholder"};

        // Test in both Light and Dark mode
        for (const QString &mode : {QStringLiteral("dark"), QStringLiteral("light")}) {
            tm->setMode(mode);

            for (const QString &token : requiredColorTokens) {
                QVariant val = theme->property(token.toUtf8().constData());
                QVERIFY2(val.isValid(), qPrintable(QString("Token %1 is missing in mode %2").arg(token, mode)));
                QColor col = val.value<QColor>();
                if (!col.isValid() && val.canConvert<QString>()) {
                    col = QColor::fromString(val.toString());
                }
                QVERIFY2(
                    col.isValid(),
                    qPrintable(
                        QString("Token %1 has invalid color in mode %2 (val=%3)").arg(token, mode, val.toString())));
            }

            // Verify Spacing Tokens
            for (const QString &sp :
                 {"space2", "space4", "space8", "space12", "space16", "space20", "space24", "space32", "space48"}) {
                QVariant val = theme->property(sp.toUtf8().constData());
                QVERIFY2(val.isValid() && val.toDouble() > 0, qPrintable(QString("Spacing token %1 invalid").arg(sp)));
            }

            // Verify Radius Tokens
            for (const QString &rad : {"radiusSmall", "radiusMedium", "radiusLarge", "radiusXLarge", "radiusFull"}) {
                QVariant val = theme->property(rad.toUtf8().constData());
                QVERIFY2(val.isValid() && val.toDouble() > 0, qPrintable(QString("Radius token %1 invalid").arg(rad)));
            }

            // Verify Typography Scale
            for (const QString &f :
                 {"fontSmall", "fontBody", "fontSubheading", "fontHeading", "fontTitle", "fontDisplay"}) {
                QVariant val = theme->property(f.toUtf8().constData());
                QVERIFY2(val.isValid() && val.toInt() >= 10, qPrintable(QString("Font token %1 invalid").arg(f)));
            }
        }

        delete theme;
    }
};

QTEST_MAIN(TestThemeTokens)
#include "test_theme_tokens.moc"
