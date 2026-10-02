/**
 * @file test_product_crud.cpp
 * @brief Automated unit test suite for Merchant Product CRUD operations and stock flags.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include "../../models/productmodel.h"

class TestProductCrud : public QObject {
    Q_OBJECT

private slots:
    void testProductCreationAndListing() {
        ProductModel model;
        QCOMPARE(model.count(), 0);

        QJsonObject p1;
        p1[QStringLiteral("id")] = QStringLiteral("p1");
        p1[QStringLiteral("shop_id")] = QStringLiteral("shop_101");
        p1[QStringLiteral("name")] = QStringLiteral("Organic Apples 1kg");
        p1[QStringLiteral("description")] = QStringLiteral("Crisp Washington apples");
        p1[QStringLiteral("price")] = 180.0;
        p1[QStringLiteral("quantity")] = 25;

        QSignalSpy countSpy(&model, &ProductModel::countChanged);
        QSignalSpy updateSpy(&model, &ProductModel::productUpdated);

        QVERIFY(model.addProduct(p1));
        QCOMPARE(model.count(), 1);
        QCOMPARE(countSpy.count(), 1);
        QCOMPARE(updateSpy.count(), 1);

        QVariantMap item = model.getProductAt(0);
        QCOMPARE(item[QStringLiteral("id")].toString(), QStringLiteral("p1"));
        QCOMPARE(item[QStringLiteral("name")].toString(), QStringLiteral("Organic Apples 1kg"));
        QCOMPARE(item[QStringLiteral("price")].toDouble(), 180.0);
        QCOMPARE(item[QStringLiteral("quantity")].toInt(), 25);
    }

    void testProductUpdate() {
        ProductModel model;
        QJsonObject p1;
        p1[QStringLiteral("id")] = QStringLiteral("p2");
        p1[QStringLiteral("shop_id")] = QStringLiteral("shop_101");
        p1[QStringLiteral("name")] = QStringLiteral("Almond Milk 1L");
        p1[QStringLiteral("price")] = 240.0;
        p1[QStringLiteral("quantity")] = 10;
        model.addProduct(p1);

        QJsonObject updateData;
        updateData[QStringLiteral("price")] = 220.0;
        updateData[QStringLiteral("quantity")] = 4; // Should trigger low stock

        QSignalSpy updateSpy(&model, &ProductModel::productUpdated);
        QVERIFY(model.updateProduct(QStringLiteral("p2"), updateData));
        QCOMPARE(updateSpy.count(), 1);

        QVariantMap updatedItem = model.getProductAt(0);
        QCOMPARE(updatedItem[QStringLiteral("price")].toDouble(), 220.0);
        QCOMPARE(updatedItem[QStringLiteral("quantity")].toInt(), 4);

        // Verify low stock role
        QModelIndex idx = model.index(0, 0);
        QCOMPARE(model.data(idx, ProductModel::IsLowStockRole).toBool(), true);
        QCOMPARE(model.data(idx, ProductModel::IsOutOfStockRole).toBool(), false);
    }

    void testProductDeletion() {
        ProductModel model;
        QJsonObject p1;
        p1[QStringLiteral("id")] = QStringLiteral("p3");
        p1[QStringLiteral("name")] = QStringLiteral("Dark Chocolate");
        p1[QStringLiteral("price")] = 99.0;
        p1[QStringLiteral("quantity")] = 0; // Out of stock
        model.addProduct(p1);

        QModelIndex idx = model.index(0, 0);
        QCOMPARE(model.data(idx, ProductModel::IsOutOfStockRole).toBool(), true);

        QSignalSpy countSpy(&model, &ProductModel::countChanged);
        QVERIFY(model.deleteProduct(QStringLiteral("p3")));
        QCOMPARE(model.count(), 0);
        QCOMPARE(countSpy.count(), 1);

        // Deleting non-existent product returns false
        QVERIFY(!model.deleteProduct(QStringLiteral("p3")));
    }

    void testPopulateFromJson() {
        ProductModel model;
        QJsonArray arr;

        for (int i = 1; i <= 3; ++i) {
            QJsonObject obj;
            obj[QStringLiteral("id")] = QString("prod_%1").arg(i);
            obj[QStringLiteral("name")] = QString("Product %1").arg(i);
            obj[QStringLiteral("price")] = 50.0 * i;
            obj[QStringLiteral("quantity")] = i * 2;
            arr.append(obj);
        }

        model.populateFromJson(arr);
        QCOMPARE(model.count(), 3);
        QCOMPARE(model.getProductAt(0)[QStringLiteral("name")].toString(), QStringLiteral("Product 1"));
        QCOMPARE(model.getProductAt(2)[QStringLiteral("price")].toDouble(), 150.0);
    }
};

QTEST_MAIN(TestProductCrud)
#include "test_product_crud.moc"
