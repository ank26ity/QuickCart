#include "productmodel.h"
#include "../api/networkmanager.h"
#include <QtCore/QJsonDocument>
#include <QtCore/QThreadPool>
#include <QtConcurrent/QtConcurrent>

ProductModel::ProductModel(QObject *parent)
    : QAbstractListModel(parent), m_isLoading(false)
{
}

int ProductModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_products.size();
}

QVariant ProductModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_products.size())
        return QVariant();

    const ProductItemData &p = m_products.at(index.row());
    switch (role) {
    case IdRole: return p.id;
    case ShopIdRole: return p.shopId;
    case NameRole: return p.name;
    case DescriptionRole: return p.description;
    case PriceRole: return p.price;
    case QuantityRole: return p.quantity;
    case ImageRole: return p.image;
    case IsLowStockRole: return (p.quantity > 0 && p.quantity <= 5);
    case IsOutOfStockRole: return (p.quantity <= 0);
    default: return QVariant();
    }
}

QHash<int, QByteArray> ProductModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "productId";
    roles[ShopIdRole] = "shopId";
    roles[NameRole] = "name";
    roles[DescriptionRole] = "description";
    roles[PriceRole] = "price";
    roles[QuantityRole] = "quantity";
    roles[ImageRole] = "image";
    roles[IsLowStockRole] = "isLowStock";
    roles[IsOutOfStockRole] = "isOutOfStock";
    return roles;
}

bool ProductModel::isLoading() const
{
    return m_isLoading;
}

int ProductModel::count() const
{
    return m_products.size();
}

void ProductModel::fetchProductsForShop(const QString &shopId)
{
    if (shopId.isEmpty()) return;

    m_isLoading = true;
    emit loadingChanged();

    QString endpoint = QString("/api/shops/%1").arg(shopId);

    NetworkManager::instance()->get(endpoint, [this](bool success, const QJsonDocument &doc, const QString &err) {
        Q_UNUSED(err);
        if (!success || !doc.isObject()) {
            beginResetModel();
            m_products.clear();
            endResetModel();
            m_isLoading = false;
            emit loadingChanged();
            emit countChanged();
            return;
        }

        QJsonObject obj = doc.object();
        QJsonArray itemsArr = obj.value("items").toArray();

        QThreadPool::globalInstance()->start([this, itemsArr]() {
            QVector<ProductItemData> newProducts;
            for (const QJsonValue &val : itemsArr) {
                if (!val.isObject()) continue;
                QJsonObject itemObj = val.toObject();

                ProductItemData item;
                item.id = itemObj.value("_id").toString(itemObj.value("id").toString());
                item.shopId = itemObj.value("shop_id").toString();
                item.name = itemObj.value("name").toString();
                item.description = itemObj.value("description").toString();
                item.price = itemObj.value("price").toDouble(0.0);
                item.quantity = itemObj.value("quantity").toInt(0);
                item.image = itemObj.value("image").toString("https://images.unsplash.com/photo-1542838132-92c53300491e?auto=format&fit=crop&q=80&w=300");

                newProducts.append(item);
            }

            QMetaObject::invokeMethod(this, [this, newProducts]() {
                beginResetModel();
                m_products = newProducts;
                endResetModel();
                m_isLoading = false;
                emit loadingChanged();
                emit countChanged();
            });
        });
    });
}

QVariantMap ProductModel::getProductAt(int index) const
{
    if (index < 0 || index >= m_products.size()) return QVariantMap();
    const ProductItemData &p = m_products.at(index);
    QVariantMap map;
    map["id"] = p.id;
    map["shopId"] = p.shopId;
    map["name"] = p.name;
    map["description"] = p.description;
    map["price"] = p.price;
    map["quantity"] = p.quantity;
    map["image"] = p.image;
    return map;
}
