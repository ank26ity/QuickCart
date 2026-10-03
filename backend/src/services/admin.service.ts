import { Db, ObjectId } from 'mongodb';

export class AdminService {
  constructor(private db: Db) {}

  public async getMetrics() {
    const totalOrders = await this.db.collection('orders').countDocuments();
    const activeShops = await this.db.collection('shops').countDocuments({ isOpen: true });
    const activeUsers = await this.db.collection('users').countDocuments({ isActive: true });

    return {
      totalOrders,
      activeShops,
      activeUsers
    };
  }

  public async registerShop(shopData: { name: string; category?: string; latitude?: number; longitude?: number }) {
    const shopCount = await this.db.collection('shops').countDocuments();
    const shopId = `shop_${shopCount + 1}`;

    const shop = {
      _id: shopId,
      name: shopData.name,
      category: shopData.category || 'grocery',
      description: `QuickCart Partner Store ${shopId}`,
      lat: shopData.latitude || 12.9716,
      lng: shopData.longitude || 77.5946,
      rating: 5.0,
      address: 'Platform Express Hub, Bangalore',
      isOpen: true,
      createdAt: new Date(),
      updatedAt: new Date()
    };

    await this.db.collection('shops').insertOne(shop as any);
    await this.logAction('shop_registered', shopId);

    return shop;
  }

  public async provisionUser(userData: { name: string; email: string; phone?: string; role?: string }) {
    const userCount = await this.db.collection('users').countDocuments();
    const userId = `user_${userCount + 1}`;

    const user = {
      _id: userId,
      name: userData.name,
      email: userData.email,
      phone: userData.phone || '+919999999999',
      role: userData.role || 'customer',
      status: 'active',
      isActive: true,
      createdAt: new Date(),
      updatedAt: new Date()
    };

    await this.db.collection('users').insertOne(user as any);
    await this.logAction('user_provisioned', userId);

    return user;
  }

  public async approveCourier(courierId: string) {
    const cId = ObjectId.isValid(courierId) ? new ObjectId(courierId) : courierId;

    await this.db.collection('users').updateOne(
      { $or: [{ _id: cId as any }, { _id: courierId as any }, { email: courierId }] },
      { $set: { complianceStatus: 'approved', updatedAt: new Date() } }
    );

    await this.logAction('courier_approved', courierId);
    return { success: true, status: 'approved' };
  }

  public async suspendUser(userId: string) {
    const uId = ObjectId.isValid(userId) ? new ObjectId(userId) : userId;

    await this.db.collection('users').updateOne(
      { $or: [{ _id: uId as any }, { _id: userId as any }, { email: userId }] },
      { $set: { status: 'suspended', isActive: false, updatedAt: new Date() } }
    );

    await this.logAction('user_suspended', userId);
    return { success: true, status: 'suspended' };
  }

  public async manualDispatch(orderId: string, courierId: string) {
    const oId = ObjectId.isValid(orderId) ? new ObjectId(orderId) : orderId;

    await this.db.collection('orders').updateOne(
      { $or: [{ _id: oId as any }, { _id: orderId as any }] },
      { $set: { delivery_boy_id: courierId, courierId, status: 'assigned', updatedAt: new Date() } }
    );

    await this.logAction('manual_dispatch', orderId, { courierId });
    return { success: true, status: 'assigned' };
  }

  public async getAuditLogs() {
    return await this.db.collection('audit_logs').find().sort({ timestamp: -1 }).toArray();
  }

  private async logAction(action: string, targetId: string, details?: any) {
    await this.db.collection('audit_logs').insertOne({
      action,
      targetId,
      ...details,
      timestamp: new Date().toISOString()
    });
  }
}
