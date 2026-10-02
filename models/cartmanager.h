/**
 * @file cartmanager.h
 * @brief Shopping cart state management, single-store enforcement, and checkout processing.
 * @layer ViewModels / Services (Layer 2 - Presentation Models)
 *
 * Public API Summary:
 * - itemCount, subtotal, deliveryFee, total, shopId, items, isSubmitting, errorMessage
 * - addItem(productMap, maxStock): Add product with stock guard and single-store mismatch prompt
 * - updateQuantity(productId, delta): Modify item count within stock limits
 * - clearCart(): Empty current cart contents
 * - placeOrder(address): Checkout with generated idempotency key
 * - resetForTesting(): Reset cart state between test runs
 *
 * Dependencies:
 * - core/validators.h, api/apiclient.h, api/networkmanager.h
 *
 * Tests:
 * - Covered by tests/cpp/test_customer_cart_flow.cpp
 */

#ifndef CARTMANAGER_H
#define CARTMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>
#include <QtCore/QVector>

struct CartEntry {
    QString productId;
    QString shopId;
    QString name;
    qint64 pricePaise{0};
    int quantity{0};
    int maxStock{0};
    QString image;
    double price() const { return pricePaise / 100.0; }
};

class CartManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(int itemCount READ itemCount NOTIFY cartChanged)
    Q_PROPERTY(double subtotal READ subtotal NOTIFY cartChanged)
    Q_PROPERTY(double deliveryFee READ deliveryFee NOTIFY cartChanged)
    Q_PROPERTY(double total READ total NOTIFY cartChanged)
    Q_PROPERTY(qint64 subtotalPaise READ subtotalPaise NOTIFY cartChanged)
    Q_PROPERTY(qint64 deliveryFeePaise READ deliveryFeePaise NOTIFY cartChanged)
    Q_PROPERTY(qint64 totalPaise READ totalPaise NOTIFY cartChanged)
    Q_PROPERTY(double deliveryDistanceKm READ deliveryDistanceKm WRITE setDeliveryDistanceKm NOTIFY cartChanged)
    Q_PROPERTY(QString shopId READ shopId NOTIFY cartChanged)
    Q_PROPERTY(QVariantList items READ items NOTIFY cartChanged)
    Q_PROPERTY(bool isSubmitting READ isSubmitting NOTIFY submittingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

public:
    explicit CartManager(QObject *parent = nullptr);
    static CartManager *instance();

    int itemCount() const;
    double subtotal() const;
    double deliveryFee() const;
    double total() const;

    qint64 subtotalPaise() const;
    qint64 deliveryFeePaise() const;
    qint64 totalPaise() const;

    double deliveryDistanceKm() const;
    void setDeliveryDistanceKm(double km);

    QString shopId() const;
    QVariantList items() const;
    bool isSubmitting() const;
    QString errorMessage() const;

    Q_INVOKABLE bool addItem(const QVariantMap &product, int maxStock);
    Q_INVOKABLE void updateQuantity(const QString &productId, int change);
    Q_INVOKABLE void clearCart();
    Q_INVOKABLE void placeOrder(const QString &address);
    Q_INVOKABLE void syncServerCalculation();

    /**
     * @brief Reset cart state for test isolation.
     */
    void resetForTesting();

signals:
    void cartChanged();
    void submittingChanged();
    void errorChanged();
    void orderPlacedSuccess(const QString &orderId);
    void promptStoreMismatch(const QVariantMap &pendingProduct, int pendingMaxStock);
    void serverCalculated();

private:
    QVector<CartEntry> m_cart;
    bool m_isSubmitting{false};
    QString m_errorMessage;
    double m_deliveryDistanceKm{1.5};
    qint64 m_serverSubtotalPaise{0};
    qint64 m_serverDeliveryFeePaise{0};
    qint64 m_serverTotalPaise{0};
    bool m_hasServerCalculation{false};
};

#endif // CARTMANAGER_H
