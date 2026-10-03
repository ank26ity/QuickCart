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

  public async getDeliveryConfig(session?: any) {
    const cfg = await this.db.collection('app_config').findOne({ key: 'delivery' }, { session });
    return {
      freeDeliveryThresholdPaise: cfg?.freeDeliveryThresholdPaise ?? config.freeDeliveryThresholdPaise ?? 49900,
      defaultDeliveryFeePaise: cfg?.defaultDeliveryFeePaise ?? config.defaultDeliveryFeePaise ?? 4900
    };
  }

  public async calculateCart(items: Array<{ productId?: string; quantity: number; pricePaise: number }>) {
    let subtotalPaise = 0;
    for (const it of items) {
      const qty = Math.max(1, it.quantity || 1);
      const price = Number(it.pricePaise) || 0;
      subtotalPaise += (qty * price);
    }

    const deliveryConfig = await this.getDeliveryConfig();
    const deliveryFeePaise = (subtotalPaise === 0 || subtotalPaise >= deliveryConfig.freeDeliveryThresholdPaise)
      ? 0
      : deliveryConfig.defaultDeliveryFeePaise;

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

    // 1. Validate items quantity: positive integer between 1 and 50
    for (const item of items) {
      if (typeof item.quantity !== 'number' || !Number.isInteger(item.quantity) || item.quantity <= 0 || item.quantity > 50) {
        throw {
          status: 400,
          error: 'Bad Request',
          message: `Invalid item quantity (${item.quantity}): quantity must be a positive integer between 1 and 50`
        };
      }
    }

    // Canonical request body representation for payload integrity verification
    const bodyCanonical = JSON.stringify({
      shopId: shopId.toString(),
      deliveryAddress,
      items: items.map(i => ({ productId: i.productId.toString(), quantity: i.quantity }))
    });
    const requestHash = crypto.createHash('sha256').update(bodyCanonical).digest('hex');

    // 2. Check existing idempotency key (scoped per user + payload hash check)
    const existingKey = await this.db.collection('idempotency_keys').findOne({ key: idempotencyKey });
    if (existingKey) {
      // Never return another user's order!
      if (existingKey.customerId && existingKey.customerId !== customerId) {
        throw {
          status: 403,
          error: 'Forbidden',
          message: 'Forbidden: Idempotency key belongs to another user'
        };
      }

      // Same key + different body -> HTTP 422 Unprocessable Entity
      if (existingKey.requestHash && existingKey.requestHash !== requestHash) {
        throw {
          status: 422,
          error: 'Unprocessable Entity',
          message: 'Idempotency conflict: request payload differs from previous request for this key'
        };
      }

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
        if (inTxKey) {
          if (inTxKey.customerId && inTxKey.customerId !== customerId) {
            throw { status: 403, error: 'Forbidden', message: 'Forbidden: Idempotency key belongs to another user' };
          }
          if (inTxKey.requestHash && inTxKey.requestHash !== requestHash) {
            throw { status: 422, error: 'Unprocessable Entity', message: 'Idempotency conflict: payload differs' };
          }
          if (inTxKey.responseBody) {
            createdOrder = inTxKey.responseBody;
            return;
          }
        }

        // Fetch product & inventory details for price calculation & stock check
        let subtotalPaise = 0;
        const orderItems: any[] = [];

        for (const item of items) {
          const pId = ObjectId.isValid(item.productId) ? new ObjectId(item.productId) : item.productId;

          // Find product from database (server is source of truth for price)
          let product = await this.db.collection('products').findOne(
            { $or: [{ _id: pId as any }, { _id: item.productId as any }] },
            { session }
          );

          if (!product) {
            throw {
              status: 400,
              error: 'Bad Request',
              message: `Product ${item.productId} not found`
            };
          }

          // 3. Products must belong to shopId
          const prodShopId = product.shopId ? product.shopId.toString() : (product.shop_id ? product.shop_id.toString() : '');
          if (prodShopId && prodShopId !== shopId.toString()) {
            throw {
              status: 400,
              error: 'Bad Request',
              message: `Cross-shop ordering rejected: Product ${item.productId} belongs to shop ${prodShopId}, not ${shopId}`
            };
          }

          // 4. Products must be active
          if (product.isActive === false || product.is_active === false) {
            throw {
              status: 400,
              error: 'Bad Request',
              message: `Product ${item.productId} is inactive`
            };
          }

          const pricePaise = product.sellingPricePaise || product.pricePaise || Math.round((product.price || 0) * 100);
          const productName = product.name || `Product ${item.productId}`;
          const qty = item.quantity;
          const lineTotal = pricePaise * qty;
          subtotalPaise += lineTotal;

          orderItems.push({
            productId: item.productId,
            name: productName,
            quantity: qty,
            unitPricePaise: pricePaise,
            totalPricePaise: lineTotal
          });

          // 5. Check & decrement inventory stock atomically: verify modifiedCount === 1
          const invResult = await this.db.collection('inventory').updateOne(
            {
              $or: [{ productId: pId as any }, { productId: item.productId as any }],
              stock: { $gte: qty }
            },
            { $inc: { stock: -qty }, $set: { updatedAt: new Date() } },
            { session }
          );

          if (invResult.matchedCount === 0 || invResult.modifiedCount !== 1) {
            throw {
              status: 409,
              error: 'Insufficient stock',
              message: `Insufficient stock for product ${item.productId}. Required: ${qty}`
            };
          }

          // Also update product stock field if maintained directly on product
          await this.db.collection('products').updateOne(
            { _id: product._id, stock: { $gte: qty } },
            { $inc: { stock: -qty } },
            { session }
          );
        }

        // Read delivery configuration from database/config
        const deliveryConfig = await this.getDeliveryConfig(session);
        const deliveryFeePaise = (subtotalPaise === 0 || subtotalPaise >= deliveryConfig.freeDeliveryThresholdPaise)
          ? 0
          : deliveryConfig.defaultDeliveryFeePaise;
        const totalPaise = subtotalPaise + deliveryFeePaise;

        const orderId = 'order_' + crypto.randomUUID().replace(/-/g, '');
        const deliveryOtp = crypto.randomInt(1000, 9999).toString();

        const orderDoc = {
          _id: orderId,
          orderNumber: 'QC-' + Date.now().toString().slice(-6),
          customerId,
          shopId,
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
          idempotencyKey,
          idempotency_key: idempotencyKey,
          deliveryOtp,
          created_at: new Date().toISOString(),
          createdAt: new Date(),
          updatedAt: new Date()
        };

        await this.db.collection('orders').insertOne(orderDoc as any, { session });

        // Record idempotency record with requestHash and customerId
        await this.db.collection('idempotency_keys').insertOne({
          key: idempotencyKey,
          customerId,
          orderId,
          requestHash,
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
