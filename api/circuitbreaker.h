#ifndef CIRCUITBREAKER_H
#define CIRCUITBREAKER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QMap>

class CircuitBreaker : public QObject {
    Q_OBJECT

public:
    enum class State {
        Closed,  // Normal
        Open,    // Tripped, fast-failing
        HalfOpen // Testing recovery
    };
    Q_ENUM(State)

    explicit CircuitBreaker(int failureThreshold = 5, int recoveryTimeoutMs = 15000, QObject *parent = nullptr);

    bool canExecute(const QString &endpoint);
    void recordSuccess(const QString &endpoint);
    void recordFailure(const QString &endpoint);
    State getState(const QString &endpoint);
    void resetAll();

    // Exponential backoff with jitter calculator
    static int calculateBackoffMs(int attempt, int baseDelayMs = 500, int maxDelayMs = 8000);

signals:
    void stateChanged(const QString &endpoint, State newState);

private:
    struct CircuitEntry {
        State state{State::Closed};
        int failureCount{0};
        qint64 lastFailureTime{0};
        int halfOpenSuccessCount{0};
    };

    int m_failureThreshold;
    int m_recoveryTimeoutMs;
    QMap<QString, CircuitEntry> m_circuits;

    QString cleanEndpointKey(const QString &endpoint) const;
};

#endif // CIRCUITBREAKER_H
