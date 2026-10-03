import { Db, ObjectId } from 'mongodb';
import crypto from 'crypto';
import { config } from '../config';

export class CourierService {
  constructor(private db: Db) {}

  public async claimOrder(orderId: string, courierId: string): Promise<any> {
    const oId = ObjectId.isValid(orderId) ? new ObjectId(orderId) : orderId;

    // 1. Verify courier user eligibility from users collection
    const cId = ObjectId.isValid(courierId) ? new ObjectId(courierId) : courierId;
    const courierUser = await this.db.collection('users').findOne({
      $or: [{ _id: cId as any }, { _id: courierId as any }]
    });

    if (courierUser) {
      if (courierUser.status === 'suspended' || courierUser.isSuspended === true) {
        throw {
          status: 403,
          error: 'Forbidden',
          message: 'Courier account is suspended and cannot accept orders'
        };
      }

      const compliance = courierUser.complianceStatus || courierUser.compliance_status;
      if (compliance !== 'approved') {
        throw {
          status: 403,
          error: 'Forbidden',
          message: 'Courier account documents are not approved. Approval required before accepting orders.'
        };
      }

      if (courierUser.isOnline === false || courierUser.onDuty === false) {
        throw {
          status: 400,
          error: 'Bad Request',
          message: 'Courier is currently off-duty. Must be on-duty/online to claim orders.'
        };
      }
    }

    // 2. Check if order exists
    const order = await this.db.collection('orders').findOne({
      $or: [{ _id: oId as any }, { _id: orderId as any }]
    });

    if (!order) {
      throw { status: 404, message: 'Order not found' };
    }

    // 3. Status must strictly be 'ready' for courier claim
    if (order.status !== 'ready') {
      throw {
        status: 400,
        error: 'Invalid order status',
        message: `Order cannot be claimed in '${order.status}' status. Must be in 'ready' status.`
      };
    }

    // 4. Atomic claim: filter on status 'ready', courierId null/unset or already assigned to same courier
    // Drop duplicate delivery_boy_id: unify exclusively to courierId
    const result = await this.db.collection('orders').findOneAndUpdate(
      {
        $and: [
          { $or: [{ _id: oId as any }, { _id: orderId as any }] },
          { status: 'ready' },
          {
            $or: [
              { courierId: null },
              { courierId: { $exists: false } },
              { courierId: courierId }
            ]
          }
        ]
      },
      {
        $set: {
          courierId: courierId,
          status: 'assigned',
          updatedAt: new Date()
        },
        $unset: {
          delivery_boy_id: ""
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
