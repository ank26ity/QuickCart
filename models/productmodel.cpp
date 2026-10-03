#include "productmodel.h"
#include "../api/networkmanager.h"
#include <QtCore/QJsonDocument>
#include <QtCore/QThreadPool>
#include <QtCore/QPointer>
#include <QtConcurrent/QtConcurrent>

ProductModel::ProductModel(QObject *parent) : QAbstractListModel(parent), m_isLoading(false) {}

int ProductModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return m_products.size();
}

QVariant ProductModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_products.size())
        return QVariant();

    const ProductItemData &p = m_products.at(index.row());
    switch (role) {
        case IdRole:
            return p.id;
        case ShopIdRole:
            return p.shopId;
        case NameRole:
            return p.name;
        case DescriptionRole:
            return p.description;
        case PriceRole:
            return p.pricePaise;
        case QuantityRole:
            return p.quantity;
        case ImageRole:
            return p.image;
        case IsLowStockRole:
            return (p.quantity > 0 && p.quantity <= 5);
        case IsOutOfStockRole:
            return (p.quantity <= 0);
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> ProductModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "productId";
    roles[ShopIdRole] = "shopId";
    roles[NameRole] = "name";
    roles[DescriptionRole] = "description";
    roles[PriceRole] = "pricePaise";
    roles[QuantityRole] = "quantity";
    roles[ImageRole] = "image";
    roles[IsLowStockRole] = "isLowStock";
    roles[IsOutOfStockRole] = "isOutOfStock";
    return roles;
}

bool ProductModel::isLoading() const {
    return m_isLoading;
}

int ProductModel::count() const {
    return m_products.size();
}

void ProductModel::fetchProductsForShop(const QString &shopId) {
    if (shopId.isEmpty())
        return;

    m_isLoading = true;
    emit loadingChanged();

    QString endpoint = QString("/api/shops/%1").arg(shopId);

    QPointer<ProductModel> self(this);
    NetworkManager::instance()->get(endpoint, [self](bool success, const QJsonDocument &doc, const QString &err) {
        Q_UNUSED(err);
        if (!self)
            return;
        if (!success || !doc.isObject()) {
            self->beginResetModel();
            self->m_products.clear();
            self->endResetModel();
            self->m_isLoading = false;
            emit self->loadingChanged();
            emit self->countChanged();
            return;
        }

        QJsonObject obj = doc.object();
        QJsonArray itemsArr = obj.value("items").toArray();

        QThreadPool::globalInstance()->start([self, itemsArr]() {
            QVector<ProductItemData> newProducts;
            for (const QJsonValue &val : itemsArr) {
                if (!val.isObject())
                    continue;
                QJsonObject itemObj = val.toObject();

                ProductItemData item;
                item.id = itemObj.value("_id").toString(itemObj.value("id").toString());
                item.shopId = itemObj.value("shop_id").toString();
                item.name = itemObj.value("name").toString();
                item.description = itemObj.value("description").toString();
                if (itemObj.contains("price_paise"))
                    item.pricePaise = itemObj.value("price_paise").toInteger();
                else if (itemObj.contains("pricePaise"))
                    item.pricePaise = itemObj.value("pricePaise").toInteger();
                else
                    item.pricePaise = static_cast<qint64>(std::round(itemObj.value("price").toDouble(0.0) * 100.0));
                item.quantity = itemObj.value("quantity").toInt(0);
                item.image = itemObj.value("image").toString(
                    "https://images.unsplash.com/photo-1542838132-92c53300491e?auto=format&fit=crop&q=80&w=300");

                newProducts.append(item);
            }

            if (!self)
                return;
            QMetaObject::invokeMethod(self.data(), [self, newProducts]() {
                if (!self)
                    return;
                self->beginResetModel();
                self->m_products = newProducts;
                self->endResetModel();
                self->m_isLoading = false;
                emit self->loadingChanged();
                emit self->countChanged();
            });
        });
    });
}

void ProductModel::populateFromJson(const QJsonArray &itemsArr) {
    QVector<ProductItemData> newProducts;
    for (const QJsonValue &val : itemsArr) {
        if (!val.isObject())
            continue;
        QJsonObject itemObj = val.toObject();

        ProductItemData item;
        item.id = itemObj.value(QStringLiteral("_id")).toString(itemObj.value(QStringLiteral("id")).toString());
        item.shopId = itemObj.value(QStringLiteral("shop_id")).toString();
        item.name = itemObj.value(QStringLiteral("name")).toString();
        item.description = itemObj.value(QStringLiteral("description")).toString();
        if (itemObj.contains(QStringLiteral("price_paise")))
            item.pricePaise = itemObj.value(QStringLiteral("price_paise")).toInteger();
        else if (itemObj.contains(QStringLiteral("pricePaise")))
            item.pricePaise = itemObj.value(QStringLiteral("pricePaise")).toInteger();
        else
            item.pricePaise =
                static_cast<qint64>(std::round(itemObj.value(QStringLiteral("price")).toDouble(0.0) * 100.0));
        item.quantity = itemObj.value(QStringLiteral("quantity")).toInt(0);
        item.image =
            itemObj.value(QStringLiteral("image"))
                .toString(QStringLiteral(
                    "https://images.unsplash.com/photo-1542838132-92c53300491e?auto=format&fit=crop&q=80&w=300"));

        newProducts.append(item);
    }

    beginResetModel();
    m_products = newProducts;
    endResetModel();
    m_isLoading = false;
    emit loadingChanged();
    emit countChanged();
}

bool ProductModel::addProduct(const QJsonObject &productData) {
    ProductItemData item;
    item.id = productData.value(QStringLiteral("_id"))
                  .toString(productData.value(QStringLiteral("id"))
                                .toString(QStringLiteral("prod_") + QString::number(m_products.size() + 1)));
    item.shopId = productData.value(QStringLiteral("shop_id")).toString();
    item.name = productData.value(QStringLiteral("name")).toString();
    item.description = productData.value(QStringLiteral("description")).toString();
    if (productData.contains(QStringLiteral("price_paise")))
        item.pricePaise = productData.value(QStringLiteral("price_paise")).toInteger();
    else if (productData.contains(QStringLiteral("pricePaise")))
        item.pricePaise = productData.value(QStringLiteral("pricePaise")).toInteger();
    else
        item.pricePaise =
            static_cast<qint64>(std::round(productData.value(QStringLiteral("price")).toDouble(0.0) * 100.0));
    item.quantity = productData.value(QStringLiteral("quantity")).toInt(0);
    item.image = productData.value(QStringLiteral("image"))
                     .toString(QStringLiteral(
                         "https://images.unsplash.com/photo-1542838132-92c53300491e?auto=format&fit=crop&q=80&w=300"));

    beginInsertRows(QModelIndex(), m_products.size(), m_products.size());
    m_products.append(item);
    endInsertRows();

    emit countChanged();
    emit productUpdated(item.id);
    return true;
}

bool ProductModel::updateProduct(const QString &productId, const QJsonObject &productData) {
    for (int i = 0; i < m_products.size(); ++i) {
        if (m_products[i].id == productId) {
            if (productData.contains(QStringLiteral("name")))
                m_products[i].name = productData.value(QStringLiteral("name")).toString();
            if (productData.contains(QStringLiteral("description")))
                m_products[i].description = productData.value(QStringLiteral("description")).toString();
            if (productData.contains(QStringLiteral("price_paise")))
                m_products[i].pricePaise = productData.value(QStringLiteral("price_paise")).toInteger();
            else if (productData.contains(QStringLiteral("pricePaise")))
                m_products[i].pricePaise = productData.value(QStringLiteral("pricePaise")).toInteger();
            else if (productData.contains(QStringLiteral("price")))
                m_products[i].pricePaise =
                    static_cast<qint64>(std::round(productData.value(QStringLiteral("price")).toDouble() * 100.0));
            if (productData.contains(QStringLiteral("quantity")))
                m_products[i].quantity = productData.value(QStringLiteral("quantity")).toInt();
            if (productData.contains(QStringLiteral("image")))
                m_products[i].image = productData.value(QStringLiteral("image")).toString();

            emit dataChanged(index(i, 0), index(i, 0));
            emit productUpdated(productId);
            return true;
        }
    }
    return false;
}

bool ProductModel::deleteProduct(const QString &productId) {
    for (int i = 0; i < m_products.size(); ++i) {
        if (m_products[i].id == productId) {
            beginRemoveRows(QModelIndex(), i, i);
            m_products.removeAt(i);
            endRemoveRows();
            emit countChanged();
            emit productUpdated(productId);
            return true;
        }
    }
    return false;
}

QVariantMap ProductModel::getProductAt(int index) const {
    if (index < 0 || index >= m_products.size())
        return QVariantMap();
    const ProductItemData &p = m_products.at(index);
    QVariantMap map;
    map["id"] = p.id;
    map["shopId"] = p.shopId;
    map["name"] = p.name;
    map["description"] = p.description;
    map["pricePaise"] = p.pricePaise;
    map["quantity"] = p.quantity;
    map["image"] = p.image;
    return map;
}
