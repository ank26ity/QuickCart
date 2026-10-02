#include "circuitbreaker.h"
#include "../core/logging.h"
#include <QtCore/QRandomGenerator>

CircuitBreaker::CircuitBreaker(int failureThreshold, int recoveryTimeoutMs, QObject *parent)
    : QObject(parent), m_failureThreshold(failureThreshold), m_recoveryTimeoutMs(recoveryTimeoutMs) {}

QString CircuitBreaker::cleanEndpointKey(const QString &endpoint) const {
    int queryIdx = endpoint.indexOf('?');
    QString path = (queryIdx != -1) ? endpoint.left(queryIdx) : endpoint;
    // Group resource IDs e.g. /api/orders/123 -> /api/orders
    QStringList parts = path.split('/', Qt::SkipEmptyParts);
    if (parts.size() >= 2) {
        return "/" + parts[0] + "/" + parts[1];
    }
    return path;
}

bool CircuitBreaker::canExecute(const QString &endpoint) {
    QString key = cleanEndpointKey(endpoint);
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (!m_circuits.contains(key)) {
        return true;
    }

    CircuitEntry &entry = m_circuits[key];
    if (entry.state == State::Closed) {
        return true;
    }

    if (entry.state == State::Open) {
        if (now - entry.lastFailureTime > m_recoveryTimeoutMs) {
            entry.state = State::HalfOpen;
            entry.halfOpenSuccessCount = 0;
            qCDebug(qcNetwork) << "Circuit breaker for" << key << "transitioned from OPEN to HALF-OPEN";
            emit stateChanged(key, State::HalfOpen);
            return true;
        }
        qCWarning(qcNetwork) << "Circuit breaker fast-failed request to" << key << "(circuit is OPEN)";
        return false;
    }

    // HalfOpen allows probe traffic
    return true;
}

void CircuitBreaker::recordSuccess(const QString &endpoint) {
    QString key = cleanEndpointKey(endpoint);
    if (!m_circuits.contains(key))
        return;

    CircuitEntry &entry = m_circuits[key];
    if (entry.state == State::HalfOpen) {
        entry.halfOpenSuccessCount++;
        if (entry.halfOpenSuccessCount >= 2) {
            entry.state = State::Closed;
            entry.failureCount = 0;
            qCDebug(qcNetwork) << "Circuit breaker for" << key << "fully recovered to CLOSED";
            emit stateChanged(key, State::Closed);
        }
    } else if (entry.state == State::Closed) {
        entry.failureCount = 0;
    }
}

void CircuitBreaker::recordFailure(const QString &endpoint) {
    QString key = cleanEndpointKey(endpoint);
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    CircuitEntry &entry = m_circuits[key];
    entry.lastFailureTime = now;
    entry.failureCount++;

    if (entry.state == State::HalfOpen) {
        entry.state = State::Open;
        qCWarning(qcNetwork) << "Probe failed! Circuit breaker for" << key << "re-opened";
        emit stateChanged(key, State::Open);
    } else if (entry.state == State::Closed && entry.failureCount >= m_failureThreshold) {
        entry.state = State::Open;
        qCWarning(qcNetwork) << "Failure threshold reached (" << entry.failureCount << ")! Circuit OPEN for" << key;
        emit stateChanged(key, State::Open);
    }
}

CircuitBreaker::State CircuitBreaker::getState(const QString &endpoint) {
    QString key = cleanEndpointKey(endpoint);
    if (!m_circuits.contains(key))
        return State::Closed;
    CircuitEntry &entry = m_circuits[key];
    if (entry.state == State::Open) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - entry.lastFailureTime > m_recoveryTimeoutMs) {
            entry.state = State::HalfOpen;
        }
    }
    return entry.state;
}

int CircuitBreaker::calculateBackoffMs(int attempt, int baseDelayMs, int maxDelayMs) {
    int expDelay = baseDelayMs * (1 << qMin(attempt, 6)); // cap exponential multiplier at 64x
    int capped = qMin(expDelay, maxDelayMs);
    // Add 10-25% randomized full jitter
    int jitter = QRandomGenerator::global()->bounded(capped / 4 + 50);
    return capped + jitter;
}
