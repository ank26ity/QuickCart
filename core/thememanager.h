/**
 * @file thememanager.h
 * @brief Persistent Theme Manager and System Color Scheme Synchronizer.
 * @layer Core / ViewModels (C++)
 * @dependencies QtCore, QtGui (QStyleHints), QSettings
 * @tests Covered by test_thememanager.cpp
 */

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

// ── 1. Includes ────────────────────────────────────────────────────────────
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtGui/QStyleHints>
#include <QtGui/QGuiApplication>
#include <QtQml/qqmlregistration.h>
#include <QtQml/QQmlEngine>
#include <QtQml/QJSEngine>

// ── 2. Class Declaration ───────────────────────────────────────────────────
class ThemeManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // ── 3. Q_PROPERTYs ─────────────────────────────────────────────────────
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(bool isDark READ isDark NOTIFY isDarkChanged)
    Q_PROPERTY(QString accentRole READ accentRole WRITE setAccentRole NOTIFY accentRoleChanged)
    Q_PROPERTY(bool reducedMotion READ reducedMotion NOTIFY reducedMotionChanged)

public:
    enum class ThemeMode {
        Light,
        Dark,
        System
    };
    Q_ENUM(ThemeMode)

    // ── 4. Public API ──────────────────────────────────────────────────────
    explicit ThemeManager(QObject *parent = nullptr);
    static ThemeManager* instance();

    /**
     * Factory function for Qt QML singleton registration
     */
    static ThemeManager* create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(qmlEngine);
        Q_UNUSED(jsEngine);
        return instance();
    }

    /**
     * Gets the active theme mode ("light", "dark", "system").
     */
    QString mode() const;

    /**
     * Sets the target theme mode persistently.
     */
    Q_INVOKABLE void setMode(const QString &modeStr);

    /**
     * Whether the active rendered appearance is dark.
     */
    bool isDark() const;

    /**
     * Gets the accent role identifier.
     */
    QString accentRole() const;
    void setAccentRole(const QString &role);

    /**
     * Whether reduced motion animations are active.
     */
    bool reducedMotion() const;
    void setReducedMotion(bool enabled);

    /**
     * Toggles between Light and Dark mode.
     */
    Q_INVOKABLE void toggleTheme();

    /**
     * Resets the singleton state for automated testing isolation.
     */
    void resetForTesting();

    // ── 5. Signals ─────────────────────────────────────────────────────────
signals:
    void modeChanged();
    void isDarkChanged();
    void accentRoleChanged();
    void reducedMotionChanged();

    // ── 6. Private Helpers & Members ───────────────────────────────────────
private:
    ThemeMode m_mode{ThemeMode::System};
    bool m_isDark{true};
    QString m_accentRole{QStringLiteral("customer")};
    bool m_reducedMotion{false};

    void loadSettings();
    void saveSettings();
    void updateEffectiveTheme();
    bool querySystemDarkMode() const;
};

#endif // THEMEMANAGER_H
