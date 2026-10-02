/**
 * @file validators.cpp
 * @brief Implementation of input and domain validators.
 * @layer Core (Layer 1 - Foundations)
 * @tests Covered by tests/cpp/test_validators.cpp
 */

#include "validators.h"
#include <QtCore/QRegularExpression>
#include <QtMath>
#include <cmath>

Result<void> Validators::validateEmail(const QString &email)
{
    QString trimmed = email.trimmed();
    if (trimmed.isEmpty()) {
        return Result<void>::error(AppError::validation(QStringLiteral("Email address cannot be empty.")));
    }

    static const QRegularExpression emailRegex(
        QStringLiteral("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$")
    );

    if (!emailRegex.match(trimmed).hasMatch()) {
        return Result<void>::error(AppError::validation(QStringLiteral("Invalid email address format.")));
    }

    return Result<void>::ok();
}

Result<void> Validators::validatePhone(const QString &phone)
{
    QString clean = phone.trimmed();
    clean.remove(QLatin1Char(' '));
    clean.remove(QLatin1Char('-'));
    clean.remove(QLatin1Char('('));
    clean.remove(QLatin1Char(')'));

    if (clean.isEmpty()) {
        return Result<void>::error(AppError::validation(QStringLiteral("Phone number cannot be empty.")));
    }

    // Standard E.164 phone regex (7 to 15 digits, optional leading +)
    static const QRegularExpression phoneRegex(QStringLiteral("^\\+?[1-9]\\d{6,14}$"));

    if (!phoneRegex.match(clean).hasMatch()) {
        return Result<void>::error(AppError::validation(QStringLiteral("Invalid phone number format.")));
    }

    return Result<void>::ok();
}

Result<void> Validators::validatePassword(const QString &password)
{
    if (password.length() < 8) {
        return Result<void>::error(AppError::validation(QStringLiteral("Password must be at least 8 characters long.")));
    }

    bool hasUpper = false;
    bool hasLower = false;
    bool hasDigit = false;

    for (const QChar &ch : password) {
        if (ch.isUpper()) hasUpper = true;
        else if (ch.isLower()) hasLower = true;
        else if (ch.isDigit()) hasDigit = true;
    }

    if (!hasUpper || !hasLower || !hasDigit) {
        return Result<void>::error(AppError::validation(
            QStringLiteral("Password must include at least one uppercase letter, one lowercase letter, and one number.")
        ));
    }

    return Result<void>::ok();
}

Result<void> Validators::validatePrice(double price)
{
    if (std::isnan(price) || std::isinf(price)) {
        return Result<void>::error(AppError::validation(QStringLiteral("Price value is invalid.")));
    }
    if (price <= 0.0) {
        return Result<void>::error(AppError::validation(QStringLiteral("Price must be greater than zero.")));
    }
    if (price > 500000.0) {
        return Result<void>::error(AppError::validation(QStringLiteral("Price exceeds maximum permitted limit.")));
    }

    return Result<void>::ok();
}

Result<void> Validators::validatePricePaise(qint64 paise)
{
    if (paise <= 0) {
        return Result<void>::error(AppError::validation(QStringLiteral("Price in paise must be strictly positive.")));
    }
    if (paise > 50000000) { // ₹500,000.00
        return Result<void>::error(AppError::validation(QStringLiteral("Price in paise exceeds maximum transaction limit of ₹500,000.00.")));
    }
    return Result<void>::ok();
}

Result<void> Validators::validateQuantity(int quantity, int stockLimit)
{
    if (quantity <= 0) {
        return Result<void>::error(AppError::validation(QStringLiteral("Quantity must be at least 1.")));
    }
    if (stockLimit <= 0) {
        return Result<void>::error(AppError::conflict(QStringLiteral("Product is currently out of stock."), QStringLiteral("ERR_OUT_OF_STOCK")));
    }
    if (quantity > stockLimit) {
        return Result<void>::error(AppError::conflict(
            QString(QStringLiteral("Requested quantity (%1) exceeds available stock (%2).")).arg(quantity).arg(stockLimit),
            QStringLiteral("ERR_STOCK_LIMIT_EXCEEDED")
        ));
    }

    return Result<void>::ok();
}

Result<void> Validators::validateCoordinates(double lat, double lng)
{
    if (std::isnan(lat) || std::isnan(lng) || std::isinf(lat) || std::isinf(lng)) {
        return Result<void>::error(AppError::validation(QStringLiteral("Coordinates must be valid numbers.")));
    }
    if (lat < -90.0 || lat > 90.0) {
        return Result<void>::error(AppError::validation(QStringLiteral("Latitude must be between -90.0 and +90.0.")));
    }
    if (lng < -180.0 || lng > 180.0) {
        return Result<void>::error(AppError::validation(QStringLiteral("Longitude must be between -180.0 and +180.0.")));
    }

    return Result<void>::ok();
}

double Validators::calculateHaversineDistanceKm(double lat1, double lon1, double lat2, double lon2)
{
    constexpr double EARTH_RADIUS_KM = 6371.0;

    double dLat = qDegreesToRadians(lat2 - lat1);
    double dLon = qDegreesToRadians(lon2 - lon1);

    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(qDegreesToRadians(lat1)) * std::cos(qDegreesToRadians(lat2)) *
               std::sin(dLon / 2.0) * std::sin(dLon / 2.0);

    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return EARTH_RADIUS_KM * c;
}

Result<double> Validators::validateWithinDeliveryRadius(double userLat, double userLng,
                                                      double shopLat, double shopLng,
                                                      double maxRadiusKm)
{
    auto userCoordRes = validateCoordinates(userLat, userLng);
    if (userCoordRes.isError()) {
        return Result<double>::error(userCoordRes.error());
    }

    auto shopCoordRes = validateCoordinates(shopLat, shopLng);
    if (shopCoordRes.isError()) {
        return Result<double>::error(shopCoordRes.error());
    }

    double distanceKm = calculateHaversineDistanceKm(userLat, userLng, shopLat, shopLng);
    if (distanceKm > maxRadiusKm) {
        return Result<double>::error(AppError::validation(
            QString(QStringLiteral("Shop is %1 km away, which exceeds the %2 km delivery radius."))
                .arg(QString::number(distanceKm, 'f', 2))
                .arg(QString::number(maxRadiusKm, 'f', 1)),
            QStringLiteral("ERR_OUT_OF_DELIVERY_RADIUS")
        ));
    }

    return Result<double>::ok(distanceKm);
}

Result<void> Validators::validateOtp(const QString &otp, int requiredLength)
{
    QString trimmed = otp.trimmed();
    if (trimmed.length() != requiredLength) {
        return Result<void>::error(AppError::validation(
            QString(QStringLiteral("OTP must be exactly %1 digits long.")).arg(requiredLength)
        ));
    }

    for (const QChar &ch : trimmed) {
        if (!ch.isDigit()) {
            return Result<void>::error(AppError::validation(QStringLiteral("OTP must contain digits only.")));
        }
    }

    return Result<void>::ok();
}

Result<void> Validators::validateOpeningHours(int openMinutes, int closeMinutes)
{
    if (openMinutes < 0 || openMinutes > 1439 || closeMinutes < 0 || closeMinutes > 1440) {
        return Result<void>::error(AppError::validation(QStringLiteral("Operating hours minutes must be between 0 and 1440.")));
    }
    if (openMinutes >= closeMinutes) {
        return Result<void>::error(AppError::validation(QStringLiteral("Opening time must be strictly before closing time.")));
    }

    return Result<void>::ok();
}
