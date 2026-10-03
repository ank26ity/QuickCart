import { Db, ObjectId } from 'mongodb';
import crypto from 'crypto';
import { config } from '../config';

export class CourierService {
  constructor(private db: Db) {}

  public async claimOrder(orderId: string, courierId: string): Promise<any> {
    const oId = ObjectId.isValid(orderId) ? new ObjectId(orderId) : orderId;

    // Check if order exists
    const order = await this.db.collection('orders').findOne({
      $or: [{ _id: oId as any }, { _id: orderId as any }]
    });

    if (!order) {
      throw { status: 404, message: 'Order not found' };
    }

    // Atomic claim: only succeeds if delivery_boy_id is null/unset OR already claimed by the exact same courier
    const result = await this.db.collection('orders').findOneAndUpdate(
      {
        $and: [
          { $or: [{ _id: oId as any }, { _id: orderId as any }] },
          {
            $or: [
              { delivery_boy_id: null },
              { delivery_boy_id: { $exists: false } },
              { delivery_boy_id: courierId }
            ]
          }
        ]
      },
      {
        $set: {
          delivery_boy_id: courierId,
          courierId: courierId,
          status: 'assigned',
          updatedAt: new Date()
        }
      },
      { returnDocument: 'after' }
    );

    if (!result) {
      // Order already claimed by another courier!
      throw {
        status: 409,
        error: 'Order already claimed by another courier',
        message: 'Conflict: This order has already been accepted by another courier partner.'
      };
    }

    // Append to audit log
    await this.db.collection('audit_logs').insertOne({
      action: 'courier_order_assigned',
      orderId,
      courierId,
      timestamp: new Date().toISOString()
    });

    return result;
  }

  public async verifyDeliveryOtp(orderId: string, submittedOtp: string): Promise<any> {
    const oId = ObjectId.isValid(orderId) ? new ObjectId(orderId) : orderId;

    const order = await this.db.collection('orders').findOne({
      $or: [{ _id: oId as any }, { _id: orderId as any }]
    });

    if (!order) {
      throw { status: 404, message: 'Order not found' };
    }

    const expectedOtp = order.deliveryOtp || '1234';

    if (submittedOtp.trim() !== expectedOtp.trim()) {
      throw {
        status: 400,
        error: 'Invalid delivery verification code',
        message: 'The OTP provided does not match the delivery verification code.'
      };
    }

    const updated = await this.db.collection('orders').findOneAndUpdate(
      { $or: [{ _id: oId as any }, { _id: orderId as any }] },
      { $set: { status: 'delivered', updatedAt: new Date() } },
      { returnDocument: 'after' }
    );

    // Audit log
    await this.db.collection('audit_logs').insertOne({
      action: 'order_delivered',
      orderId,
      timestamp: new Date().toISOString()
    });

    return updated;
  }

  public generateUploadUrl(docType: string, mimeType: string, fileSize: number) {
    const allowedMime = ['image/jpeg', 'image/png', 'application/pdf'];
    if (!allowedMime.includes(mimeType)) {
      throw { status: 400, message: `Invalid MIME type ${mimeType}. Allowed: JPEG, PNG, PDF.` };
    }

    if (fileSize > 5 * 1024 * 1024) {
      throw { status: 400, message: 'File size exceeds maximum permitted limit of 5MB.' };
    }

    const fileKey = `courier_docs/${docType}_${crypto.randomUUID()}.${mimeType.split('/')[1]}`;
    const signature = crypto.createHmac('sha256', config.jwtSecret)
      .update(`${fileKey}:${Date.now() + 900000}`)
      .digest('hex');

    const uploadUrl = `https://storage.quickcart.in/upload/${fileKey}?expires=${Date.now() + 900000}&sig=${signature}`;

    return { uploadUrl, fileKey };
  }
}
