import { Db } from 'mongodb';
import { DatabaseManager } from '../connection';

export async function runMigration001(db: Db): Promise<void> {
  console.log('[Migration 001] Starting collection creation, $jsonSchema validation, and indexing...');

  // 1. USERS COLLECTION
  const existingCollections = (await db.listCollections().toArray()).map(c => c.name);

  if (!existingCollections.includes('users')) {
    await db.createCollection('users', {
      validator: {
        $jsonSchema: {
          bsonType: 'object',
          required: ['name', 'email', 'phone', 'role', 'isActive', 'version', 'createdAt', 'updatedAt'],
          properties: {
            name: { bsonType: 'string', minLength: 2, maxLength: 100 },
            email: { bsonType: 'string', pattern: '^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$' },
            phone: { bsonType: 'string', pattern: '^\\+?[1-9]\\d{9,14}$' },
            passwordHash: { bsonType: 'string' },
            role: { enum: ['customer', 'shopkeeper', 'delivery', 'admin'] },
            isActive: { bsonType: 'bool' },
            shopId: { bsonType: ['objectId', 'null'] },
            version: { bsonType: 'int', minimum: 1 },
            createdAt: { bsonType: 'date' },
            updatedAt: { bsonType: 'date' },
            deletedAt: { bsonType: ['date', 'null'] }
          }
        }
      },
      validationLevel: 'moderate',
      validationAction: 'error'
    });
  }
  await db.collection('users').createIndex({ email: 1 }, { unique: true });
  await db.collection('users').createIndex({ phone: 1 }, { unique: true });
  await db.collection('users').createIndex({ role: 1, isActive: 1 });

  // 2. SHOPS COLLECTION
  if (!existingCollections.includes('shops')) {
    await db.createCollection('shops', {
      validator: {
        $jsonSchema: {
          bsonType: 'object',
          required: ['name', 'ownerId', 'category', 'location', 'isOpen', 'version', 'createdAt', 'updatedAt'],
          properties: {
            name: { bsonType: 'string', minLength: 2 },
            ownerId: { bsonType: 'objectId' },
            category: { bsonType: 'string' },
            description: { bsonType: 'string' },
            image: { bsonType: 'string' },
            address: { bsonType: 'string' },
            location: {
              bsonType: 'object',
              required: ['type', 'coordinates'],
              properties: {
                type: { enum: ['Point'] },
                coordinates: {
                  bsonType: 'array',
                  minItems: 2,
                  maxItems: 2,
                  items: { bsonType: 'double' }
                }
              }
            },
            rating: { bsonType: 'double', minimum: 0.0, maximum: 5.0 },
            totalRatings: { bsonType: 'int', minimum: 0 },
            isOpen: { bsonType: 'bool' },
            serviceRadiusKm: { bsonType: 'double', minimum: 0.5, maximum: 25.0 },
            version: { bsonType: 'int', minimum: 1 },
            createdAt: { bsonType: 'date' },
            updatedAt: { bsonType: 'date' }
          }
        }
      }
    });
  }
  // 2dsphere index for 3km radius spatial queries
  await db.collection('shops').createIndex({ location: '2dsphere' });
  await db.collection('shops').createIndex({ category: 1, isOpen: 1, rating: -1 });
  await db.collection('shops').createIndex({ ownerId: 1 });

  // 3. CATEGORIES COLLECTION
  if (!existingCollections.includes('categories')) {
    await db.createCollection('categories', {
      validator: {
        $jsonSchema: {
          bsonType: 'object',
          required: ['name', 'slug', 'path', 'displayOrder', 'isActive', 'version'],
          properties: {
            name: { bsonType: 'string' },
            slug: { bsonType: 'string' },
            path: { bsonType: 'string' }, // Materialized path
            parentId: { bsonType: ['objectId', 'null'] },
            displayOrder: { bsonType: 'int' },
            isActive: { bsonType: 'bool' },
            version: { bsonType: 'int', minimum: 1 }
          }
        }
      }
    });
  }
  await db.collection('categories').createIndex({ slug: 1 }, { unique: true });
  await db.collection('categories').createIndex({ path: 1 });
  await db.collection('categories').createIndex({ parentId: 1, displayOrder: 1 });

  // 4. PRODUCTS COLLECTION
  if (!existingCollections.includes('products')) {
    await db.createCollection('products', {
      validator: {
        $jsonSchema: {
          bsonType: 'object',
          required: ['shopId', 'categoryId', 'name', 'mrpPaise', 'sellingPricePaise', 'gstRatePercent', 'isVeg', 'isActive', 'version'],
          properties: {
            shopId: { bsonType: 'objectId' },
            categoryId: { bsonType: 'objectId' },
            name: { bsonType: 'string', minLength: 2 },
            description: { bsonType: 'string' },
            image: { bsonType: 'string' },
            mrpPaise: { bsonType: 'int', minimum: 0 },
            sellingPricePaise: { bsonType: 'int', minimum: 0 },
            gstRatePercent: { enum: [0, 5, 12, 18, 28] },
            isVeg: { bsonType: 'bool' },
            unit: { bsonType: 'string' },
            isActive: { bsonType: 'bool' },
            version: { bsonType: 'int', minimum: 1 }
          }
        }
      }
    });
  }
  // Compound ESR index for fast catalog retrieval per shop
  await db.collection('products').createIndex({ shopId: 1, categoryId: 1, isActive: 1 });
  await db.collection('products').createIndex({ name: 'text', description: 'text' });

  // 5. INVENTORY COLLECTION (Per-Shop Stock)
  if (!existingCollections.includes('inventory')) {
    await db.createCollection('inventory', {
      validator: {
        $jsonSchema: {
          bsonType: 'object',
          required: ['shopId', 'productId', 'stock', 'reservedStock', 'version'],
          properties: {
            shopId: { bsonType: 'objectId' },
            productId: { bsonType: 'objectId' },
            stock: { bsonType: 'int', minimum: 0 },
            reservedStock: { bsonType: 'int', minimum: 0 },
            lowStockThreshold: { bsonType: 'int', minimum: 0 },
            version: { bsonType: 'int', minimum: 1 }
          }
        }
      }
    });
  }
  await db.collection('inventory').createIndex({ shopId: 1, productId: 1 }, { unique: true });
  await db.collection('inventory').createIndex({ shopId: 1, stock: 1 });

  // 6. ORDERS COLLECTION
  if (!existingCollections.includes('orders')) {
    await db.createCollection('orders', {
      validator: {
        $jsonSchema: {
          bsonType: 'object',
          required: ['orderNumber', 'customerId', 'shopId', 'status', 'items', 'deliveryAddress', 'pricing', 'paymentStatus', 'idempotencyKey', 'version'],
          properties: {
            orderNumber: { bsonType: 'string' },
            customerId: { bsonType: 'objectId' },
            shopId: { bsonType: 'objectId' },
            courierId: { bsonType: ['objectId', 'null'] },
            status: { enum: ['pending', 'preparing', 'ready', 'out_for_delivery', 'delivered', 'cancelled', 'rejected'] },
            items: {
              bsonType: 'array',
              minItems: 1,
              items: {
                bsonType: 'object',
                required: ['productId', 'name', 'quantity', 'unitPricePaise', 'totalPricePaise'],
                properties: {
                  productId: { bsonType: 'objectId' },
                  name: { bsonType: 'string' },
                  quantity: { bsonType: 'int', minimum: 1 },
                  unitPricePaise: { bsonType: 'int', minimum: 0 },
                  mrpPaise: { bsonType: 'int', minimum: 0 },
                  gstAmountPaise: { bsonType: 'int', minimum: 0 },
                  totalPricePaise: { bsonType: 'int', minimum: 0 }
                }
              }
            },
            pricing: {
              bsonType: 'object',
              required: ['itemSubtotalPaise', 'deliveryFeePaise', 'finalTotalPaise'],
              properties: {
                itemSubtotalPaise: { bsonType: 'int', minimum: 0 },
                deliveryFeePaise: { bsonType: 'int', minimum: 0 },
                surgeFeePaise: { bsonType: 'int', minimum: 0 },
                discountPaise: { bsonType: 'int', minimum: 0 },
                totalGstPaise: { bsonType: 'int', minimum: 0 },
                finalTotalPaise: { bsonType: 'int', minimum: 0 }
              }
            },
            paymentStatus: { enum: ['pending', 'authorized', 'captured', 'failed', 'refunded'] },
            idempotencyKey: { bsonType: 'string' },
            version: { bsonType: 'int', minimum: 1 }
          }
        }
      }
    });
  }
  await db.collection('orders').createIndex({ orderNumber: 1 }, { unique: true });
  await db.collection('orders').createIndex({ customerId: 1, createdAt: -1 });
  await db.collection('orders').createIndex({ shopId: 1, status: 1, createdAt: -1 });
  await db.collection('orders').createIndex({ courierId: 1, status: 1 });
  await db.collection('orders').createIndex({ idempotencyKey: 1 }, { unique: true });

  // 7. COURIERS COLLECTION
  if (!existingCollections.includes('couriers')) {
    await db.createCollection('couriers', {
      validator: {
        $jsonSchema: {
          bsonType: 'object',
          required: ['userId', 'vehicleType', 'licenseNumber', 'rcNumber', 'isOnline', 'isAssigned', 'version'],
          properties: {
            userId: { bsonType: 'objectId' },
            vehicleType: { enum: ['bicycle', 'scooter', 'motorcycle', 'auto', 'car'] },
            licenseNumber: { bsonType: 'string' },
            rcNumber: { bsonType: 'string' },
            isOnline: { bsonType: 'bool' },
            isAssigned: { bsonType: 'bool' },
            currentOrderId: { bsonType: ['objectId', 'null'] },
            totalEarningsPaise: { bsonType: 'int', minimum: 0 },
            totalTripsCompleted: { bsonType: 'int', minimum: 0 },
            version: { bsonType: 'int', minimum: 1 }
          }
        }
      }
    });
  }
  await db.collection('couriers').createIndex({ userId: 1 }, { unique: true });
  await db.collection('couriers').createIndex({ isOnline: 1, isAssigned: 1 });
  await db.collection('couriers').createIndex({ currentLocation: '2dsphere' });

  // 8. IDEMPOTENCY KEYS COLLECTION with TTL
  if (!existingCollections.includes('idempotency_keys')) {
    await db.createCollection('idempotency_keys');
  }
  await db.collection('idempotency_keys').createIndex({ key: 1 }, { unique: true });
  await db.collection('idempotency_keys').createIndex({ expiresAt: 1 }, { expireAfterSeconds: 0 });

  // 9. OTP REQUESTS with TTL
  if (!existingCollections.includes('otp_requests')) {
    await db.createCollection('otp_requests');
  }
  await db.collection('otp_requests').createIndex({ phone: 1, createdAt: -1 });
  await db.collection('otp_requests').createIndex({ expiresAt: 1 }, { expireAfterSeconds: 0 });

  // 10. SESSIONS / REFRESH TOKENS with TTL
  if (!existingCollections.includes('sessions')) {
    await db.createCollection('sessions');
  }
  await db.collection('sessions').createIndex({ userId: 1 });
  await db.collection('sessions').createIndex({ refreshTokenHash: 1 }, { unique: true });
  await db.collection('sessions').createIndex({ expiresAt: 1 }, { expireAfterSeconds: 0 });

  console.log('[Migration 001] Completed successfully!');
}

// Allow direct CLI execution: ts-node src/db/migrations/001_create_collections_and_schemas.ts
if (require.main === module) {
  (async () => {
    try {
      const db = await DatabaseManager.getInstance().connect();
      await runMigration001(db);
      await DatabaseManager.getInstance().disconnect();
      process.exit(0);
    } catch (err) {
      console.error('[Migration Error]:', err);
      process.exit(1);
    }
  })();
}
