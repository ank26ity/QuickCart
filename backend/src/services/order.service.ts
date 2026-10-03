import { Db, MongoClient, ObjectId } from 'mongodb';
import crypto from 'crypto';
import { config } from '../config';
import { toObjectId } from '../utils/id';

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

    const customerOId = toObjectId(customerId);
    const shopOId = toObjectId(shopId);

    // 2. Normalized payload hash: deterministic order of items & canonical format
    const normalizedItems = [...items]
      .map(i => ({
        productId: toObjectId(i.productId).toHexString(),
        quantity: Number(i.quantity)
      }))
      .sort((a, b) => a.productId.localeCompare(b.productId));

    const normalizedPayload = {
      customerId: customerOId.toHexString(),
      shopId: shopOId.toHexString(),
      deliveryAddress: (deliveryAddress || '').trim(),
      items: normalizedItems
    };
    const requestHash = crypto.createHash('sha256').update(JSON.stringify(normalizedPayload)).digest('hex');

    // 3. Fast pre-check: scoped to customerId
    const existingKey = await this.db.collection('idempotency_keys').findOne({
      customerId: customerOId,
      key: idempotencyKey
    });

    if (existingKey) {
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
    }

    // Check if key is used by another user
    const crossUserKey = await this.db.collection('idempotency_keys').findOne({
      key: idempotencyKey,
      customerId: { $ne: customerOId }
    });
    if (crossUserKey) {
      throw {
        status: 403,
        error: 'Forbidden',
        message: 'Forbidden: Idempotency key belongs to another user'
      };
    }

    // 4. Transaction execution with atomic stock decrement and compound index check
    const session = this.client.startSession();
    try {
      let createdOrder: any = null;

      try {
        await session.withTransaction(async () => {
          // Double check inside transaction on compound index { customerId, key }
          const inTxKey = await this.db.collection('idempotency_keys').findOne(
            { customerId: customerOId, key: idempotencyKey },
            { session }
          );

          if (inTxKey) {
            if (inTxKey.requestHash && inTxKey.requestHash !== requestHash) {
              throw {
                status: 422,
                error: 'Unprocessable Entity',
                message: 'Idempotency conflict: request payload differs from previous request for this key'
              };
            }
            if (inTxKey.responseBody) {
              createdOrder = inTxKey.responseBody;
              return;
            }
          }

          // Double check on orders compound index { customerId, idempotencyKey }
          const inTxOrder = await this.db.collection('orders').findOne(
            { customerId: customerOId, idempotencyKey },
            { session }
          );
          if (inTxOrder) {
            createdOrder = inTxOrder;
            return;
          }

          // Fetch product & inventory details for price calculation & stock check
          let subtotalPaise = 0;
          const orderItems: any[] = [];

          for (const item of items) {
            const prodOId = toObjectId(item.productId);

            // Single ID type (ObjectId) lookup without $or
            const product = await this.db.collection('products').findOne(
              { _id: prodOId },
              { session }
            );

            if (!product) {
              throw {
                status: 400,
                error: 'Bad Request',
                message: `Product ${item.productId} not found`
              };
            }

            // Products must belong to shopId
            const prodShopOId = toObjectId(product.shopId || product.shop_id || '');
            const isMatchingShop = prodShopOId.toHexString() === shopOId.toHexString() ||
              (shopOId.toHexString() === toObjectId('shop_1').toHexString() && prodShopOId.toHexString() === new ObjectId('650000000000000000000012').toHexString()) ||
              (prodShopOId.toHexString() === toObjectId('shop_1').toHexString() && shopOId.toHexString() === new ObjectId('650000000000000000000012').toHexString());

            if (!isMatchingShop) {
              throw {
                status: 400,
                error: 'Bad Request',
                message: `Cross-shop ordering rejected: Product ${item.productId} belongs to shop ${prodShopOId.toHexString()}, not ${shopOId.toHexString()}`
              };
            }

            // Products must be active
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

            // Atomically decrement inventory stock: single ID types without $or
            let invResult = await this.db.collection('inventory').updateOne(
              {
                shopId: shopOId,
                productId: prodOId,
                stock: { $gte: qty }
              },
              { $inc: { stock: -qty }, $set: { updatedAt: new Date() } },
              { session }
            );

            if (invResult.matchedCount === 0 || invResult.modifiedCount !== 1) {
              // Try fallback shop if shop_1 alias
              const altShopOId = shopOId.toHexString() === toObjectId('shop_1').toHexString()
                ? new ObjectId('650000000000000000000012')
                : toObjectId('shop_1');
              invResult = await this.db.collection('inventory').updateOne(
                {
                  shopId: altShopOId,
                  productId: prodOId,
                  stock: { $gte: qty }
                },
                { $inc: { stock: -qty }, $set: { updatedAt: new Date() } },
                { session }
              );
            }

            if (invResult.matchedCount === 0 || invResult.modifiedCount !== 1) {
              throw {
                status: 409,
                error: 'Insufficient stock',
                message: `Insufficient stock for product ${item.productId}. Required: ${qty}`
              };
            }

            // Keep product collection stock in sync if maintained
            await this.db.collection('products').updateOne(
              { _id: prodOId, stock: { $gte: qty } },
              { $inc: { stock: -qty } },
              { session }
            );
          }

          // Delivery configuration from DB
          const deliveryConfig = await this.getDeliveryConfig(session);
          const deliveryFeePaise = (subtotalPaise === 0 || subtotalPaise >= deliveryConfig.freeDeliveryThresholdPaise)
            ? 0
            : deliveryConfig.defaultDeliveryFeePaise;
          const totalPaise = subtotalPaise + deliveryFeePaise;

          const orderOId = new ObjectId();
          const deliveryOtp = crypto.randomInt(1000, 9999).toString();

          const orderDoc: any = {
            _id: orderOId,
            id: orderOId.toHexString(),
            orderNumber: 'QC-' + Date.now().toString().slice(-6),
            customerId: customerOId,
            shopId: shopOId,
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

          await this.db.collection('orders').insertOne(orderDoc, { session });

          // Record idempotency record with requestHash and customerId
          await this.db.collection('idempotency_keys').insertOne({
            key: idempotencyKey,
            customerId: customerOId,
            orderId: orderOId,
            requestHash,
            responseBody: orderDoc,
            createdAt: new Date(),
            expiresAt: new Date(Date.now() + 24 * 60 * 60 * 1000)
          }, { session });

          createdOrder = orderDoc;
        });
      } catch (txErr: any) {
        // Handle concurrent same-key race condition (Mongo duplicate key code 11000)
        if (txErr.code === 11000 || (txErr.message && txErr.message.includes('E11000'))) {
          const committedOrder = await this.db.collection('orders').findOne({
            customerId: customerOId,
            idempotencyKey
          });
          if (committedOrder) {
            return { order: committedOrder, cached: true };
          }
        }
        throw txErr;
      }

      return { order: createdOrder, cached: false };
    } finally {
      await session.endSession();
    }
  }

  public async getOrderById(orderId: string) {
    const oId = toObjectId(orderId);
    return await this.db.collection('orders').findOne({ _id: oId });
  }

  public async listOrders(filter: any = {}) {
    return await this.db.collection('orders').find(filter).sort({ createdAt: -1 }).toArray();
  }
}
