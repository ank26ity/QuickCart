#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantMap>

class AppConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString environment READ environment WRITE setEnvironment NOTIFY environmentChanged)
    Q_PROPERTY(QString apiBaseUrl READ apiBaseUrl WRITE setApiBaseUrl NOTIFY apiBaseUrlChanged)
    Q_PROPERTY(QString wsBaseUrl READ wsBaseUrl NOTIFY wsBaseUrlChanged)
    Q_PROPERTY(int requestTimeoutMs READ requestTimeoutMs WRITE setRequestTimeoutMs NOTIFY timeoutChanged)
    Q_PROPERTY(int maxRetryAttempts READ maxRetryAttempts CONSTANT)
    Q_PROPERTY(bool isProduction READ isProduction NOTIFY environmentChanged)

public:
    enum class Environment {
        Development,
        Staging,
        Production
    };
    Q_ENUM(Environment)

    explicit AppConfig(QObject *parent = nullptr);
    static AppConfig* instance();

    QString environment() const;
    void setEnvironment(const QString &envName);

    Environment environmentType() const;
    bool isProduction() const;

    QString apiBaseUrl() const;
    void setApiBaseUrl(const QString &url);

    QString wsBaseUrl() const;
    int requestTimeoutMs() const;
    void setRequestTimeoutMs(int ms);

    int maxRetryAttempts() const;

    Q_INVOKABLE bool isFeatureEnabled(const QString &featureName) const;
    Q_INVOKABLE void setFeatureFlag(const QString &featureName, bool enabled);
    Q_INVOKABLE QVariantMap allFeatureFlags() const;

signals:
    void environmentChanged();
    void apiBaseUrlChanged();
    void wsBaseUrlChanged();
    void timeoutChanged();
    void featureFlagsChanged();

private:
    Environment m_env{Environment::Development};
    QString m_apiBaseUrl;
    QString m_wsBaseUrl;
    int m_timeoutMs{10000};
    int m_maxRetries{3};
    QVariantMap m_featureFlags;

    void updateUrlsForEnvironment();
};

#endif // APPCONFIG_H
