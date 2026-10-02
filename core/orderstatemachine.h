/**
 * @file orderstatemachine.h
 * @brief Finite State Machine governing the QuickCart order lifecycle with RBAC guards.
 * @layer Core (Layer 1 - Foundations)
 *
 * Public API Summary:
 * - enum class OrderStatus: Pending, Accepted, Preparing, Ready, Assigned, PickedUp, Delivered, Cancelled, Rejected.
 * - enum class OrderActor: Customer, Merchant, Courier, Admin, System.
 * - statusToString(OrderStatus status): Standardized lowercase status representation.
 * - statusFromString(const QString &str): Parse string to OrderStatus.
 * - isTerminalState(OrderStatus status): Check if state is final (Delivered, Cancelled, Rejected).
 * - canTransition(OrderStatus from, OrderStatus to, OrderActor actor): Validate lifecycle transition.
 * - transition(OrderStatus current, OrderStatus next, OrderActor actor): Execute transition or return AppError.
 *
 * Dependencies:
 * - core/result.h
 *
 * Tests:
 * - Covered by tests/cpp/test_orderstatemachine.cpp
 */

#ifndef ORDERSTATEMACHINE_H
#define ORDERSTATEMACHINE_H

#include "result.h"
#include <QtCore/QString>
#include <QtCore/QSet>

class OrderStateMachine {
public:
    enum class OrderStatus {
        Unknown,
        Pending,
        Accepted,
        Preparing,
        Ready,
        Assigned,
        PickedUp,
        Delivered,
        Cancelled,
        Rejected
    };

    enum class OrderActor { Unknown, Customer, Merchant, Courier, Admin, System };

    /**
     * @brief Convert OrderStatus to lowercase API-standard string.
     */
    static QString statusToString(OrderStatus status);

    /**
     * @brief Parse string to OrderStatus.
     */
    static OrderStatus statusFromString(const QString &statusStr);

    /**
     * @brief Convert OrderActor to string.
     */
    static QString actorToString(OrderActor actor);

    /**
     * @brief Parse string to OrderActor.
     */
    static OrderActor actorFromString(const QString &actorStr);

    /**
     * @brief Check whether the status is terminal (cannot transition further).
     */
    static bool isTerminalState(OrderStatus status);

    /**
     * @brief Check whether a transition from state 'from' to state 'to' by 'actor' is permitted.
     * @param from Current order status.
     * @param to Target order status.
     * @param actor The entity attempting the change.
     * @return Result::ok() if allowed, Result::error() explaining why forbidden.
     */
    static Result<void> canTransition(OrderStatus from, OrderStatus to, OrderActor actor);

    /**
     * @brief Perform an order lifecycle transition.
     * @param current Current order status.
     * @param next Target order status.
     * @param actor Entity performing the action.
     * @return Result::ok(next) if valid, Result::error() otherwise.
     */
    static Result<OrderStatus> transition(OrderStatus current, OrderStatus next, OrderActor actor);
};

#endif // ORDERSTATEMACHINE_H
