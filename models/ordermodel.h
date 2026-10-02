/**
 * @file ordermodel.h
 * @brief Multi-role order queue management with OrderStateMachine validation and polling sync.
 * @layer ViewModels / Presentation Models (Layer 2)
 *
 * Public API Summary:
 * - rowCount(), data(), roleNames(): QAbstractListModel interface
 * - isLoading, count, errorMessage
 * - fetchOrders(isOnlineRider): Async fetch active order queue
 * - populateFromJson(arr): Direct population from JSON records
 * - updateOrderStatus(orderId, nextStatus, actorStr): Validated transition via OrderStateMachine
 * - assignRiderToOrder(orderId, riderId): Assign courier to ready order
 * - startPolling(intervalMs), stopPolling(): Background polling sync across roles
 * - getOrderAt(index): Direct order map access
 *
 * Dependencies:
 * - core/orderstatemachine.h, api/networkmanager.h
 *
 * Tests:
 * - Covered by tests/cpp/test_merchant_flow.cpp, tests/cpp/test_courier_flow.cpp
 */

#ifndef ORDERMODEL_H
#define ORDERMODEL_H

#include <QtCore/QAbstractListModel>
#include <QtCore/QString>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QVector>
#include <QtCore/QTimer>
#include "../core/orderstatemachine.h"

struct OrderRecordData {
    QString id;
    QString customerId;
    QString shopId;
    QString shopName;
    QString deliveryBoyId;
    QString address;
    QString status;
    double subtotal{0.0};
    double deliveryFee{50.0};
    double total{0.0};
    QString createdAt;
    QVariantList items;
};

class OrderModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

public:
    enum OrderRoles {
        IdRole = Qt::UserRole + 1,
        CustomerIdRole,
        ShopIdRole,
        ShopNameRole,
        DeliveryBoyIdRole,
        AddressRole,
        StatusRole,
        SubtotalRole,
        DeliveryFeeRole,
        TotalRole,
        CreatedAtRole,
        ItemsRole
    };

    explicit OrderModel(QObject *parent = nullptr);
    ~OrderModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::UserRole + 1) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isLoading() const;
    int count() const;
    QString errorMessage() const;

    Q_INVOKABLE void fetchOrders(bool isOnlineRider = true);
    Q_INVOKABLE void updateOrderStatus(const QString &orderId, const QString &nextStatus,
                                       const QString &actorRole = "merchant");
    Q_INVOKABLE void assignRiderToOrder(const QString &orderId, const QString &riderId);
    Q_INVOKABLE QVariantMap getOrderAt(int index) const;

    /**
     * @brief Direct model population from JSON array.
     */
    void populateFromJson(const QJsonArray &arr);

    /**
     * @brief Start automated polling for cross-role order synchronization.
     */
    Q_INVOKABLE void startPolling(int intervalMs = 3000);

    /**
     * @brief Stop polling timer.
     */
    Q_INVOKABLE void stopPolling();

signals:
    void loadingChanged();
    void countChanged();
    void errorChanged();
    void orderUpdated();

private:
    QVector<OrderRecordData> m_orders;
    bool m_isLoading{false};
    QString m_errorMessage;
    QTimer *m_pollTimer{nullptr};
};

#endif // ORDERMODEL_H
