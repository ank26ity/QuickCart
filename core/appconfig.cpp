#include "appconfig.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QProcessEnvironment>

static AppConfig *s_appConfigInstance = nullptr;

AppConfig::AppConfig(QObject *parent)
    : QObject(parent)
{
    s_appConfigInstance = this;

    // Default feature flags
    m_featureFlags["enableBiometrics"] = true;
    m_featureFlags["enableLiveWebSocket"] = true;
    m_featureFlags["enableDynamicSurge"] = true;
    m_featureFlags["enableCourierBatching"] = true;
    m_featureFlags["enableOfflineOrderQueue"] = true;

    // Check environment variables
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString envVar = env.value(QStringLiteral("QUICKCART_ENV"), QStringLiteral("development")).toLower();
    setEnvironment(envVar);

    QString customApi = env.value(QStringLiteral("QUICKCART_API_URL"));
    if (!customApi.isEmpty()) {
        m_apiBaseUrl = customApi;
    }
}

AppConfig* AppConfig::instance()
{
    return s_appConfigInstance;
}

QString AppConfig::environment() const
{
    switch (m_env) {
    case Environment::Staging: return QStringLiteral("staging");
    case Environment::Production: return QStringLiteral("production");
    default: return QStringLiteral("development");
    }
}

void AppConfig::setEnvironment(const QString &envName)
{
    QString lower = envName.toLower();
    if (lower == QStringLiteral("production") || lower == QStringLiteral("prod")) {
        m_env = Environment::Production;
    } else if (lower == QStringLiteral("staging")) {
        m_env = Environment::Staging;
    } else {
        m_env = Environment::Development;
    }
    updateUrlsForEnvironment();
    emit environmentChanged();
}

AppConfig::Environment AppConfig::environmentType() const
{
    return m_env;
}

bool AppConfig::isProduction() const
{
    return m_env == Environment::Production;
}

QString AppConfig::apiBaseUrl() const
{
    return m_apiBaseUrl;
}

void AppConfig::setApiBaseUrl(const QString &url)
{
    if (m_apiBaseUrl != url) {
        m_apiBaseUrl = url;
        emit apiBaseUrlChanged();
    }
}

QString AppConfig::wsBaseUrl() const
{
    return m_wsBaseUrl;
}

int AppConfig::requestTimeoutMs() const
{
    return m_timeoutMs;
}

void AppConfig::setRequestTimeoutMs(int ms)
{
    if (m_timeoutMs != ms) {
        m_timeoutMs = ms;
        emit timeoutChanged();
    }
}

int AppConfig::maxRetryAttempts() const
{
    return m_maxRetries;
}

bool AppConfig::isFeatureEnabled(const QString &featureName) const
{
    return m_featureFlags.value(featureName, false).toBool();
}

void AppConfig::setFeatureFlag(const QString &featureName, bool enabled)
{
    if (m_featureFlags.value(featureName) != enabled) {
        m_featureFlags[featureName] = enabled;
        emit featureFlagsChanged();
    }
}

QVariantMap AppConfig::allFeatureFlags() const
{
    return m_featureFlags;
}

void AppConfig::updateUrlsForEnvironment()
{
    switch (m_env) {
    case Environment::Production:
        m_apiBaseUrl = QStringLiteral("https://api.quickcart.delivery");
        m_wsBaseUrl = QStringLiteral("wss://realtime.quickcart.delivery/ws");
        m_timeoutMs = 12000;
        break;
    case Environment::Staging:
        m_apiBaseUrl = QStringLiteral("https://staging-api.quickcart.delivery");
        m_wsBaseUrl = QStringLiteral("wss://staging-realtime.quickcart.delivery/ws");
        m_timeoutMs = 15000;
        break;
    case Environment::Development:
    default:
        m_apiBaseUrl = QStringLiteral("http://localhost:5001");
        m_wsBaseUrl = QStringLiteral("ws://localhost:5001/ws");
        m_timeoutMs = 10000;
        break;
    }
    emit apiBaseUrlChanged();
    emit wsBaseUrlChanged();
}
