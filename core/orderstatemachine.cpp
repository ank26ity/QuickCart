/**
 * @file orderstatemachine.cpp
 * @brief Implementation of OrderStateMachine with transitions and actor checks.
 * @layer Core (Layer 1 - Foundations)
 * @tests Covered by tests/cpp/test_orderstatemachine.cpp
 */

#include "orderstatemachine.h"

QString OrderStateMachine::statusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::Pending:
            return QStringLiteral("pending");
        case OrderStatus::Accepted:
            return QStringLiteral("accepted");
        case OrderStatus::Preparing:
            return QStringLiteral("preparing");
        case OrderStatus::Ready:
            return QStringLiteral("ready");
        case OrderStatus::Assigned:
            return QStringLiteral("assigned");
        case OrderStatus::PickedUp:
            return QStringLiteral("picked_up");
        case OrderStatus::Delivered:
            return QStringLiteral("delivered");
        case OrderStatus::Cancelled:
            return QStringLiteral("cancelled");
        case OrderStatus::Rejected:
            return QStringLiteral("rejected");
        default:
            return QStringLiteral("unknown");
    }
}

OrderStateMachine::OrderStatus OrderStateMachine::statusFromString(const QString &statusStr) {
    QString s = statusStr.trimmed().toLower();
    if (s == QStringLiteral("pending"))
        return OrderStatus::Pending;
    if (s == QStringLiteral("accepted"))
        return OrderStatus::Accepted;
    if (s == QStringLiteral("preparing"))
        return OrderStatus::Preparing;
    if (s == QStringLiteral("ready"))
        return OrderStatus::Ready;
    if (s == QStringLiteral("assigned"))
        return OrderStatus::Assigned;
    if (s == QStringLiteral("picked_up") || s == QStringLiteral("pickedup"))
        return OrderStatus::PickedUp;
    if (s == QStringLiteral("delivered"))
        return OrderStatus::Delivered;
    if (s == QStringLiteral("cancelled") || s == QStringLiteral("canceled"))
        return OrderStatus::Cancelled;
    if (s == QStringLiteral("rejected"))
        return OrderStatus::Rejected;
    return OrderStatus::Unknown;
}

QString OrderStateMachine::actorToString(OrderActor actor) {
    switch (actor) {
        case OrderActor::Customer:
            return QStringLiteral("customer");
        case OrderActor::Merchant:
            return QStringLiteral("merchant");
        case OrderActor::Courier:
            return QStringLiteral("courier");
        case OrderActor::Admin:
            return QStringLiteral("admin");
        case OrderActor::System:
            return QStringLiteral("system");
        default:
            return QStringLiteral("unknown");
    }
}

OrderStateMachine::OrderActor OrderStateMachine::actorFromString(const QString &actorStr) {
    QString a = actorStr.trimmed().toLower();
    if (a == QStringLiteral("customer"))
        return OrderActor::Customer;
    if (a == QStringLiteral("merchant") || a == QStringLiteral("shopkeeper"))
        return OrderActor::Merchant;
    if (a == QStringLiteral("courier") || a == QStringLiteral("delivery") || a == QStringLiteral("rider"))
        return OrderActor::Courier;
    if (a == QStringLiteral("admin"))
        return OrderActor::Admin;
    if (a == QStringLiteral("system"))
        return OrderActor::System;
    return OrderActor::Unknown;
}

bool OrderStateMachine::isTerminalState(OrderStatus status) {
    return status == OrderStatus::Delivered || status == OrderStatus::Cancelled || status == OrderStatus::Rejected;
}

Result<void> OrderStateMachine::canTransition(OrderStatus from, OrderStatus to, OrderActor actor) {
    if (from == OrderStatus::Unknown || to == OrderStatus::Unknown) {
        return Result<void>::error(
            AppError::validation(QStringLiteral("Cannot transition between unrecognized order states."),
                                 QStringLiteral("ERR_UNKNOWN_STATUS")));
    }

    if (from == to) {
        return Result<void>::error(
            AppError::conflict(QString(QStringLiteral("Order is already in '%1' state.")).arg(statusToString(from)),
                               QStringLiteral("ERR_NOOP_TRANSITION")));
    }

    if (isTerminalState(from)) {
        return Result<void>::error(
            AppError::conflict(QString(QStringLiteral("Order is in terminal state '%1' and cannot be modified."))
                                   .arg(statusToString(from)),
                               QStringLiteral("ERR_TERMINAL_STATE")));
    }

    // Role-based transition matrix
    switch (from) {
        case OrderStatus::Pending:
            if (to == OrderStatus::Accepted) {
                if (actor == OrderActor::Merchant || actor == OrderActor::Admin)
                    return Result<void>::ok();
                return Result<void>::error(
                    AppError::forbidden(QStringLiteral("Only merchants or admins can accept pending orders.")));
            }
            if (to == OrderStatus::Rejected) {
                if (actor == OrderActor::Merchant || actor == OrderActor::Admin)
                    return Result<void>::ok();
                return Result<void>::error(
                    AppError::forbidden(QStringLiteral("Only merchants or admins can reject pending orders.")));
            }
            if (to == OrderStatus::Cancelled) {
                if (actor == OrderActor::Customer || actor == OrderActor::Admin || actor == OrderActor::System)
                    return Result<void>::ok();
                return Result<void>::error(
                    AppError::forbidden(QStringLiteral("Courier cannot cancel a pending order.")));
            }
            break;

        case OrderStatus::Accepted:
            if (to == OrderStatus::Preparing) {
                if (actor == OrderActor::Merchant || actor == OrderActor::Admin)
                    return Result<void>::ok();
                return Result<void>::error(
                    AppError::forbidden(QStringLiteral("Only merchants or admins can move an order to preparing.")));
            }
            if (to == OrderStatus::Cancelled) {
                if (actor == OrderActor::Admin || actor == OrderActor::Merchant)
                    return Result<void>::ok();
                return Result<void>::error(
                    AppError::forbidden(QStringLiteral("Customer cannot cancel an order once accepted by merchant.")));
            }
            break;

        case OrderStatus::Preparing:
            if (to == OrderStatus::Ready) {
                if (actor == OrderActor::Merchant || actor == OrderActor::Admin)
                    return Result<void>::ok();
                return Result<void>::error(
                    AppError::forbidden(QStringLiteral("Only merchants or admins can mark an order ready.")));
            }
            if (to == OrderStatus::Cancelled) {
                if (actor == OrderActor::Admin)
                    return Result<void>::ok();
                return Result<void>::error(
                    AppError::forbidden(QStringLiteral("Only admin can cancel an order being prepared.")));
            }
            break;

        case OrderStatus::Ready:
            if (to == OrderStatus::Assigned) {
                if (actor == OrderActor::Admin || actor == OrderActor::Courier)
                    return Result<void>::ok();
                return Result<void>::error(
                    AppError::forbidden(QStringLiteral("Only couriers or admins can assign ready orders.")));
            }
            if (to == OrderStatus::PickedUp) {
                if (actor == OrderActor::Courier || actor == OrderActor::Admin)
                    return Result<void>::ok();
                return Result<void>::error(
                    AppError::forbidden(QStringLiteral("Only couriers or admins can pick up orders.")));
            }
            if (to == OrderStatus::Cancelled) {
                if (actor == OrderActor::Admin)
                    return Result<void>::ok();
                return Result<void>::error(AppError::forbidden(QStringLiteral("Only admin can cancel a ready order.")));
            }
            break;

        case OrderStatus::Assigned:
            if (to == OrderStatus::PickedUp) {
                if (actor == OrderActor::Courier || actor == OrderActor::Admin)
                    return Result<void>::ok();
                return Result<void>::error(
                    AppError::forbidden(QStringLiteral("Only assigned courier or admin can pick up order.")));
            }
            if (to == OrderStatus::Ready) {
                // Courier unassignment / release back to pool
                if (actor == OrderActor::Courier || actor == OrderActor::Admin)
                    return Result<void>::ok();
                return Result<void>::error(
                    AppError::forbidden(QStringLiteral("Only courier or admin can unassign an order.")));
            }
            if (to == OrderStatus::Cancelled) {
                if (actor == OrderActor::Admin)
                    return Result<void>::ok();
                return Result<void>::error(
                    AppError::forbidden(QStringLiteral("Only admin can cancel an assigned order.")));
            }
            break;

        case OrderStatus::PickedUp:
            if (to == OrderStatus::Delivered) {
                if (actor == OrderActor::Courier || actor == OrderActor::Admin)
                    return Result<void>::ok();
                return Result<void>::error(
                    AppError::forbidden(QStringLiteral("Only delivering courier or admin can complete delivery.")));
            }
            if (to == OrderStatus::Cancelled) {
                if (actor == OrderActor::Admin)
                    return Result<void>::ok();
                return Result<void>::error(
                    AppError::forbidden(QStringLiteral("Only admin can cancel an in-transit order.")));
            }
            break;

        default:
            break;
    }

    return Result<void>::error(
        AppError::conflict(QString(QStringLiteral("Forbidden transition from '%1' to '%2' by '%3'."))
                               .arg(statusToString(from))
                               .arg(statusToString(to))
                               .arg(actorToString(actor)),
                           QStringLiteral("ERR_INVALID_TRANSITION")));
}

Result<OrderStateMachine::OrderStatus> OrderStateMachine::transition(OrderStatus current, OrderStatus next,
                                                                     OrderActor actor) {
    auto check = canTransition(current, next, actor);
    if (check.isError()) {
        return Result<OrderStatus>::error(check.error());
    }
    return Result<OrderStatus>::ok(next);
}
