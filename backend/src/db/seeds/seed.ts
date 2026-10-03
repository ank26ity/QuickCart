import { ObjectId } from 'mongodb';
import { DatabaseManager } from '../connection';
import { runMigration001 } from '../migrations/001_create_collections_and_schemas';
import { toObjectId } from '../../utils/id';

export async function seedDatabase(customDb?: any): Promise<void> {
  const db = customDb || await DatabaseManager.getInstance().connect();
  console.log('[Seed] Starting database seed...');

  // Ensure schemas & indexes are applied first
  await runMigration001(db);

  const now = new Date();

  // 1. SEED USERS
  const customerId = new ObjectId('650000000000000000000001');
  const merchantId = new ObjectId('650000000000000000000002');
  const courierId = new ObjectId('650000000000000000000003');
  const adminId = new ObjectId('650000000000000000000004');
  const shopId = new ObjectId('650000000000000000000010');

  const argon2Hash = '$argon2id$v=19$m=65536,p=4,t=3$fuIvNtyj9ucr7p/gZZvEMw$P8obqjv0gixqcergX2Tg/Uf3NuAjMU3vWVrmgNOB4Tg'; // "Password@123"

  const users = [
    {
      _id: customerId,
      name: 'Rohan Sharma',
      email: 'customer@quickcart.com',
      phone: '+919876543210',
      passwordHash: argon2Hash,
      role: 'customer',
      isActive: true,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    },
    {
      _id: merchantId,
      name: 'Priya Patel',
      email: 'merchant@quickcart.com',
      phone: '+919876543211',
      passwordHash: argon2Hash,
      role: 'shopkeeper',
      shopId: shopId,
      isActive: true,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    },
    {
      _id: courierId,
      name: 'Amit Kumar',
      email: 'courier@quickcart.com',
      phone: '+919876543212',
      passwordHash: argon2Hash,
      role: 'delivery',
      complianceStatus: 'approved',
      onDuty: true,
      isOnline: true,
      status: 'active',
      isActive: true,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    },
    {
      _id: new ObjectId('650000000000000000000005'),
      name: 'Courier User 1',
      email: 'user_courier_1@quickcart.com',
      phone: '+919876543214',
      passwordHash: argon2Hash,
      role: 'delivery',
      complianceStatus: 'approved',
      onDuty: true,
      isOnline: true,
      status: 'active',
      isActive: true,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    },
    {
      _id: toObjectId('user_courier_1'),
      name: 'Courier User 1 (Alias)',
      email: 'courier_1@quickcart.com',
      phone: '+919876543219',
      passwordHash: argon2Hash,
      role: 'delivery',
      complianceStatus: 'approved',
      onDuty: true,
      isOnline: true,
      status: 'active',
      isActive: true,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    },
    {
      _id: adminId,
      name: 'System Admin',
      email: 'admin@quickcart.com',
      phone: '+919876543213',
      passwordHash: argon2Hash,
      role: 'admin',
      isActive: true,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    }
  ];

  for (const u of users) {
    await db.collection('users').updateOne({ _id: u._id }, { $set: u }, { upsert: true });
  }

  // 2. SEED CATEGORIES (Materialized Path Pattern)
  const catGroceryId = new ObjectId('650000000000000000000020');
  const catDairyId = new ObjectId('650000000000000000000021');

  const categories = [
    {
      _id: catGroceryId,
      name: 'Grocery & Staples',
      slug: 'grocery-staples',
      path: ',grocery,',
      parentId: null,
      displayOrder: 1,
      isActive: true,
      version: 1,
      createdAt: now,
      updatedAt: now
    },
    {
      _id: catDairyId,
      name: 'Dairy & Breakfast',
      slug: 'dairy-breakfast',
      path: ',grocery,dairy,',
      parentId: catGroceryId,
      displayOrder: 2,
      isActive: true,
      version: 1,
      createdAt: now,
      updatedAt: now
    }
  ];

  for (const c of categories) {
    await db.collection('categories').updateOne({ _id: c._id }, { $set: c }, { upsert: true });
  }

  // 3. SEED SHOPS (GeoJSON coordinates [lng, lat])
  const shops = [
    {
      _id: shopId,
      name: 'Fresh Mart Supermarket',
      ownerId: merchantId,
      category: 'Grocery',
      description: 'Daily fresh farm vegetables, dairy, and household essentials',
      image: 'https://images.unsplash.com/photo-1542838132-92c53300491e?auto=format&fit=crop&q=80&w=600',
      address: 'Block A, Connaught Place, New Delhi',
      location: {
        type: 'Point',
        coordinates: [77.2185, 28.6315] // ~1.8 km from center
      },
      lat: 28.6315,
      lng: 77.2185,
      rating: 4.8,
      totalRatings: 342,
      isOpen: true,
      serviceRadiusKm: 5.0,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    },
    {
      _id: new ObjectId('650000000000000000000011'),
      name: 'Daily Needs Express',
      ownerId: merchantId,
      category: 'Grocery',
      description: 'Instant 10-minute grocery delivery from your neighbourhood dark store',
      image: 'https://images.unsplash.com/photo-1578916171728-46686eac8d58?auto=format&fit=crop&q=80&w=600',
      address: 'Barakhamba Road, New Delhi',
      location: {
        type: 'Point',
        coordinates: [77.2250, 28.6280] // ~1.5 km
      },
      lat: 28.6280,
      lng: 77.2250,
      rating: 4.6,
      totalRatings: 180,
      isOpen: true,
      serviceRadiusKm: 5.0,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    },
    {
      _id: new ObjectId('650000000000000000000012'),
      name: 'Fresh Mart Daily Bangalore',
      ownerId: merchantId,
      category: 'Grocery',
      description: 'Daily fresh farm vegetables, dairy, and household essentials in Bangalore',
      image: 'https://images.unsplash.com/photo-1542838132-92c53300491e?auto=format&fit=crop&q=80&w=600',
      address: '100 Feet Rd, Indiranagar, Bangalore',
      location: {
        type: 'Point',
        coordinates: [77.5946, 12.9716]
      },
      lat: 12.9716,
      lng: 77.5946,
      rating: 4.8,
      totalRatings: 342,
      isOpen: true,
      serviceRadiusKm: 10.0,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    },
    {
      _id: toObjectId('shop_1'),
      name: 'Fresh Mart Daily Bangalore (Shop 1)',
      ownerId: merchantId,
      category: 'Grocery',
      description: 'Daily fresh farm vegetables, dairy, and household essentials in Bangalore',
      image: 'https://images.unsplash.com/photo-1542838132-92c53300491e?auto=format&fit=crop&q=80&w=600',
      address: '100 Feet Rd, Indiranagar, Bangalore',
      location: {
        type: 'Point',
        coordinates: [77.5946, 12.9716]
      },
      lat: 12.9716,
      lng: 77.5946,
      rating: 4.8,
      totalRatings: 342,
      isOpen: true,
      serviceRadiusKm: 10.0,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    }
  ];

  for (const s of shops) {
    await db.collection('shops').updateOne({ _id: s._id }, { $set: s }, { upsert: true });
  }

  // 4. SEED PRODUCTS & INVENTORY
  const prod1Id = new ObjectId('650000000000000000000030');
  const prod2Id = new ObjectId('650000000000000000000031');

  const products = [
    {
      _id: prod1Id,
      shopId: shopId,
      categoryId: catDairyId,
      name: 'Organic Full Cream Milk 1L',
      description: 'Farm-fresh pasteurized organic milk rich in calcium and vitamins',
      image: 'https://images.unsplash.com/photo-1550583724-b2692b85b150?auto=format&fit=crop&q=80&w=400',
      mrpPaise: 7500, // ₹75.00
      sellingPricePaise: 6800, // ₹68.00
      gstRatePercent: 5,
      isVeg: true,
      unit: 'bottle',
      variants: [],
      modifiers: [],
      isActive: true,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    },
    {
      _id: toObjectId('prod_1'),
      shopId: new ObjectId('650000000000000000000012'),
      categoryId: catDairyId,
      name: 'Organic Fresh Milk 1L',
      description: 'Farm-fresh milk in Bangalore',
      mrpPaise: 12000,
      sellingPricePaise: 12000,
      pricePaise: 12000,
      price: 120,
      isActive: true,
      version: 1,
      createdAt: now,
      updatedAt: now
    }
  ];

  for (const p of products) {
    await db.collection('products').updateOne({ _id: p._id }, { $set: p }, { upsert: true });
  }

  // Inventory records
  const inventoryItems = [
    {
      _id: new ObjectId(),
      shopId: shopId,
      productId: prod1Id,
      stock: 45,
      reservedStock: 0,
      lowStockThreshold: 5,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    },
    {
      _id: new ObjectId(),
      shopId: shopId,
      productId: prod2Id,
      stock: 4, // Triggers "Low Stock" badge in UI (<= 5)
      reservedStock: 0,
      lowStockThreshold: 5,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    },
    {
      _id: new ObjectId(),
      shopId: new ObjectId('650000000000000000000012'),
      productId: toObjectId('prod_1'),
      stock: 100,
      reservedStock: 0,
      lowStockThreshold: 5,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    },
    {
      _id: new ObjectId(),
      shopId: toObjectId('shop_1'),
      productId: toObjectId('prod_1'),
      stock: 100,
      reservedStock: 0,
      lowStockThreshold: 5,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    }
  ];

  for (const inv of inventoryItems) {
    await db.collection('inventory').updateOne(
      { shopId: inv.shopId, productId: inv.productId },
      { $set: inv },
      { upsert: true }
    );
  }

  // 5. SEED COURIER PROFILE
  const courierProfile = {
    _id: new ObjectId(),
    userId: courierId,
    vehicleType: 'motorcycle',
    licenseNumber: 'DL-1420200019283',
    rcNumber: 'DL01AB9876',
    isOnline: true,
    isAssigned: false,
    currentOrderId: null,
    currentLocation: {
      type: 'Point',
      coordinates: [77.2150, 28.6300]
    },
    totalEarningsPaise: 45000, // ₹450.00
    totalTripsCompleted: 9,
    rating: 4.9,
    version: 1,
    createdAt: now,
    updatedAt: now,
    deletedAt: null
  };

  await db.collection('couriers').updateOne(
    { userId: courierId },
    { $set: courierProfile },
    { upsert: true }
  );

  // 6. SEED APP CONFIG
  await db.collection('app_config').updateOne(
    { key: 'delivery_config' },
    {
      $set: {
        key: 'delivery_config',
        defaultDeliveryFeePaise: 4900,
        freeDeliveryThresholdPaise: 49900,
        deliveryFeePaise: 4900,
        updatedAt: now
      }
    },
    { upsert: true }
  );

  console.log('[Seed] Database seeded successfully with test users, shops, catalog, and inventory!');
}

if (require.main === module) {
  (async () => {
    try {
      await seedDatabase();
      await DatabaseManager.getInstance().disconnect();
      process.exit(0);
    } catch (err) {
      console.error('[Seed Error]:', err);
      process.exit(1);
    }
  })();
}
