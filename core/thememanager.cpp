/**
 * @file thememanager.cpp
 * @brief Implementation of ThemeManager.
 * @layer Core / ViewModels (C++)
 * @tests Covered by test_thememanager.cpp
 */

#include "thememanager.h"
#include <QtCore/QSettings>
#include <QtCore/QDebug>

static ThemeManager *s_themeManagerInstance = nullptr;

ThemeManager::ThemeManager(QObject *parent) : QObject(parent) {
    s_themeManagerInstance = this;

    // Listen to OS system color scheme changes live
    if (QGuiApplication::styleHints()) {
        connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme scheme) {
            Q_UNUSED(scheme);
            if (m_mode == ThemeMode::System) {
                updateEffectiveTheme();
            }
        });
    }

    loadSettings();
}

ThemeManager *ThemeManager::instance() {
    if (!s_themeManagerInstance) {
        new ThemeManager(qApp);
    }
    return s_themeManagerInstance;
}

QString ThemeManager::mode() const {
    switch (m_mode) {
        case ThemeMode::Light:
            return QStringLiteral("light");
        case ThemeMode::Dark:
            return QStringLiteral("dark");
        case ThemeMode::System:
            return QStringLiteral("system");
    }
    return QStringLiteral("system");
}

void ThemeManager::setMode(const QString &modeStr) {
    ThemeMode targetMode = ThemeMode::System;
    QString lower = modeStr.trimmed().toLower();
    if (lower == QStringLiteral("light"))
        targetMode = ThemeMode::Light;
    else if (lower == QStringLiteral("dark"))
        targetMode = ThemeMode::Dark;

    if (m_mode != targetMode) {
        m_mode = targetMode;
        saveSettings();
        emit modeChanged();
        updateEffectiveTheme();
    }
}

bool ThemeManager::isDark() const {
    return m_isDark;
}

QString ThemeManager::accentRole() const {
    return m_accentRole;
}

void ThemeManager::setAccentRole(const QString &role) {
    if (m_accentRole != role) {
        m_accentRole = role.toLower();
        emit accentRoleChanged();
    }
}

bool ThemeManager::reducedMotion() const {
    return m_reducedMotion;
}

void ThemeManager::setReducedMotion(bool enabled) {
    if (m_reducedMotion != enabled) {
        m_reducedMotion = enabled;
        saveSettings();
        emit reducedMotionChanged();
    }
}

void ThemeManager::toggleTheme() {
    if (m_isDark) {
        setMode(QStringLiteral("light"));
    } else {
        setMode(QStringLiteral("dark"));
    }
}

void ThemeManager::resetForTesting() {
    m_mode = ThemeMode::System;
    m_reducedMotion = false;
    m_accentRole = QStringLiteral("customer");
    QSettings settings("QuickCart", "Theme");
    settings.clear();
    updateEffectiveTheme();
    emit modeChanged();
    emit reducedMotionChanged();
    emit accentRoleChanged();
}

bool ThemeManager::querySystemDarkMode() const {
    if (QGuiApplication::styleHints()) {
        return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    }
    return true; // Default to dark theme if unavailable
}

void ThemeManager::updateEffectiveTheme() {
    bool newIsDark = true;
    switch (m_mode) {
        case ThemeMode::Light:
            newIsDark = false;
            break;
        case ThemeMode::Dark:
            newIsDark = true;
            break;
        case ThemeMode::System:
            newIsDark = querySystemDarkMode();
            break;
    }

    if (m_isDark != newIsDark) {
        m_isDark = newIsDark;
        emit isDarkChanged();
    }
}

void ThemeManager::loadSettings() {
    QSettings settings("QuickCart", "Theme");
    QString savedMode = settings.value("themeMode", "system").toString();
    m_reducedMotion = settings.value("reducedMotion", false).toBool();

    QString lower = savedMode.trimmed().toLower();
    if (lower == QStringLiteral("light"))
        m_mode = ThemeMode::Light;
    else if (lower == QStringLiteral("dark"))
        m_mode = ThemeMode::Dark;
    else
        m_mode = ThemeMode::System;

    updateEffectiveTheme();
}

void ThemeManager::saveSettings() {
    QSettings settings("QuickCart", "Theme");
    settings.setValue("themeMode", mode());
    settings.setValue("reducedMotion", m_reducedMotion);
}
