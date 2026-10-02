#ifndef LOGGING_H
#define LOGGING_H

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/QJsonObject>

Q_DECLARE_LOGGING_CATEGORY(qcNetwork)
Q_DECLARE_LOGGING_CATEGORY(qcAuth)
Q_DECLARE_LOGGING_CATEGORY(qcRbac)
Q_DECLARE_LOGGING_CATEGORY(qcStorage)
Q_DECLARE_LOGGING_CATEGORY(qcOrder)

class StructuredLogger
{
public:
    static void init();
    static QString maskPII(const QString &text);
    static QString maskPhone(const QString &phone);
    static QString maskEmail(const QString &email);
    static QJsonObject sanitizeJson(const QJsonObject &obj);
};

#endif // LOGGING_H
