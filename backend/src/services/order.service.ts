import { Db, MongoClient, ObjectId } from 'mongodb';
import crypto from 'crypto';
import { config } from '../config';

export interface CreateOrderParams {
  customerId: string;
  shopId: string;
  deliveryAddress: string;
  items: Array<{ productId: string; quantity: number; pricePaise?: number }>;
  idempotencyKey: string;
}

export class OrderService {
  constructor(private db: Db, private client: MongoClient) {}

  public async calculateCart(items: Array<{ productId?: string; quantity: number; pricePaise: number }>) {
    let subtotalPaise = 0;
    for (const it of items) {
      const qty = Math.max(1, it.quantity || 1);
      const price = Number(it.pricePaise) || 0;
      subtotalPaise += (qty * price);
    }

    const deliveryFeePaise = (subtotalPaise === 0 || subtotalPaise >= config.freeDeliveryThresholdPaise)
      ? 0
      : config.defaultDeliveryFeePaise;

    const totalPaise = subtotalPaise + deliveryFeePaise;

    return {
      subtotalPaise,
      deliveryFeePaise,
      totalPaise
    };
  }

  public async createOrder(params: CreateOrderParams) {
    const { customerId, shopId, deliveryAddress, items, idempotencyKey } = params;

    if (!idempotencyKey) {
      throw { status: 400, message: 'Idempotency key is required' };
    }

    if (!items || items.length === 0) {
      throw { status: 400, message: 'Order must contain at least one item' };
    }

    // 1. Check existing idempotency key
    const existingKey = await this.db.collection('idempotency_keys').findOne({ key: idempotencyKey });
    if (existingKey) {
      if (existingKey.responseBody) {
        return { order: existingKey.responseBody, cached: true };
      }
      throw { status: 409, message: 'Concurrent order creation in progress for this idempotency key' };
    }

    // Check if client supports transactions (replica set mode)
    const session = this.client.startSession();
    try {
      let createdOrder: any = null;

      await session.withTransaction(async () => {
        // Double-check idempotency key inside transaction
        const inTxKey = await this.db.collection('idempotency_keys').findOne({ key: idempotencyKey }, { session });
        if (inTxKey && inTxKey.responseBody) {
          createdOrder = inTxKey.responseBody;
          return;
        }

        // Fetch product & inventory details for price calculation & stock check
        let subtotalPaise = 0;
        const orderItems: any[] = [];

        for (const item of items) {
          const pId = ObjectId.isValid(item.productId) ? new ObjectId(item.productId) : item.productId;
          const sId = ObjectId.isValid(shopId) ? new ObjectId(shopId) : shopId;

          // Find product from database (server is source of truth for price)
          let product = await this.db.collection('products').findOne(
            { $or: [{ _id: pId as any }, { _id: item.productId as any }] },
            { session }
          );

          const pricePaise = product
            ? (product.sellingPricePaise || product.pricePaise || Math.round((product.price || 0) * 100))
            : (item.pricePaise || 1000);

          const productName = product ? product.name : `Product ${item.productId}`;
          const qty = Math.max(1, item.quantity);
          const lineTotal = pricePaise * qty;
          subtotalPaise += lineTotal;

          orderItems.push({
            productId: item.productId,
            name: productName,
            quantity: qty,
            unitPricePaise: pricePaise,
            totalPricePaise: lineTotal
          });

          // Check & decrement inventory stock atomically
          if (product) {
            const invResult = await this.db.collection('inventory').updateOne(
              {
                $or: [{ productId: pId as any }, { productId: item.productId as any }],
                stock: { $gte: qty }
              },
              { $inc: { stock: -qty }, $set: { updatedAt: new Date() } },
              { session }
            );

            // Also update product stock field if maintained directly on product
            await this.db.collection('products').updateOne(
              { _id: product._id, stock: { $gte: qty } },
              { $inc: { stock: -qty } },
              { session }
            );
          }
        }

        const deliveryFeePaise = (subtotalPaise === 0 || subtotalPaise >= config.freeDeliveryThresholdPaise)
          ? 0
          : config.defaultDeliveryFeePaise;
        const totalPaise = subtotalPaise + deliveryFeePaise;

        const orderId = 'order_' + crypto.randomUUID().replace(/-/g, '');
        const deliveryOtp = crypto.randomInt(1000, 9999).toString();

        const orderDoc = {
          _id: orderId,
          orderNumber: 'QC-' + Date.now().toString().slice(-6),
          customerId,
          shopId,
          delivery_boy_id: null,
          courierId: null,
          status: 'pending',
          deliveryAddress,
          items: orderItems,
          pricing: {
            itemSubtotalPaise: subtotalPaise,
            deliveryFeePaise,
            totalPaise
          },
          subtotalPaise,
          deliveryFeePaise,
          totalPaise,
          idempotency_key: idempotencyKey,
          deliveryOtp,
          created_at: new Date().toISOString(),
          createdAt: new Date(),
          updatedAt: new Date()
        };

        await this.db.collection('orders').insertOne(orderDoc as any, { session });

        // Record idempotency record
        await this.db.collection('idempotency_keys').insertOne({
          key: idempotencyKey,
          customerId,
          orderId,
          responseBody: orderDoc,
          createdAt: new Date(),
          expiresAt: new Date(Date.now() + 24 * 60 * 60 * 1000)
        }, { session });

        createdOrder = orderDoc;
      });

      return { order: createdOrder, cached: false };
    } finally {
      await session.endSession();
    }
  }

  public async getOrderById(orderId: string) {
    const oId = ObjectId.isValid(orderId) ? new ObjectId(orderId) : orderId;
    return await this.db.collection('orders').findOne({
      $or: [{ _id: oId as any }, { _id: orderId as any }]
    });
  }

  public async listOrders(filter: any = {}) {
    return await this.db.collection('orders').find(filter).sort({ createdAt: -1 }).toArray();
  }
}
