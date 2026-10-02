/**
 * @file test_orderstatemachine.cpp
 * @brief Automated unit test suite for OrderStateMachine allowed/forbidden transitions and RBAC actor checks.
 * @layer Tests (C++ / Qt Test)
 */

#include <QtTest/QtTest>
#include "../../core/orderstatemachine.h"

using Status = OrderStateMachine::OrderStatus;
using Actor = OrderStateMachine::OrderActor;

class TestOrderStateMachine : public QObject
{
    Q_OBJECT

private slots:
    void testSerialization()
    {
        // Status string round-trips
        const QVector<Status> statuses = {
            Status::Pending, Status::Accepted, Status::Preparing, Status::Ready,
            Status::Assigned, Status::PickedUp, Status::Delivered, Status::Cancelled,
            Status::Rejected, Status::Unknown
        };
        for (Status s : statuses) {
            QString str = OrderStateMachine::statusToString(s);
            QVERIFY(!str.isEmpty());
            QCOMPARE(OrderStateMachine::statusFromString(str), s);
        }
        QCOMPARE(OrderStateMachine::statusFromString(QStringLiteral("canceled")), Status::Cancelled);
        QCOMPARE(OrderStateMachine::statusFromString(QStringLiteral("nonexistent")), Status::Unknown);

        // Actor string round-trips
        const QVector<Actor> actors = {
            Actor::Customer, Actor::Merchant, Actor::Courier, Actor::Admin, Actor::System, Actor::Unknown
        };
        for (Actor a : actors) {
            QString str = OrderStateMachine::actorToString(a);
            QVERIFY(!str.isEmpty());
            QCOMPARE(OrderStateMachine::actorFromString(str), a);
        }
        QCOMPARE(OrderStateMachine::actorFromString(QStringLiteral("invalid")), Actor::Unknown);
    }

    void testAllowedMerchantFlow()
    {
        // Pending -> Accepted (Merchant)
        auto r1 = OrderStateMachine::transition(Status::Pending, Status::Accepted, Actor::Merchant);
        QVERIFY(r1.isSuccess());
        QCOMPARE(r1.value(), Status::Accepted);

        // Accepted -> Preparing (Merchant)
        auto r2 = OrderStateMachine::transition(Status::Accepted, Status::Preparing, Actor::Merchant);
        QVERIFY(r2.isSuccess());
        QCOMPARE(r2.value(), Status::Preparing);

        // Preparing -> Ready (Merchant)
        auto r3 = OrderStateMachine::transition(Status::Preparing, Status::Ready, Actor::Merchant);
        QVERIFY(r3.isSuccess());
        QCOMPARE(r3.value(), Status::Ready);
    }

    void testAllowedCourierFlow()
    {
        // Ready -> Assigned (Courier claim)
        auto r1 = OrderStateMachine::transition(Status::Ready, Status::Assigned, Actor::Courier);
        QVERIFY(r1.isSuccess());

        // Assigned -> PickedUp (Courier pickup)
        auto r2 = OrderStateMachine::transition(Status::Assigned, Status::PickedUp, Actor::Courier);
        QVERIFY(r2.isSuccess());

        // PickedUp -> Delivered (Courier OTP confirmation)
        auto r3 = OrderStateMachine::transition(Status::PickedUp, Status::Delivered, Actor::Courier);
        QVERIFY(r3.isSuccess());
        QCOMPARE(r3.value(), Status::Delivered);
    }

    void testCustomerCancellation()
    {
        // Customer can cancel Pending order
        auto r1 = OrderStateMachine::transition(Status::Pending, Status::Cancelled, Actor::Customer);
        QVERIFY(r1.isSuccess());

        // Customer CANNOT cancel after order has been Accepted
        auto r2 = OrderStateMachine::transition(Status::Accepted, Status::Cancelled, Actor::Customer);
        QVERIFY(r2.isError());
        QCOMPARE(r2.error().category, ErrorCategory::Authorization);

        // Customer CANNOT cancel when in transit
        auto r3 = OrderStateMachine::transition(Status::PickedUp, Status::Cancelled, Actor::Customer);
        QVERIFY(r3.isError());
    }

    void testMerchantRejection()
    {
        // Merchant rejects pending order
        auto r1 = OrderStateMachine::transition(Status::Pending, Status::Rejected, Actor::Merchant);
        QVERIFY(r1.isSuccess());
        QCOMPARE(r1.value(), Status::Rejected);
    }

    void testAdminSuperuserPrivileges()
    {
        // Admin can cancel order at any active phase
        QVERIFY(OrderStateMachine::canTransition(Status::Pending, Status::Cancelled, Actor::Admin).isSuccess());
        QVERIFY(OrderStateMachine::canTransition(Status::Accepted, Status::Cancelled, Actor::Admin).isSuccess());
        QVERIFY(OrderStateMachine::canTransition(Status::Preparing, Status::Cancelled, Actor::Admin).isSuccess());
        QVERIFY(OrderStateMachine::canTransition(Status::Ready, Status::Cancelled, Actor::Admin).isSuccess());
        QVERIFY(OrderStateMachine::canTransition(Status::Assigned, Status::Cancelled, Actor::Admin).isSuccess());
        QVERIFY(OrderStateMachine::canTransition(Status::PickedUp, Status::Cancelled, Actor::Admin).isSuccess());
    }

    void testForbiddenActorTransitions()
    {
        // Courier cannot accept orders
        auto r1 = OrderStateMachine::transition(Status::Pending, Status::Accepted, Actor::Courier);
        QVERIFY(r1.isError());

        // Customer cannot mark ready
        auto r2 = OrderStateMachine::transition(Status::Preparing, Status::Ready, Actor::Customer);
        QVERIFY(r2.isError());

        // Merchant cannot complete delivery
        auto r3 = OrderStateMachine::transition(Status::PickedUp, Status::Delivered, Actor::Merchant);
        QVERIFY(r3.isError());
    }

    void testTerminalStatesCannotTransition()
    {
        // Delivered cannot transition anywhere
        QVERIFY(OrderStateMachine::isTerminalState(Status::Delivered));
        auto r1 = OrderStateMachine::transition(Status::Delivered, Status::Pending, Actor::Admin);
        QVERIFY(r1.isError());
        QCOMPARE(r1.error().category, ErrorCategory::Conflict);

        // Cancelled cannot transition anywhere
        QVERIFY(OrderStateMachine::isTerminalState(Status::Cancelled));
        auto r2 = OrderStateMachine::transition(Status::Cancelled, Status::Accepted, Actor::Merchant);
        QVERIFY(r2.isError());

        // Rejected cannot transition anywhere
        QVERIFY(OrderStateMachine::isTerminalState(Status::Rejected));
        auto r3 = OrderStateMachine::transition(Status::Rejected, Status::Preparing, Actor::Merchant);
        QVERIFY(r3.isError());
    }

    void testNoopTransition()
    {
        // Transitioning to same state must fail with conflict
        auto res = OrderStateMachine::transition(Status::Pending, Status::Pending, Actor::Customer);
        QVERIFY(res.isError());
        QCOMPARE(res.error().category, ErrorCategory::Conflict);
    }

    void testOrderCancellationLifecycle()
    {
        // Test explicit cancellation lifecycle matrix:
        // 1. Customer cancellation allowed in Pending
        QVERIFY(OrderStateMachine::canTransition(Status::Pending, Status::Cancelled, Actor::Customer).isSuccess());

        // 2. Merchant cancellation allowed in Pending and Accepted
        QVERIFY(OrderStateMachine::canTransition(Status::Pending, Status::Cancelled, Actor::Merchant).isError()); // Merchant uses Reject in pending
        QVERIFY(OrderStateMachine::canTransition(Status::Accepted, Status::Cancelled, Actor::Merchant).isSuccess());

        // 3. Admin cancellation allowed across all non-terminal stages
        QVERIFY(OrderStateMachine::canTransition(Status::Preparing, Status::Cancelled, Actor::Admin).isSuccess());

        // 4. Cancelled is an absolute terminal state: rejects all subsequent transitions
        QVERIFY(OrderStateMachine::isTerminalState(Status::Cancelled));
        auto r = OrderStateMachine::transition(Status::Cancelled, Status::Pending, Actor::Admin);
        QVERIFY(r.isError());
        QCOMPARE(r.error().category, ErrorCategory::Conflict);
    }
};

QTEST_MAIN(TestOrderStateMachine)
#include "test_orderstatemachine.moc"
