#include "networkmanager.h"
#include "../core/appconfig.h"
#include "../core/logging.h"
#include "../security/securestorage.h"
#include <QtCore/QUrl>
#include <QtCore/QUuid>
#include <QtCore/QSysInfo>
#include <QtCore/QTimer>
#include <QtCore/QJsonDocument>
#include <QtConcurrent/QtConcurrent>

static NetworkManager *s_networkManagerInstance = nullptr;

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent), m_baseUrl(QStringLiteral("http://localhost:5001")), m_circuitBreaker(5, 15000, this) {
    s_networkManagerInstance = this;
    if (AppConfig::instance()) {
        m_baseUrl = AppConfig::instance()->apiBaseUrl();
        connect(AppConfig::instance(), &AppConfig::apiBaseUrlChanged, this,
                [this]() { setBaseUrl(AppConfig::instance()->apiBaseUrl()); });
    }
}

NetworkManager *NetworkManager::instance() {
    if (!s_networkManagerInstance) {
        new NetworkManager(qApp);
    }
    return s_networkManagerInstance;
}

void NetworkManager::resetForTesting() {
    m_refreshQueue.clear();
    m_isRefreshingToken = false;
    m_interceptors.clear();
    m_circuitBreaker.resetAll();
}

void NetworkManager::resetCircuitBreakers() {
    m_circuitBreaker.resetAll();
}

QString NetworkManager::baseUrl() const {
    return m_baseUrl;
}

void NetworkManager::setBaseUrl(const QString &url) {
    if (m_baseUrl != url) {
        m_baseUrl = url;
        emit baseUrlChanged();
    }
}

bool NetworkManager::isOnline() const {
    return m_isOnline;
}

void NetworkManager::addInterceptor(std::shared_ptr<INetworkInterceptor> interceptor) {
    m_interceptors.append(interceptor);
}

void NetworkManager::prepareRequest(QNetworkRequest &request, const QString &verb, const QString &endpoint) {
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("X-Client-Platform", QSysInfo::prettyProductName().toUtf8());
    request.setRawHeader("X-App-Version", "1.0.0");

    // Inject Bearer access token if stored securely
    if (SecureStorage::instance()) {
        QString token = SecureStorage::instance()->accessToken();
        if (!token.isEmpty()) {
            request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());
        }
    }

    // Inject Idempotency key for mutating requests to prevent duplicate orders/charges
    if (verb == QStringLiteral("POST") || verb == QStringLiteral("PATCH")) {
        QString idempotencyKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
        request.setRawHeader("X-Idempotency-Key", idempotencyKey.toUtf8());
    }
}

void NetworkManager::retryRequest(const QString &verb, const QString &endpoint, const QByteArray &data, int attempt,
                                  ResultCallback callback) {
    sendRequest(verb, endpoint, data, attempt, callback);
}

void NetworkManager::sendRequest(const QString &verb, const QString &endpoint, const QByteArray &data, int attempt,
                                 ResultCallback callback) {
    // 1. Check Circuit Breaker
    if (!m_circuitBreaker.canExecute(endpoint)) {
        AppError err =
            AppError::network(QStringLiteral("Circuit breaker open for endpoint (service experiencing degradation)"));
        qCWarning(qcNetwork) << "[CIRCUIT-OPEN]" << verb << endpoint;
        callback(Result<QJsonDocument>::error(err));
        return;
    }

    // 2. Prepare URL & Network Request
    QUrl url = QUrl(m_baseUrl).resolved(QUrl(endpoint));
    QNetworkRequest request(url);
    prepareRequest(request, verb, endpoint);

    for (const auto &interceptor : m_interceptors) {
        interceptor->onRequest(request, data);
    }

    qCDebug(qcNetwork) << "[REQ]" << verb << url.toString() << "(Attempt:" << attempt + 1 << ")";

    // 3. Dispatch HTTP request
    QNetworkReply *reply = nullptr;
    if (verb == QStringLiteral("GET")) {
        reply = m_nam.get(request);
    } else if (verb == QStringLiteral("POST")) {
        reply = m_nam.post(request, data);
    } else if (verb == QStringLiteral("PATCH")) {
        reply = m_nam.sendCustomRequest(request, "PATCH", data);
    } else if (verb == QStringLiteral("DELETE")) {
        reply = m_nam.deleteResource(request);
    }

    if (!reply) {
        callback(Result<QJsonDocument>::error(AppError::network(QStringLiteral("Failed to dispatch request"))));
        return;
    }

    // Ensure deleteLater is called on every QNetworkReply immediately
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);

    // 4. Handle HTTP Response
    connect(reply, &QNetworkReply::finished, this, [this, reply, verb, endpoint, data, attempt, callback]() {
        QByteArray respBytes = reply->readAll();
        int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        for (const auto &interceptor : m_interceptors) {
            interceptor->onResponse(reply, respBytes);
        }

        // 4A. Handle 401 Unauthorized with token refresh rotation (exclude auth endpoints and only attempt once)
        bool isAuthEndpoint =
            endpoint.contains(QStringLiteral("/auth/login")) || endpoint.contains(QStringLiteral("/auth/refresh")) ||
            endpoint.contains(QStringLiteral("/auth/signup")) || endpoint.contains(QStringLiteral("/auth/otp")) ||
            endpoint.contains(QStringLiteral("/admin/audit-logs"));
        if (httpCode == 401 && attempt == 0 && !isAuthEndpoint && SecureStorage::instance() &&
            !SecureStorage::instance()->refreshToken().isEmpty()) {
            qCWarning(qcAuth) << "Received 401 Unauthorized for" << endpoint << "- Queuing for token refresh";
            PendingRequest pr{verb, endpoint, data, attempt + 1, callback};
            m_refreshQueue.enqueue(pr);
            if (!m_isRefreshingToken) {
                handleTokenRefresh();
            }
            return;
        }

        // 4B. Handle transient server errors (NEVER retry 4xx client errors)
        bool isClientError = (httpCode >= 400 && httpCode < 500);
        bool isTransientError = !isClientError && (httpCode == 502 || httpCode == 503 || httpCode == 504 ||
                                                   reply->error() == QNetworkReply::TimeoutError);
        int maxRetries = AppConfig::instance() ? AppConfig::instance()->maxRetryAttempts() : 3;

        if (isTransientError && attempt < maxRetries) {
            m_circuitBreaker.recordFailure(endpoint);
            int backoffDelay = CircuitBreaker::calculateBackoffMs(attempt);
            qCWarning(qcNetwork) << "Transient error (" << httpCode << ") on" << endpoint << "- Retrying in"
                                 << backoffDelay << "ms (Attempt" << attempt + 1 << "/" << maxRetries << ")";
            QTimer::singleShot(backoffDelay, this, [this, verb, endpoint, data, attempt, callback]() {
                sendRequest(verb, endpoint, data, attempt + 1, callback);
            });
            return;
        }

        // 4C. Check standard network errors
        if (reply->error() != QNetworkReply::NoError && (httpCode < 200 || httpCode >= 300)) {
            m_circuitBreaker.recordFailure(endpoint);
            QString errText = reply->errorString();
            if (!respBytes.isEmpty()) {
                QJsonDocument errDoc = QJsonDocument::fromJson(respBytes);
                if (errDoc.isObject() && errDoc.object().contains("message")) {
                    errText = errDoc.object().value("message").toString();
                }
            }
            qCWarning(qcNetwork) << "[FAIL]" << verb << endpoint << "Status:" << httpCode << "Error:" << errText;
            emit requestFailed(endpoint, errText);

            AppError appErr{ErrorCategory::Network, httpCode, errText, QString(),
                            QStringLiteral("ERR_HTTP_") + QString::number(httpCode)};
            callback(Result<QJsonDocument>::error(appErr));
            return;
        }

        // 4D. Parse JSON in-thread
        m_circuitBreaker.recordSuccess(endpoint);
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(respBytes, &parseError);
        if (parseError.error != QJsonParseError::NoError && !respBytes.isEmpty()) {
            AppError err = AppError::server(QStringLiteral("JSON parse error: ") + parseError.errorString());
            callback(Result<QJsonDocument>::error(err));
        } else {
            callback(Result<QJsonDocument>::ok(doc));
        }
    });
}

void NetworkManager::handleTokenRefresh() {
    m_isRefreshingToken = true;
    QString refToken = SecureStorage::instance()->refreshToken();
    if (refToken.isEmpty()) {
        m_isRefreshingToken = false;
        flushPendingQueue(false);
        return;
    }

    QJsonObject body;
    body["refreshToken"] = refToken;
    body["refresh_token"] = refToken;
    QByteArray bodyBytes = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QUrl url = QUrl(m_baseUrl).resolved(QUrl(QStringLiteral("/api/auth/refresh")));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = m_nam.post(request, bodyBytes);
    if (!reply) {
        m_isRefreshingToken = false;
        flushPendingQueue(false);
        return;
    }

    // Guard refresh call with a 2-second timeout to prevent stalling
    QTimer *refreshTimeout = new QTimer(reply);
    refreshTimeout->setSingleShot(true);
    connect(refreshTimeout, &QTimer::timeout, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });
    refreshTimeout->start(2000);

    connect(reply, &QNetworkReply::finished, this, [this, reply, refreshTimeout]() {
        refreshTimeout->stop();
        reply->deleteLater();
        int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        m_isRefreshingToken = false;

        if (httpCode >= 200 && httpCode < 300) {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QString newAccess =
                    doc.object().value("accessToken").toString(doc.object().value("access_token").toString());
                QString newRefresh =
                    doc.object().value("refreshToken").toString(doc.object().value("refresh_token").toString());
                if (!newAccess.isEmpty()) {
                    SecureStorage::instance()->saveTokens(
                        newAccess, newRefresh.isEmpty() ? SecureStorage::instance()->refreshToken() : newRefresh);
                    qCDebug(qcAuth) << "Token refresh successful, replay pending queued requests";
                    flushPendingQueue(true);
                    return;
                }
            }
        }

        qCWarning(qcAuth) << "Token refresh failed or expired! Flushing queue and requiring re-login";
        SecureStorage::instance()->clearTokens();
        emit tokenRefreshRequired();
        flushPendingQueue(false);
    });
}

void NetworkManager::flushPendingQueue(bool success) {
    while (!m_refreshQueue.isEmpty()) {
        PendingRequest pr = m_refreshQueue.dequeue();
        if (success) {
            sendRequest(pr.verb, pr.endpoint, pr.data, pr.attempt, pr.callback);
        } else {
            pr.callback(Result<QJsonDocument>::error(
                AppError::auth(QStringLiteral("Unauthorized: Session expired (HTTP 401). Please log in again."))));
        }
    }
}

// Typed Result methods
void NetworkManager::executeGet(const QString &endpoint, ResultCallback callback) {
    sendRequest(QStringLiteral("GET"), endpoint, QByteArray(), 0, callback);
}

void NetworkManager::executePost(const QString &endpoint, const QJsonObject &body, ResultCallback callback) {
    QByteArray data = QJsonDocument(body).toJson(QJsonDocument::Compact);
    sendRequest(QStringLiteral("POST"), endpoint, data, 0, callback);
}

void NetworkManager::executePatch(const QString &endpoint, const QJsonObject &body, ResultCallback callback) {
    QByteArray data = QJsonDocument(body).toJson(QJsonDocument::Compact);
    sendRequest(QStringLiteral("PATCH"), endpoint, data, 0, callback);
}

void NetworkManager::executeDelete(const QString &endpoint, ResultCallback callback) {
    sendRequest(QStringLiteral("DELETE"), endpoint, QByteArray(), 0, callback);
}

// Backwards compatibility layer
void NetworkManager::get(const QString &endpoint, JsonCallback callback) {
    executeGet(endpoint, [callback](const Result<QJsonDocument> &res) {
        if (res.isSuccess()) {
            callback(true, res.value(), QString());
        } else {
            callback(false, QJsonDocument(), res.error().message);
        }
    });
}

void NetworkManager::post(const QString &endpoint, const QJsonObject &body, JsonCallback callback) {
    executePost(endpoint, body, [callback](const Result<QJsonDocument> &res) {
        if (res.isSuccess()) {
            callback(true, res.value(), QString());
        } else {
            callback(false, QJsonDocument(), res.error().message);
        }
    });
}

void NetworkManager::patch(const QString &endpoint, const QJsonObject &body, JsonCallback callback) {
    executePatch(endpoint, body, [callback](const Result<QJsonDocument> &res) {
        if (res.isSuccess()) {
            callback(true, res.value(), QString());
        } else {
            callback(false, QJsonDocument(), res.error().message);
        }
    });
}

void NetworkManager::remove(const QString &endpoint, JsonCallback callback) {
    executeDelete(endpoint, [callback](const Result<QJsonDocument> &res) {
        if (res.isSuccess()) {
            callback(true, res.value(), QString());
        } else {
            callback(false, QJsonDocument(), res.error().message);
        }
    });
}
