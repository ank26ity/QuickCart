import { Db, ObjectId, ClientSession } from 'mongodb';
import { BaseRepository, RepositoryError } from './base.repository';
import { Order, OrderStatus, UserRole } from '../types/models';

export class OrderRepository extends BaseRepository<Order> {
  constructor(db: Db) {
    super(db, 'orders');
  }

  private static readonly VALID_TRANSITIONS: Record<OrderStatus, OrderStatus[]> = {
    pending: ['preparing', 'rejected', 'cancelled'],
    preparing: ['ready', 'cancelled'],
    ready: ['out_for_delivery', 'cancelled'],
    out_for_delivery: ['delivered', 'cancelled'],
    delivered: [],
    cancelled: [],
    rejected: []
  };

  /**
   * Enforces State-Machine transitions and actor permissions atomically in MongoDB
   */
  public async transitionOrderStatus(
    orderId: ObjectId,
    fromStatus: OrderStatus,
    toStatus: OrderStatus,
    actorId: ObjectId,
    actorRole: UserRole,
    deliveryOtp?: string
  ): Promise<Order> {
    // 1. Verify valid FSM transition
    const allowed = OrderRepository.VALID_TRANSITIONS[fromStatus] || [];
    if (!allowed.includes(toStatus)) {
      throw new RepositoryError(
        'VALIDATION_FAILED',
        `Illegal state transition from ${fromStatus} to ${toStatus}`
      );
    }

    // 2. Build filter enforcing existing status and ownership
    const filter: any = {
      _id: orderId,
      status: fromStatus,
      deletedAt: null
    };

    if (actorRole === 'shopkeeper') {
      // Merchant can only change orders for their shop
      const shop = await this.db.collection('shops').findOne({ ownerId: actorId });
      if (!shop) throw new RepositoryError('UNAUTHORIZED', 'Merchant does not own an active shop');
      filter.shopId = shop._id;
    } else if (actorRole === 'delivery') {
      // Courier can only transition orders assigned to them
      filter.courierId = actorId;
      if (toStatus === 'delivered' && deliveryOtp) {
        filter.deliveryOtp = deliveryOtp;
      }
    } else if (actorRole === 'customer') {
      filter.customerId = actorId;
    }
    // Admin can transition any order

    const update: any = {
      $set: {
        status: toStatus,
        updatedAt: new Date()
      },
      $inc: { version: 1 }
    };

    const result = await this.collection.findOneAndUpdate(filter, update, { returnDocument: 'after' });
    if (!result) {
      throw new RepositoryError(
        'VERSION_CONFLICT',
        `Failed to transition order status. Order may have already changed status or access was denied.`
      );
    }

    // Record order event
    await this.db.collection('order_events').insertOne({
      _id: new ObjectId(),
      orderId,
      fromStatus,
      toStatus,
      actorId,
      actorRole,
      timestamp: new Date(),
      createdAt: new Date(),
      updatedAt: new Date(),
      version: 1
    });

    return result as Order;
  }
}
