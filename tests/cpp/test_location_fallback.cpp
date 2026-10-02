/**
 * @file test_location_fallback.cpp
 * @brief Automated unit test suite for GPS fallback, coordinate bounding, and hyperlocal shop distance filtering.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include "../../core/validators.h"
#include "../../models/shopmodel.h"

class TestLocationFallback : public QObject {
    Q_OBJECT

private slots:
    void testCoordinateValidation() {
        // Valid coordinates
        QVERIFY(Validators::validateCoordinates(28.6139, 77.2090).isSuccess());   // New Delhi
        QVERIFY(Validators::validateCoordinates(-33.8688, 151.2093).isSuccess()); // Sydney
        QVERIFY(Validators::validateCoordinates(0.0, 0.0).isSuccess());

        // Out of bounds latitude
        QVERIFY(Validators::validateCoordinates(91.0, 77.0).isError());
        QVERIFY(Validators::validateCoordinates(-90.1, 77.0).isError());

        // Out of bounds longitude
        QVERIFY(Validators::validateCoordinates(28.0, 180.5).isError());
        QVERIFY(Validators::validateCoordinates(28.0, -181.0).isError());
    }

    void testHaversineDistanceAccuracy() {
        // New Delhi Connaught Place to India Gate (~2.3 km)
        double cpLat = 28.6315, cpLng = 77.2167;
        double igLat = 28.6129, igLng = 77.2295;
        double dist = Validators::calculateHaversineDistanceKm(cpLat, cpLng, igLat, igLng);

        QVERIFY(dist > 2.0 && dist < 2.8);

        // Same point distance is 0.0
        double selfDist = Validators::calculateHaversineDistanceKm(cpLat, cpLng, cpLat, cpLng);
        QVERIFY(selfDist < 0.001);
    }

    void testShopModelHyperlocalFilteringAndFallback() {
        ShopModel model;
        QJsonArray shops;

        // Shop 1: 1 km away (inside 3km radius)
        QJsonObject s1;
        s1[QStringLiteral("id")] = QStringLiteral("s1");
        s1[QStringLiteral("name")] = QStringLiteral("Local Fresh Mart");
        s1[QStringLiteral("lat")] = 28.6200;
        s1[QStringLiteral("lng")] = 77.2100;
        shops.append(s1);

        // Shop 2: 12 km away (outside 3km radius)
        QJsonObject s2;
        s2[QStringLiteral("id")] = QStringLiteral("s2");
        s2[QStringLiteral("name")] = QStringLiteral("Distant Superstore");
        s2[QStringLiteral("lat")] = 28.5000;
        s2[QStringLiteral("lng")] = 77.1000;
        shops.append(s2);

        // Customer at Delhi center: 28.6139, 77.2090
        model.populateFromJson(shops, 28.6139, 77.2090);

        // Only Shop 1 should be included; Shop 2 should be filtered out (>3km)
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.getShopAt(0)[QStringLiteral("id")].toString(), QStringLiteral("s1"));

        // When GPS location is absent (0.0, 0.0) -> Fallback mode: all stores listed with distance = 0.0
        model.populateFromJson(shops, 0.0, 0.0);
        QCOMPARE(model.count(), 2);
    }

    void testPermissionDeniedAndManualAddressFlow() {
        ShopModel model;
        QJsonArray shops;

        // Shop near Indiranagar Bangalore (12.9716, 77.5946)
        QJsonObject s1;
        s1[QStringLiteral("id")] = QStringLiteral("s_blr_1");
        s1[QStringLiteral("name")] = QStringLiteral("Indiranagar Organic");
        s1[QStringLiteral("lat")] = 12.9716;
        s1[QStringLiteral("lng")] = 77.5946;
        shops.append(s1);

        // Shop in Whitefield Bangalore (~18km away)
        QJsonObject s2;
        s2[QStringLiteral("id")] = QStringLiteral("s_blr_2");
        s2[QStringLiteral("name")] = QStringLiteral("Whitefield Mega Store");
        s2[QStringLiteral("lat")] = 12.9698;
        s2[QStringLiteral("lng")] = 77.7500;
        shops.append(s2);

        // Scenario 1: User denies GPS permission -> Fallback coordinates (0.0, 0.0)
        // With GPS denied, ShopModel enters fallback mode and shows all stores without distance filter
        model.populateFromJson(shops, 0.0, 0.0);
        QCOMPARE(model.count(), 2);

        // Scenario 2: User enters manual address
        // Invalid manual address coordinates rejected
        QVERIFY(Validators::validateCoordinates(100.0, 77.0).isError());

        // Valid manual address geocoded: Customer manually selects Indiranagar (12.9720, 77.5950)
        double manualLat = 12.9720;
        double manualLng = 77.5950;
        QVERIFY(Validators::validateCoordinates(manualLat, manualLng).isSuccess());

        // Re-filtering with manual address coordinates applied
        model.populateFromJson(shops, manualLat, manualLng);
        QCOMPARE(model.count(), 1); // Only s1 within 3km
        QCOMPARE(model.getShopAt(0)[QStringLiteral("id")].toString(), QStringLiteral("s_blr_1"));

        // Verify calculated distance is < 0.5km
        double dist = model.getShopAt(0)[QStringLiteral("distance")].toDouble();
        QVERIFY(dist > 0.0 && dist < 0.5);
    }
};

QTEST_MAIN(TestLocationFallback)
#include "test_location_fallback.moc"
