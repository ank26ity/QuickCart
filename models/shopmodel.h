/**
 * @file shopmodel.h
 * @brief Shop discovery model with coordinates and 3 km hyperlocal boundary filtering.
 * @layer ViewModels / Presentation Models (Layer 2)
 *
 * Public API Summary:
 * - rowCount(), data(), roleNames(): QAbstractListModel interface
 * - isLoading, count
 * - fetchShops(categoryFilter, lat, lng): Async fetch from API with location filter
 * - populateFromJson(arr, lat, lng): Synchronous population for deterministic parsing/tests
 * - getShopAt(index): Direct access to shop map
 *
 * Dependencies:
 * - core/validators.h, api/networkmanager.h
 *
 * Tests:
 * - Covered by tests/cpp/test_customer_cart_flow.cpp
 */

#ifndef SHOPMODEL_H
#define SHOPMODEL_H

#include <QtCore/QAbstractListModel>
#include <QtCore/QString>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QVector>

struct ShopItemData {
    QString id;
    QString name;
    QString category;
    QString description;
    QString image;
    double rating{5.0};
    double distance{0.0};
    QString address;
    QString ownerId;
    double lat{0.0};
    double lng{0.0};
};

class ShopModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum ShopRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        CategoryRole,
        DescriptionRole,
        ImageRole,
        RatingRole,
        DistanceRole,
        AddressRole,
        OwnerIdRole,
        LatRole,
        LngRole
    };

    explicit ShopModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::UserRole + 1) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isLoading() const;
    int count() const;

    Q_INVOKABLE void fetchShops(const QString &categoryFilter = "all", double lat = 0.0, double lng = 0.0);
    Q_INVOKABLE QVariantMap getShopAt(int index) const;

    /**
     * @brief Populate shops directly from JSON data applying the 3 km filter.
     */
    void populateFromJson(const QJsonArray &arr, double userLat = 0.0, double userLng = 0.0);

signals:
    void loadingChanged();
    void countChanged();
    void shopsLoaded();

private:
    QVector<ShopItemData> m_shops;
    bool m_isLoading{false};
};

#endif // SHOPMODEL_H
