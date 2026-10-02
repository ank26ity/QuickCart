/**
 * @file cartmanager.cpp
 * @brief Implementation of CartManager.
 * @layer ViewModels / Services (Layer 2 - Presentation Models)
 * @tests Covered by tests/cpp/test_customer_cart_flow.cpp
 */

#include "cartmanager.h"
#include "../api/networkmanager.h"
#include "../api/apiclient.h"
#include "../core/validators.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

static CartManager *s_cartManagerInstance = nullptr;

CartManager::CartManager(QObject *parent)
    : QObject(parent), m_isSubmitting(false)
{
    s_cartManagerInstance = this;
}

CartManager* CartManager::instance()
{
    if (!s_cartManagerInstance) {
        new CartManager(qApp);
    }
    return s_cartManagerInstance;
}

int CartManager::itemCount() const
{
    int count = 0;
    for (const CartEntry &entry : m_cart) {
        count += entry.quantity;
    }
    return count;
}

double CartManager::subtotal() const
{
    double sum = 0.0;
    for (const CartEntry &entry : m_cart) {
        sum += (entry.price * entry.quantity);
    }
    return sum;
}

double CartManager::deliveryFee() const
{
    return m_cart.isEmpty() ? 0.0 : 50.0;
}

double CartManager::total() const
{
    return subtotal() + deliveryFee();
}

QString CartManager::shopId() const
{
    return m_cart.isEmpty() ? QString() : m_cart.first().shopId;
}

QVariantList CartManager::items() const
{
    QVariantList list;
    for (const CartEntry &entry : m_cart) {
        QVariantMap map;
        map[QStringLiteral("productId")] = entry.productId;
        map[QStringLiteral("shopId")] = entry.shopId;
        map[QStringLiteral("name")] = entry.name;
        map[QStringLiteral("price")] = entry.price;
        map[QStringLiteral("quantity")] = entry.quantity;
        map[QStringLiteral("maxStock")] = entry.maxStock;
        map[QStringLiteral("image")] = entry.image;
        list.append(map);
    }
    return list;
}

bool CartManager::isSubmitting() const
{
    return m_isSubmitting;
}

QString CartManager::errorMessage() const
{
    return m_errorMessage;
}

bool CartManager::addItem(const QVariantMap &product, int maxStock)
{
    QString pId = product.value(QStringLiteral("id")).toString();
    QString pShopId = product.value(QStringLiteral("shopId")).toString();

    // 1. Single-Store Cart Rule Check
    if (!m_cart.isEmpty() && m_cart.first().shopId != pShopId) {
        emit promptStoreMismatch(product, maxStock);
        return false;
    }

    // 2. Stock Limit Enforcement
    if (maxStock <= 0) {
        m_errorMessage = QStringLiteral("This item is currently out of stock.");
        emit errorChanged();
        return false;
    }

    int existingIdx = -1;
    for (int i = 0; i < m_cart.size(); ++i) {
        if (m_cart[i].productId == pId) {
            existingIdx = i;
            break;
        }
    }

    if (existingIdx >= 0) {
        if (m_cart[existingIdx].quantity >= maxStock) {
            m_errorMessage = QString(QStringLiteral("Store only has %1 items available in stock.")).arg(maxStock);
            emit errorChanged();
            return false;
        }
        m_cart[existingIdx].quantity += 1;
    } else {
        CartEntry entry;
        entry.productId = pId;
        entry.shopId = pShopId;
        entry.name = product.value(QStringLiteral("name")).toString();
        entry.price = product.value(QStringLiteral("price")).toDouble();
        entry.quantity = 1;
        entry.maxStock = maxStock;
        entry.image = product.value(QStringLiteral("image")).toString();
        m_cart.append(entry);
    }

    m_errorMessage.clear();
    emit errorChanged();
    emit cartChanged();
    return true;
}

void CartManager::updateQuantity(const QString &productId, int change)
{
    for (int i = 0; i < m_cart.size(); ++i) {
        if (m_cart[i].productId == productId) {
            if (change > 0) {
                if (m_cart[i].quantity >= m_cart[i].maxStock) {
                    m_errorMessage = QString(QStringLiteral("Only %1 units available in store stock.")).arg(m_cart[i].maxStock);
                    emit errorChanged();
                    return;
                }
                m_cart[i].quantity += 1;
            } else {
                m_cart[i].quantity -= 1;
                if (m_cart[i].quantity <= 0) {
                    m_cart.removeAt(i);
                }
            }
            m_errorMessage.clear();
            emit errorChanged();
            emit cartChanged();
            return;
        }
    }
}

void CartManager::clearCart()
{
    m_cart.clear();
    m_errorMessage.clear();
    emit errorChanged();
    emit cartChanged();
}

void CartManager::placeOrder(const QString &address)
{
    if (m_cart.isEmpty()) {
        m_errorMessage = QStringLiteral("Cart is empty.");
        emit errorChanged();
        return;
    }

    if (address.trimmed().isEmpty()) {
        m_errorMessage = QStringLiteral("Please enter a valid delivery address.");
        emit errorChanged();
        return;
    }

    m_isSubmitting = true;
    m_errorMessage.clear();
    emit submittingChanged();
    emit errorChanged();

    QJsonObject payload;
    payload[QStringLiteral("shop_id")] = shopId();
    payload[QStringLiteral("address")] = address.trimmed();
    payload[QStringLiteral("subtotal")] = subtotal();
    payload[QStringLiteral("delivery_fee")] = deliveryFee();
    payload[QStringLiteral("total")] = total();
    payload[QStringLiteral("idempotency_key")] = ApiClient::generateIdempotencyKey();

    QJsonArray itemsArr;
    for (const CartEntry &e : m_cart) {
        QJsonObject itemObj;
        itemObj[QStringLiteral("item_id")] = e.productId;
        itemObj[QStringLiteral("name")] = e.name;
        itemObj[QStringLiteral("price")] = e.price;
        itemObj[QStringLiteral("quantity")] = e.quantity;
        itemsArr.append(itemObj);
    }
    payload[QStringLiteral("items")] = itemsArr;

    NetworkManager::instance()->post(QStringLiteral("/api/orders"), payload, [this](bool success, const QJsonDocument &doc, const QString &err) {
        m_isSubmitting = false;
        emit submittingChanged();

        if (!success) {
            m_errorMessage = err.isEmpty() ? QStringLiteral("Failed to place order.") : err;
            emit errorChanged();
            return;
        }

        QString createdOrderId;
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            createdOrderId = obj.value(QStringLiteral("_id")).toString(obj.value(QStringLiteral("id")).toString());
        }

        clearCart();
        emit orderPlacedSuccess(createdOrderId);
    });
}

void CartManager::resetForTesting()
{
    m_cart.clear();
    m_isSubmitting = false;
    m_errorMessage.clear();
    emit cartChanged();
    emit submittingChanged();
    emit errorChanged();
}
