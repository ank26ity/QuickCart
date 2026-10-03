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
          required: ['name', 'category', 'isOpen'],
          properties: {
            name: { bsonType: 'string', minLength: 2 },
            ownerId: { bsonType: ['objectId', 'string'] },
            category: { bsonType: 'string' },
            description: { bsonType: 'string' },
            image: { bsonType: 'string' },
            address: { bsonType: 'string' },
            location: {
              bsonType: 'object',
              properties: {
                type: { enum: ['Point'] },
                coordinates: {
                  bsonType: 'array',
                  minItems: 2,
                  maxItems: 2,
                  items: { bsonType: ['double', 'int'] }
                }
              }
            },
            lat: { bsonType: ['double', 'int'] },
            lng: { bsonType: ['double', 'int'] },
            rating: { bsonType: ['double', 'int'], minimum: 0.0, maximum: 5.0 },
            totalRatings: { bsonType: ['int', 'double'], minimum: 0 },
            isOpen: { bsonType: 'bool' },
            serviceRadiusKm: { bsonType: ['double', 'int'], minimum: 0.5, maximum: 25.0 },
            version: { bsonType: ['int', 'double'] },
            createdAt: { bsonType: 'date' },
            updatedAt: { bsonType: 'date' }
          }
        }
      },
      validationLevel: 'moderate'
    });
  }
  // 2dsphere index for 3km radius spatial queries
  await db.collection('shops').createIndex({ location: '2dsphere' }, { sparse: true });
  await db.collection('shops').createIndex({ category: 1, isOpen: 1, rating: -1 });

  // 3. CATEGORIES COLLECTION
  if (!existingCollections.includes('categories')) {
    await db.createCollection('categories', {
      validator: {
        $jsonSchema: {
          bsonType: 'object',
          required: ['name', 'slug'],
          properties: {
            name: { bsonType: 'string' },
            slug: { bsonType: 'string' },
            path: { bsonType: 'string' }, // Materialized path
            parentId: { bsonType: ['objectId', 'string', 'null'] },
            displayOrder: { bsonType: 'int' },
            isActive: { bsonType: 'bool' },
            version: { bsonType: ['int', 'double'] }
          }
        }
      },
      validationLevel: 'moderate'
    });
  }
  await db.collection('categories').createIndex({ slug: 1 }, { unique: true });
  await db.collection('categories').createIndex({ path: 1 });

  // 4. PRODUCTS COLLECTION
  if (!existingCollections.includes('products')) {
    await db.createCollection('products', {
      validator: {
        $jsonSchema: {
          bsonType: 'object',
          required: ['name'],
          properties: {
            shopId: { bsonType: ['objectId', 'string'] },
            categoryId: { bsonType: ['objectId', 'string'] },
            name: { bsonType: 'string', minLength: 2 },
            description: { bsonType: 'string' },
            image: { bsonType: 'string' },
            mrpPaise: { bsonType: ['int', 'double'] },
            sellingPricePaise: { bsonType: ['int', 'double'] },
            pricePaise: { bsonType: ['int', 'double'] },
            price: { bsonType: ['double', 'int'] },
            stock: { bsonType: ['int', 'double'] },
            gstRatePercent: { bsonType: ['int', 'double'] },
            isVeg: { bsonType: 'bool' },
            unit: { bsonType: 'string' },
            isActive: { bsonType: 'bool' },
            version: { bsonType: ['int', 'double'] }
          }
        }
      },
      validationLevel: 'moderate'
    });
  }
  await db.collection('products').createIndex({ shopId: 1, categoryId: 1, isActive: 1 });

  // 5. INVENTORY COLLECTION (Per-Shop Stock)
  if (!existingCollections.includes('inventory')) {
    await db.createCollection('inventory', {
      validator: {
        $jsonSchema: {
          bsonType: 'object',
          required: ['stock'],
          properties: {
            shopId: { bsonType: ['objectId', 'string'] },
            productId: { bsonType: ['objectId', 'string'] },
            stock: { bsonType: ['int', 'double'] },
            reservedStock: { bsonType: ['int', 'double'] },
            lowStockThreshold: { bsonType: ['int', 'double'] },
            version: { bsonType: ['int', 'double'] }
          }
        }
      },
      validationLevel: 'moderate'
    });
  }
  await db.collection('inventory').createIndex({ shopId: 1, productId: 1 }, { unique: true });

  // 6. ORDERS COLLECTION
  if (!existingCollections.includes('orders')) {
    await db.createCollection('orders', {
      validator: {
        $jsonSchema: {
          bsonType: 'object',
          required: ['status', 'items'],
          properties: {
            orderNumber: { bsonType: 'string' },
            customerId: { bsonType: ['objectId', 'string'] },
            shopId: { bsonType: ['objectId', 'string'] },
            courierId: { bsonType: ['objectId', 'string', 'null'] },
            delivery_boy_id: { bsonType: ['objectId', 'string', 'null'] },
            status: { enum: ['pending', 'accepted', 'preparing', 'ready', 'assigned', 'picked_up', 'delivered', 'cancelled', 'rejected'] },
            items: { bsonType: 'array' },
            deliveryAddress: { bsonType: ['object', 'string'] },
            pricing: { bsonType: 'object' },
            subtotalPaise: { bsonType: ['int', 'double'] },
            deliveryFeePaise: { bsonType: ['int', 'double'] },
            totalPaise: { bsonType: ['int', 'double'] },
            paymentStatus: { bsonType: ['string', 'null'] },
            idempotency_key: { bsonType: 'string' },
            deliveryOtp: { bsonType: 'string' },
            version: { bsonType: ['int', 'double'] }
          }
        }
      },
      validationLevel: 'moderate'
    });
  }
  await db.collection('orders').createIndex({ orderNumber: 1 }, { unique: true });
  await db.collection('orders').createIndex({ shopId: 1, status: 1, createdAt: -1 });
  await db.collection('orders').createIndex({ courierId: 1, status: 1 });
  await db.collection('orders').createIndex({ customerId: 1, createdAt: -1 });
  await db.collection('orders').createIndex({ customerId: 1, idempotencyKey: 1 }, { unique: true });

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
