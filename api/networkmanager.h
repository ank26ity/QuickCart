#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QQueue>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <functional>
#include <memory>
#include "../core/result.h"
#include "circuitbreaker.h"
#include "networkinterceptor.h"

class NetworkManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(bool isOnline READ isOnline NOTIFY onlineStateChanged)

public:
    using JsonCallback = std::function<void(bool success, const QJsonDocument &doc, const QString &error)>;
    using ResultCallback = std::function<void(const Result<QJsonDocument> &result)>;

    explicit NetworkManager(QObject *parent = nullptr);
    static NetworkManager *instance();

    QString baseUrl() const;
    void setBaseUrl(const QString &url);

    bool isOnline() const;

    void addInterceptor(std::shared_ptr<INetworkInterceptor> interceptor);

    // Standard API methods (Backwards-compatible)
    void get(const QString &endpoint, JsonCallback callback);
    void post(const QString &endpoint, const QJsonObject &body, JsonCallback callback);
    void patch(const QString &endpoint, const QJsonObject &body, JsonCallback callback);
    void remove(const QString &endpoint, JsonCallback callback);

    // Typed Result API methods
    void executeGet(const QString &endpoint, ResultCallback callback);
    void executePost(const QString &endpoint, const QJsonObject &body, ResultCallback callback);
    void executePatch(const QString &endpoint, const QJsonObject &body, ResultCallback callback);
    void executeDelete(const QString &endpoint, ResultCallback callback);

    // Retry and Idempotency management
    void retryRequest(const QString &verb, const QString &endpoint, const QByteArray &data, int attempt,
                      ResultCallback callback);

    /**
     * @brief Reset network manager state and clear pending queues for test isolation.
     */
    void resetForTesting();
    void resetCircuitBreakers();

signals:
    void baseUrlChanged();
    void onlineStateChanged(bool isOnline);
    void requestFailed(const QString &endpoint, const QString &errorMsg);
    void tokenRefreshRequired();

private:
    struct PendingRequest {
        QString verb;
        QString endpoint;
        QByteArray data;
        int attempt{0};
        ResultCallback callback;
    };

    QNetworkAccessManager m_nam;
    QString m_baseUrl;
    bool m_isOnline{true};
    CircuitBreaker m_circuitBreaker;
    QList<std::shared_ptr<INetworkInterceptor>> m_interceptors;

    bool m_isRefreshingToken{false};
    QQueue<PendingRequest> m_refreshQueue;

    void prepareRequest(QNetworkRequest &request, const QString &verb, const QString &endpoint);
    void sendRequest(const QString &verb, const QString &endpoint, const QByteArray &data, int attempt,
                     ResultCallback callback);
    void handleTokenRefresh();
    void flushPendingQueue(bool success);
};

#endif // NETWORKMANAGER_H
