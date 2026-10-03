/**
 * @file validators.h
 * @brief Input and business rule validators for the QuickCart application.
 * @layer Core (Layer 1 - Foundations)
 *
 * Public API Summary:
 * - validateEmail(const QString &email): RFC 5322 regex validation.
 * - validatePhone(const QString &phone): E.164 and 10-digit telephone format checking.
 * - validatePassword(const QString &password): Minimum length and character complexity validation.
 * - validatePricePaise(qint64 paise): Positive currency value validation in integer paise with boundary guards.
 * - validateQuantity(int quantity, int stockLimit): Item count bounds against real-time inventory limits.
 * - validateCoordinates(double lat, double lng): Geographical coordinate bounding (-90..90, -180..180).
 * - calculateHaversineDistanceKm(...): Great-circle distance computation between geographic points.
 * - validateWithinDeliveryRadius(...): Hyperlocal delivery radius validation (3 km limit).
 * - validateOtp(const QString &otp, int requiredLength): Strict numeric one-time password verification.
 * - validateOpeningHours(int openMinutes, int closeMinutes): Operating schedule interval verification.
 *
 * Dependencies:
 * - core/result.h
 *
 * Tests:
 * - Covered by tests/cpp/test_validators.cpp
 */

#ifndef VALIDATORS_H
#define VALIDATORS_H

#include "result.h"
#include <QtCore/QString>

class Validators {
public:
    /**
     * @brief Validate user email address format according to RFC 5322 specifications.
     * @param email The email string to validate.
     * @return Result::ok() on valid email, Result::error() with Validation category otherwise.
     */
    static Result<void> validateEmail(const QString &email);

    /**
     * @brief Validate phone number format (supports E.164 + international prefixes and standard 10-digit formats).
     * @param phone The telephone string to validate.
     * @return Result::ok() on valid phone, Result::error() otherwise.
     */
    static Result<void> validatePhone(const QString &phone);

    /**
     * @brief Validate user password strength.
     * Requires at least 8 characters, at least 1 uppercase letter, 1 lowercase letter, and 1 digit.
     * @param password The raw password string.
     * @return Result::ok() on strong password, Result::error() detailing missing requirements otherwise.
     */
    static Result<void> validatePassword(const QString &password);

    /**
     * @brief Validate product or cart item price in integer paise.
     * @param paise Value to check in minor units (paise). Must be > 0 and <= 50,000,000 (₹500,000.00).
     * @return Result::ok() on valid price, Result::error() otherwise.
     */
    static Result<void> validatePricePaise(qint64 paise);

    /**
     * @brief Validate requested cart purchase quantity against merchant stock.
     * @param quantity The requested item count (must be >= 1).
     * @param stockLimit The currently available stock count (must be >= 0).
     * @return Result::ok() on valid purchase count, Result::error() otherwise.
     */
    static Result<void> validateQuantity(int quantity, int stockLimit);

    /**
     * @brief Validate latitude and longitude coordinate bounds.
     * @param lat Latitude (-90.0 to 90.0).
     * @param lng Longitude (-180.0 to 180.0).
     * @return Result::ok() on valid coordinates, Result::error() otherwise.
     */
    static Result<void> validateCoordinates(double lat, double lng);

    /**
     * @brief Calculate the spherical distance in kilometers between two GPS coordinates using the Haversine formula.
     * @param lat1 Latitude of point 1.
     * @param lon1 Longitude of point 1.
     * @param lat2 Latitude of point 2.
     * @param lon2 Longitude of point 2.
     * @return Distance in kilometers.
     */
    static double calculateHaversineDistanceKm(double lat1, double lon1, double lat2, double lon2);

    /**
     * @brief Validate whether a user is within the delivery radius of a merchant shop (default 3.0 km).
     * @param userLat Customer latitude.
     * @param userLng Customer longitude.
     * @param shopLat Shop latitude.
     * @param shopLng Shop longitude.
     * @param maxRadiusKm Hyperlocal perimeter limit in km (default 3.0 km).
     * @return Result::ok(distanceKm) if within radius, Result::error() if beyond boundary.
     */
    static Result<double> validateWithinDeliveryRadius(double userLat, double userLng, double shopLat, double shopLng,
                                                       double maxRadiusKm = 3.0);

    /**
     * @brief Validate a numeric OTP verification code.
     * @param otp The string code.
     * @param requiredLength Expected length (e.g., 4 or 6 digits).
     * @return Result::ok() on valid numeric OTP, Result::error() otherwise.
     */
    static Result<void> validateOtp(const QString &otp, int requiredLength = 4);

    /**
     * @brief Validate merchant opening and closing times.
     * @param openMinutes Minutes from midnight for opening (0..1439).
     * @param closeMinutes Minutes from midnight for closing (0..1439).
     * @return Result::ok() if openMinutes < closeMinutes and in valid bounds, Result::error() otherwise.
     */
    static Result<void> validateOpeningHours(int openMinutes, int closeMinutes);
};

#endif // VALIDATORS_H
