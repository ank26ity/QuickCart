import { ObjectId } from 'mongodb';
import { DatabaseManager } from '../connection';
import { runMigration001 } from '../migrations/001_create_collections_and_schemas';

export async function seedDatabase(): Promise<void> {
  const db = await DatabaseManager.getInstance().connect();
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

  const users = [
    {
      _id: customerId,
      name: 'Rohan Sharma',
      email: 'customer@quickcart.com',
      phone: '+919876543210',
      passwordHash: '$2b$10$wKqK.r57aH8iG2M8.1Y8U.F5b6c7d8e9f0a1b2c3d4e5f6g7h8', // "Password@123"
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
      passwordHash: '$2b$10$wKqK.r57aH8iG2M8.1Y8U.F5b6c7d8e9f0a1b2c3d4e5f6g7h8',
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
      passwordHash: '$2b$10$wKqK.r57aH8iG2M8.1Y8U.F5b6c7d8e9f0a1b2c3d4e5f6g7h8',
      role: 'delivery',
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
      passwordHash: '$2b$10$wKqK.r57aH8iG2M8.1Y8U.F5b6c7d8e9f0a1b2c3d4e5f6g7h8',
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

  // 3. SEED SHOPS (GeoJSON coordinates [lng, lat] within 3km of Connaught Place 77.2090, 28.6139)
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
      rating: 4.8,
      totalRatings: 342,
      isOpen: true,
      serviceRadiusKm: 3.0,
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
      rating: 4.6,
      totalRatings: 180,
      isOpen: true,
      serviceRadiusKm: 3.0,
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
      _id: prod2Id,
      shopId: shopId,
      categoryId: catDairyId,
      name: 'Artisan Sourdough Bread 400g',
      description: 'Slow-fermented artisan crusty sourdough loaf',
      image: 'https://images.unsplash.com/photo-1509440159596-0249088772ff?auto=format&fit=crop&q=80&w=400',
      mrpPaise: 12000, // ₹120.00
      sellingPricePaise: 11000, // ₹110.00
      gstRatePercent: 0,
      isVeg: true,
      unit: 'pack',
      variants: [],
      modifiers: [],
      isActive: true,
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
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
