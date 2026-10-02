/**
 * @file shopmodel.cpp
 * @brief Implementation of ShopModel.
 * @layer ViewModels / Presentation Models (Layer 2)
 * @tests Covered by tests/cpp/test_customer_cart_flow.cpp
 */

#include "shopmodel.h"
#include "../api/networkmanager.h"
#include "../core/validators.h"
#include <QtCore/QJsonDocument>
#include <QtCore/QThreadPool>
#include <QtConcurrent/QtConcurrent>

ShopModel::ShopModel(QObject *parent) : QAbstractListModel(parent), m_isLoading(false) {}

int ShopModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return m_shops.size();
}

QVariant ShopModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_shops.size())
        return QVariant();

    const ShopItemData &shop = m_shops.at(index.row());
    switch (role) {
        case IdRole:
            return shop.id;
        case NameRole:
            return shop.name;
        case CategoryRole:
            return shop.category;
        case DescriptionRole:
            return shop.description;
        case ImageRole:
            return shop.image;
        case RatingRole:
            return shop.rating;
        case DistanceRole:
            return shop.distance;
        case AddressRole:
            return shop.address;
        case OwnerIdRole:
            return shop.ownerId;
        case LatRole:
            return shop.lat;
        case LngRole:
            return shop.lng;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> ShopModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "shopId";
    roles[NameRole] = "name";
    roles[CategoryRole] = "category";
    roles[DescriptionRole] = "description";
    roles[ImageRole] = "image";
    roles[RatingRole] = "rating";
    roles[DistanceRole] = "distance";
    roles[AddressRole] = "address";
    roles[OwnerIdRole] = "ownerId";
    roles[LatRole] = "lat";
    roles[LngRole] = "lng";
    return roles;
}

bool ShopModel::isLoading() const {
    return m_isLoading;
}

int ShopModel::count() const {
    return m_shops.size();
}

void ShopModel::populateFromJson(const QJsonArray &arr, double userLat, double userLng) {
    bool hasLocation = (userLat != 0.0 || userLng != 0.0);

    QVector<ShopItemData> newShops;
    for (const QJsonValue &val : arr) {
        if (!val.isObject())
            continue;
        QJsonObject obj = val.toObject();

        ShopItemData item;
        item.id = obj.value(QStringLiteral("_id")).toString(obj.value(QStringLiteral("id")).toString());
        item.name = obj.value(QStringLiteral("name")).toString();
        item.category = obj.value(QStringLiteral("category")).toString(QStringLiteral("groceries"));
        item.description = obj.value(QStringLiteral("description")).toString();
        item.image =
            obj.value(QStringLiteral("image"))
                .toString(QStringLiteral(
                    "https://images.unsplash.com/photo-1542838132-92c53300491e?auto=format&fit=crop&q=80&w=600"));
        item.rating = obj.value(QStringLiteral("rating")).toDouble(5.0);
        item.address = obj.value(QStringLiteral("address")).toString();
        item.ownerId = obj.value(QStringLiteral("owner_id")).toString();

        double shopLat = 28.6139;
        double shopLng = 77.2090;
        if (obj.contains(QStringLiteral("location")) && obj.value(QStringLiteral("location")).isObject()) {
            QJsonArray coords =
                obj.value(QStringLiteral("location")).toObject().value(QStringLiteral("coordinates")).toArray();
            if (coords.size() >= 2) {
                shopLng = coords.at(0).toDouble();
                shopLat = coords.at(1).toDouble();
            }
        } else if (obj.contains(QStringLiteral("lat")) && obj.contains(QStringLiteral("lng"))) {
            shopLat = obj.value(QStringLiteral("lat")).toDouble();
            shopLng = obj.value(QStringLiteral("lng")).toDouble();
        }
        item.lat = shopLat;
        item.lng = shopLng;

        if (hasLocation) {
            item.distance = Validators::calculateHaversineDistanceKm(userLat, userLng, shopLat, shopLng);
            if (item.distance > 3.0) {
                continue; // Exclude stores outside 3km hyperlocal boundary
            }
        } else {
            item.distance = 0.0;
        }

        newShops.append(item);
    }

    beginResetModel();
    m_shops = newShops;
    endResetModel();
    m_isLoading = false;
    emit loadingChanged();
    emit countChanged();
    emit shopsLoaded();
}

void ShopModel::fetchShops(const QString &categoryFilter, double lat, double lng) {
    m_isLoading = true;
    emit loadingChanged();

    bool hasLocation = (lat != 0.0 || lng != 0.0);
    double activeLat = hasLocation ? lat : 0.0;
    double activeLng = hasLocation ? lng : 0.0;

    QString endpoint = QStringLiteral("/api/shops?category=") + categoryFilter;
    if (hasLocation) {
        endpoint += QString(QStringLiteral("&lat=%1&lng=%2"))
                        .arg(QString::number(activeLat, 'f', 6), QString::number(activeLng, 'f', 6));
    }

    NetworkManager::instance()->get(
        endpoint, [this, activeLat, activeLng](bool success, const QJsonDocument &doc, const QString &err) {
            Q_UNUSED(err);
            if (!success || !doc.isArray()) {
                beginResetModel();
                m_shops.clear();
                endResetModel();
                m_isLoading = false;
                emit loadingChanged();
                emit countChanged();
                return;
            }

            populateFromJson(doc.array(), activeLat, activeLng);
        });
}

QVariantMap ShopModel::getShopAt(int index) const {
    if (index < 0 || index >= m_shops.size())
        return QVariantMap();
    const ShopItemData &s = m_shops.at(index);
    QVariantMap map;
    map[QStringLiteral("id")] = s.id;
    map[QStringLiteral("name")] = s.name;
    map[QStringLiteral("category")] = s.category;
    map[QStringLiteral("description")] = s.description;
    map[QStringLiteral("image")] = s.image;
    map[QStringLiteral("rating")] = s.rating;
    map[QStringLiteral("distance")] = s.distance;
    map[QStringLiteral("address")] = s.address;
    map[QStringLiteral("ownerId")] = s.ownerId;
    map[QStringLiteral("lat")] = s.lat;
    map[QStringLiteral("lng")] = s.lng;
    return map;
}
