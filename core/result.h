/**
 * @file result.h
 * @brief Type-safe monadic Result<T> and AppError error-handling primitives.
 * @layer Core (Layer 1 - Foundations)
 *
 * Public API Summary:
 * - enum class ErrorCategory: Granular error domains (Network, Auth, Validation, Conflict, etc.)
 * - struct AppError: Structured error information with category, HTTP status code, message, and error code.
 * - template <typename T> class Result: Monadic container holding either a valid value T or an AppError.
 * - template <> class Result<void>: Specialization for operations that succeed without returning a payload.
 *
 * Dependencies:
 * - <variant>, <QtCore/QString>, <QtCore/QJsonObject>
 *
 * Tests:
 * - Covered by tests/cpp/test_result.cpp
 */

#ifndef RESULT_H
#define RESULT_H

#include <QtCore/QString>
#include <QtCore/QJsonObject>
#include <variant>
#include <functional>

/**
 * @brief Categorization of application error conditions for UI mapping and retry decisions.
 */
enum class ErrorCategory {
    None,
    Network,
    Authentication,
    Authorization,
    Validation,
    NotFound,
    Server,
    Timeout,
    Conflict,
    RateLimited,
    Storage,
    Unknown
};

/**
 * @brief Structured error container with machine-readable codes and user messages.
 */
struct AppError {
    ErrorCategory category{ErrorCategory::None};
    int statusCode{0};
    QString message;
    QString details;
    QString errorCode;

    /**
     * @brief Check whether this error represents an actual failure.
     * @return True if category is not None.
     */
    bool isError() const { return category != ErrorCategory::None; }

    /**
     * @brief Factory for an empty, non-error sentinel.
     */
    static AppError none() { return AppError{ErrorCategory::None, 0, QString(), QString(), QString()}; }

    /**
     * @brief Factory for network-related errors.
     */
    static AppError network(const QString &msg, int code = 0) {
        return AppError{ErrorCategory::Network, code, msg, QString(), QStringLiteral("ERR_NETWORK")};
    }

    /**
     * @brief Factory for authentication failures (HTTP 401).
     */
    static AppError auth(const QString &msg, const QString &errCode = QStringLiteral("ERR_AUTH")) {
        return AppError{ErrorCategory::Authentication, 401, msg, QString(), errCode};
    }

    /**
     * @brief Factory for authorization/permission denials (HTTP 403).
     */
    static AppError forbidden(const QString &msg) {
        return AppError{ErrorCategory::Authorization, 403, msg, QString(), QStringLiteral("ERR_FORBIDDEN")};
    }

    /**
     * @brief Factory for input validation errors (HTTP 400).
     */
    static AppError validation(const QString &msg, const QString &details = QString()) {
        return AppError{ErrorCategory::Validation, 400, msg, details, QStringLiteral("ERR_VALIDATION")};
    }

    /**
     * @brief Factory for resource conflict errors (HTTP 409).
     */
    static AppError conflict(const QString &msg, const QString &errCode = QStringLiteral("ERR_CONFLICT")) {
        return AppError{ErrorCategory::Conflict, 409, msg, QString(), errCode};
    }

    /**
     * @brief Factory for rate limiting errors (HTTP 429).
     */
    static AppError rateLimited(const QString &msg) {
        return AppError{ErrorCategory::RateLimited, 429, msg, QString(), QStringLiteral("ERR_RATE_LIMITED")};
    }

    /**
     * @brief Factory for server-side failures (HTTP 500+).
     */
    static AppError server(const QString &msg, int code = 500) {
        return AppError{ErrorCategory::Server, code, msg, QString(), QStringLiteral("ERR_SERVER")};
    }

    /**
     * @brief Factory for local secure storage failures.
     */
    static AppError storage(const QString &msg) {
        return AppError{ErrorCategory::Storage, 0, msg, QString(), QStringLiteral("ERR_STORAGE")};
    }

    /**
     * @brief Factory for timeout conditions.
     */
    static AppError timeout(const QString &msg) {
        return AppError{ErrorCategory::Timeout, 408, msg, QString(), QStringLiteral("ERR_TIMEOUT")};
    }
};

/**
 * @brief Type-safe Result container representing either success with value T or failure with AppError.
 */
template<typename T>
class Result {
public:
    /**
     * @brief Construct a successful result holding the given value.
     */
    static Result<T> ok(const T &val) { return Result<T>(val); }

    /**
     * @brief Construct a failed result with structured AppError.
     */
    static Result<T> error(const AppError &err) { return Result<T>(err); }

    /**
     * @brief Construct a failed result with message and optional category.
     */
    static Result<T> error(const QString &message, ErrorCategory category = ErrorCategory::Unknown) {
        AppError err;
        err.category = category;
        err.message = message;
        return Result<T>(err);
    }

    /**
     * @brief Check whether the operation succeeded.
     */
    bool isSuccess() const { return std::holds_alternative<T>(m_data); }

    /**
     * @brief Check whether the operation produced an error.
     */
    bool isError() const { return !isSuccess(); }

    /**
     * @brief Retrieve const reference to the enclosed value. Undefined if isError().
     */
    const T &value() const { return std::get<T>(m_data); }

    /**
     * @brief Retrieve mutable reference to the enclosed value. Undefined if isError().
     */
    T &value() { return std::get<T>(m_data); }

    /**
     * @brief Retrieve the value or a fallback default if in an error state.
     */
    T valueOr(const T &defaultValue) const { return isSuccess() ? std::get<T>(m_data) : defaultValue; }

    /**
     * @brief Retrieve const reference to the enclosed error. Undefined if isSuccess().
     */
    const AppError &error() const { return std::get<AppError>(m_data); }

private:
    explicit Result(const T &val) : m_data(val) {}
    explicit Result(const AppError &err) : m_data(err) {}

    std::variant<T, AppError> m_data;
};

/**
 * @brief Specialization of Result for operations without a payload.
 */
template<>
class Result<void> {
public:
    /**
     * @brief Construct a successful void result.
     */
    static Result<void> ok() { return Result<void>(); }

    /**
     * @brief Construct a failed void result with AppError.
     */
    static Result<void> error(const AppError &err) { return Result<void>(err); }

    /**
     * @brief Construct a failed void result with message and optional category.
     */
    static Result<void> error(const QString &message, ErrorCategory category = ErrorCategory::Unknown) {
        AppError err;
        err.category = category;
        err.message = message;
        return Result<void>(err);
    }

    /**
     * @brief Check whether the void operation succeeded.
     */
    bool isSuccess() const { return !m_error.isError(); }

    /**
     * @brief Check whether the void operation failed.
     */
    bool isError() const { return m_error.isError(); }

    /**
     * @brief Retrieve the AppError.
     */
    const AppError &error() const { return m_error; }

private:
    Result() : m_error(AppError::none()) {}
    explicit Result(const AppError &err) : m_error(err) {}

    AppError m_error;
};

#endif // RESULT_H
