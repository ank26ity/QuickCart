/**
 * @file ordermodel.cpp
 * @brief Implementation of OrderModel with OrderStateMachine validation and polling sync.
 * @layer ViewModels / Presentation Models (Layer 2)
 * @tests Covered by tests/cpp/test_merchant_flow.cpp, tests/cpp/test_courier_flow.cpp
 */

#include "ordermodel.h"
#include "../api/networkmanager.h"
#include <QtCore/QJsonDocument>
#include <QtCore/QThreadPool>
#include <QtCore/QPointer>
#include <QtConcurrent/QtConcurrent>

OrderModel::OrderModel(QObject *parent) : QAbstractListModel(parent), m_isLoading(false) {
    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, [this]() { fetchOrders(true); });
}

OrderModel::~OrderModel() {
    stopPolling();
}

int OrderModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return m_orders.size();
}

QVariant OrderModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_orders.size())
        return QVariant();

    const OrderRecordData &o = m_orders.at(index.row());
    switch (role) {
        case IdRole:
            return o.id;
        case CustomerIdRole:
            return o.customerId;
        case ShopIdRole:
            return o.shopId;
        case ShopNameRole:
            return o.shopName;
        case DeliveryBoyIdRole:
            return o.deliveryBoyId;
        case AddressRole:
            return o.address;
        case StatusRole:
            return o.status;
        case SubtotalRole:
            return o.subtotal;
        case DeliveryFeeRole:
            return o.deliveryFee;
        case TotalRole:
            return o.total;
        case CreatedAtRole:
            return o.createdAt;
        case ItemsRole:
            return o.items;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> OrderModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "orderId";
    roles[CustomerIdRole] = "customerId";
    roles[ShopIdRole] = "shopId";
    roles[ShopNameRole] = "shopName";
    roles[DeliveryBoyIdRole] = "deliveryBoyId";
    roles[AddressRole] = "address";
    roles[StatusRole] = "status";
    roles[SubtotalRole] = "subtotal";
    roles[DeliveryFeeRole] = "deliveryFee";
    roles[TotalRole] = "total";
    roles[CreatedAtRole] = "createdAt";
    roles[ItemsRole] = "items";
    return roles;
}

bool OrderModel::isLoading() const {
    return m_isLoading;
}

int OrderModel::count() const {
    return m_orders.size();
}

QString OrderModel::errorMessage() const {
    return m_errorMessage;
}

void OrderModel::populateFromJson(const QJsonArray &arr) {
    QVector<OrderRecordData> newOrders;
    for (const QJsonValue &val : arr) {
        if (!val.isObject())
            continue;
        QJsonObject obj = val.toObject();

        OrderRecordData o;
        o.id = obj.value(QStringLiteral("_id")).toString(obj.value(QStringLiteral("id")).toString());
        o.customerId = obj.value(QStringLiteral("customer_id")).toString();
        o.shopId = obj.value(QStringLiteral("shop_id")).toString();
        o.shopName = obj.value(QStringLiteral("shop_name")).toString(QStringLiteral("Partner Merchant"));
        o.deliveryBoyId = obj.value(QStringLiteral("delivery_boy_id")).toString();
        o.address = obj.value(QStringLiteral("address")).toString();
        o.status = obj.value(QStringLiteral("status")).toString(QStringLiteral("pending"));
        o.subtotal = obj.value(QStringLiteral("subtotal")).toDouble(0.0);
        o.deliveryFee = obj.value(QStringLiteral("delivery_fee")).toDouble(50.0);
        o.total = obj.value(QStringLiteral("total")).toDouble(o.subtotal + o.deliveryFee);
        o.createdAt = obj.value(QStringLiteral("created_at")).toString();

        QJsonArray itemsArr = obj.value(QStringLiteral("items")).toArray();
        QVariantList itemList;
        for (const QJsonValue &iv : itemsArr) {
            if (iv.isObject()) {
                itemList.append(iv.toObject().toVariantMap());
            }
        }
        o.items = itemList;

        newOrders.append(o);
    }

    beginResetModel();
    m_orders = newOrders;
    endResetModel();
    m_isLoading = false;
    emit loadingChanged();
    emit countChanged();
}

void OrderModel::fetchOrders(bool isOnlineRider) {
    m_isLoading = true;
    emit loadingChanged();

    QString endpoint = QString(QStringLiteral("/api/orders?isOnline=%1")).arg(isOnlineRider ? "true" : "false");

    QPointer<OrderModel> self(this);
    NetworkManager::instance()->get(endpoint, [self](bool success, const QJsonDocument &doc, const QString &err) {
        Q_UNUSED(err);
        if (!self)
            return;
        if (!success || !doc.isArray()) {
            self->beginResetModel();
            self->m_orders.clear();
            self->endResetModel();
            self->m_isLoading = false;
            emit self->loadingChanged();
            emit self->countChanged();
            return;
        }

        self->populateFromJson(doc.array());
    });
}

void OrderModel::updateOrderStatus(const QString &orderId, const QString &nextStatus, const QString &actorRole) {
    if (orderId.isEmpty())
        return;

    // Find current order to validate transition via OrderStateMachine
    OrderStateMachine::OrderStatus currentStatus = OrderStateMachine::OrderStatus::Unknown;
    for (const auto &o : m_orders) {
        if (o.id == orderId) {
            currentStatus = OrderStateMachine::statusFromString(o.status);
            break;
        }
    }

    OrderStateMachine::OrderStatus targetStatus = OrderStateMachine::statusFromString(nextStatus);
    OrderStateMachine::OrderActor actor = OrderStateMachine::actorFromString(actorRole);

    if (currentStatus != OrderStateMachine::OrderStatus::Unknown) {
        auto check = OrderStateMachine::canTransition(currentStatus, targetStatus, actor);
        if (check.isError()) {
            m_errorMessage = check.error().message;
            emit errorChanged();
            return;
        }
    }

    m_errorMessage.clear();
    emit errorChanged();

    QJsonObject body;
    body[QStringLiteral("status")] = nextStatus;

    QPointer<OrderModel> self(this);
    NetworkManager::instance()->patch(
        QString(QStringLiteral("/api/orders/%1")).arg(orderId), body,
        [self, orderId, nextStatus](bool success, const QJsonDocument &doc, const QString &err) {
            Q_UNUSED(doc);
            if (!self)
                return;
            if (success) {
                for (auto &o : self->m_orders) {
                    if (o.id == orderId) {
                        o.status = nextStatus;
                        break;
                    }
                }
                self->fetchOrders();
                emit self->orderUpdated();
            } else {
                self->m_errorMessage = err.isEmpty() ? QStringLiteral("Failed to update order status.") : err;
                emit self->errorChanged();
            }
        });
}

void OrderModel::assignRiderToOrder(const QString &orderId, const QString &riderId) {
    if (orderId.isEmpty() || riderId.isEmpty())
        return;

    QJsonObject body;
    body[QStringLiteral("delivery_boy_id")] = riderId;

    QPointer<OrderModel> self(this);
    NetworkManager::instance()->patch(QString(QStringLiteral("/api/admin/orders/%1/assign")).arg(orderId), body,
                                      [self](bool success, const QJsonDocument &doc, const QString &err) {
                                          Q_UNUSED(doc);
                                          Q_UNUSED(err);
                                          if (!self)
                                              return;
                                          if (success) {
                                              self->fetchOrders();
                                              emit self->orderUpdated();
                                          }
                                      });
}

QVariantMap OrderModel::getOrderAt(int index) const {
    if (index < 0 || index >= m_orders.size())
        return QVariantMap();
    const OrderRecordData &o = m_orders.at(index);
    QVariantMap map;
    map[QStringLiteral("id")] = o.id;
    map[QStringLiteral("customerId")] = o.customerId;
    map[QStringLiteral("shopId")] = o.shopId;
    map[QStringLiteral("shopName")] = o.shopName;
    map[QStringLiteral("deliveryBoyId")] = o.deliveryBoyId;
    map[QStringLiteral("address")] = o.address;
    map[QStringLiteral("status")] = o.status;
    map[QStringLiteral("subtotal")] = o.subtotal;
    map[QStringLiteral("deliveryFee")] = o.deliveryFee;
    map[QStringLiteral("total")] = o.total;
    map[QStringLiteral("createdAt")] = o.createdAt;
    map[QStringLiteral("items")] = o.items;
    return map;
}

void OrderModel::startPolling(int intervalMs) {
    if (m_pollTimer && !m_pollTimer->isActive()) {
        m_pollTimer->start(intervalMs);
    }
}

void OrderModel::stopPolling() {
    if (m_pollTimer && m_pollTimer->isActive()) {
        m_pollTimer->stop();
    }
}
