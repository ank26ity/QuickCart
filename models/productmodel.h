#ifndef PRODUCTMODEL_H
#define PRODUCTMODEL_H

#include <QtCore/QAbstractListModel>
#include <QtCore/QString>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QVector>

struct ProductItemData {
    QString id;
    QString shopId;
    QString name;
    QString description;
    double price;
    int quantity;
    QString image;
};

class ProductModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum ProductRoles {
        IdRole = Qt::UserRole + 1,
        ShopIdRole,
        NameRole,
        DescriptionRole,
        PriceRole,
        QuantityRole,
        ImageRole,
        IsLowStockRole,
        IsOutOfStockRole
    };

    explicit ProductModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::UserRole + 1) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isLoading() const;
    int count() const;

    Q_INVOKABLE void fetchProductsForShop(const QString &shopId);
    Q_INVOKABLE QVariantMap getProductAt(int index) const;

    /**
     * @brief Direct model population from JSON array.
     */
    void populateFromJson(const QJsonArray &arr);

    /**
     * @brief CRUD operations on products.
     */
    Q_INVOKABLE bool addProduct(const QJsonObject &productData);
    Q_INVOKABLE bool updateProduct(const QString &productId, const QJsonObject &productData);
    Q_INVOKABLE bool deleteProduct(const QString &productId);

signals:
    void loadingChanged();
    void countChanged();
    void productUpdated(const QString &productId);

private:
    QVector<ProductItemData> m_products;
    bool m_isLoading;
};

#endif // PRODUCTMODEL_H
