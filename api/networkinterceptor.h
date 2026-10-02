#ifndef NETWORKINTERCEPTOR_H
#define NETWORKINTERCEPTOR_H

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QJsonObject>

class INetworkInterceptor {
public:
    virtual ~INetworkInterceptor() = default;
    virtual void onRequest(QNetworkRequest &request, const QByteArray &body) = 0;
    virtual void onResponse(QNetworkReply *reply, const QByteArray &responseBody) = 0;
};

#endif // NETWORKINTERCEPTOR_H
