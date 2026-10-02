#include "logging.h"
#include <QtCore/QRegularExpression>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>

Q_LOGGING_CATEGORY(qcNetwork, "qc.network")
Q_LOGGING_CATEGORY(qcAuth, "qc.auth")
Q_LOGGING_CATEGORY(qcRbac, "qc.rbac")
Q_LOGGING_CATEGORY(qcStorage, "qc.storage")
Q_LOGGING_CATEGORY(qcOrder, "qc.order")

void StructuredLogger::init()
{
    // Enable verbose or debug logging rules as needed
    QLoggingCategory::setFilterRules(
        "qc.network.debug=true\n"
        "qc.auth.debug=true\n"
        "qc.rbac.debug=true\n"
        "qc.storage.debug=true\n"
        "qc.order.debug=true\n"
    );
}

QString StructuredLogger::maskPhone(const QString &phone)
{
    QString trimmed = phone.trimmed();
    if (trimmed.length() < 5) return QStringLiteral("***");
    return trimmed.left(2) + QString(trimmed.length() - 4, '*') + trimmed.right(2);
}

QString StructuredLogger::maskEmail(const QString &email)
{
    int atIdx = email.indexOf('@');
    if (atIdx <= 1) return QStringLiteral("***@***.***");
    QString userPart = email.left(atIdx);
    QString domainPart = email.mid(atIdx);
    if (userPart.length() <= 2) {
        return userPart.left(1) + QStringLiteral("***") + domainPart;
    }
    return userPart.left(2) + QString(userPart.length() - 2, '*') + domainPart;
}

QString StructuredLogger::maskPII(const QString &text)
{
    QString sanitized = text;
    // Mask Authorization Bearer tokens
    static const QRegularExpression bearerRegex(QStringLiteral("Bearer\\s+[A-Za-z0-9\\-_\\.]+"));
    sanitized.replace(bearerRegex, QStringLiteral("Bearer [REDACTED_JWT]"));

    // Mask passwords
    static const QRegularExpression passRegex(QStringLiteral("(\"password\"\\s*:\\s*\")[^\"]+(\")"));
    sanitized.replace(passRegex, QStringLiteral("\\1***REDACTED***\\2"));

    // Mask credit card patterns
    static const QRegularExpression cardRegex(QStringLiteral("\\b(?:\\d[ -]*?){13,16}\\b"));
    sanitized.replace(cardRegex, QStringLiteral("[REDACTED_PAN]"));

    return sanitized;
}

QJsonObject StructuredLogger::sanitizeJson(const QJsonObject &obj)
{
    QJsonObject copy = obj;
    const QStringList sensitiveKeys = {
        QStringLiteral("password"),
        QStringLiteral("token"),
        QStringLiteral("accessToken"),
        QStringLiteral("refreshToken"),
        QStringLiteral("secret"),
        QStringLiteral("licenseNumber"),
        QStringLiteral("rcNumber")
    };

    for (const QString &key : obj.keys()) {
        if (sensitiveKeys.contains(key, Qt::CaseInsensitive)) {
            copy[key] = QStringLiteral("[REDACTED]");
        } else if (key.compare(QStringLiteral("phone"), Qt::CaseInsensitive) == 0) {
            copy[key] = maskPhone(obj.value(key).toString());
        } else if (key.compare(QStringLiteral("email"), Qt::CaseInsensitive) == 0) {
            copy[key] = maskEmail(obj.value(key).toString());
        }
    }
    return copy;
}
