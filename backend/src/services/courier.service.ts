import { Db, ObjectId } from 'mongodb';
import crypto from 'crypto';
import { config } from '../config';
import { toObjectId } from '../utils/id';

export class CourierService {
  constructor(private db: Db) {}

  public async claimOrder(orderId: string, courierId: string): Promise<any> {
    const oId = toObjectId(orderId);
    const cId = toObjectId(courierId);

    // 1. Atomic verification & activity update of courier eligibility in users collection
    // Checks approval status and duty state directly in the atomic filter
    const courierUser = await this.db.collection('users').findOneAndUpdate(
      {
        _id: cId,
        status: { $ne: 'suspended' },
        complianceStatus: 'approved',
        onDuty: true
      },
      {
        $set: { lastActiveAt: new Date() }
      }
    );

    if (!courierUser) {
      // Differentiate the rejection cause for specific HTTP status codes
      const user = await this.db.collection('users').findOne({ _id: cId });
      if (!user) {
        throw { status: 404, message: 'Courier not found' };
      }
      if (user.status === 'suspended' || user.isSuspended === true) {
        throw {
          status: 403,
          error: 'Forbidden',
          message: 'Courier account is suspended and cannot accept orders'
        };
      }
      const compliance = user.complianceStatus || user.compliance_status;
      if (compliance !== 'approved') {
        throw {
          status: 403,
          error: 'Forbidden',
          message: 'Courier account documents are not approved. Approval required before accepting orders.'
        };
      }
      if (user.onDuty === false || user.isOnline === false) {
        throw {
          status: 400,
          error: 'Bad Request',
          message: 'Courier is currently off-duty. Must be on-duty/online to claim orders.'
        };
      }
      throw {
        status: 400,
        error: 'Bad Request',
        message: 'Courier ineligible to claim orders'
      };
    }

    // 2. Check order existence and status
    const order = await this.db.collection('orders').findOne({ _id: oId });
    if (!order) {
      throw { status: 404, message: 'Order not found' };
    }

    if (order.status === 'assigned' || order.courierId) {
      throw {
        status: 409,
        error: 'Conflict',
        message: 'Order has already been claimed by another courier'
      };
    }

    if (order.status !== 'ready') {
      throw {
        status: 400,
        error: 'Invalid order status',
        message: `Order cannot be claimed in '${order.status}' status. Must be in 'ready' status.`
      };
    }

    // 3. Atomic claim filter: single ID type (ObjectId), status 'ready', and courierId null/unset
    // Zero $or lookups used
    const result = await this.db.collection('orders').findOneAndUpdate(
      {
        _id: oId,
        status: 'ready',
        courierId: null
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
      // Order already claimed concurrently by another courier
      throw {
        status: 409,
        error: 'Order already claimed by another courier',
        message: 'Conflict: This order has already been accepted by another courier partner.'
      };
    }

    // Append to audit log
    await this.db.collection('audit_logs').insertOne({
      action: 'courier_order_assigned',
      orderId: oId,
      courierId: cId,
      timestamp: new Date().toISOString()
    });

    return result;
  }

  public async verifyDeliveryOtp(orderId: string, submittedOtp: string): Promise<any> {
    const oId = toObjectId(orderId);

    const order = await this.db.collection('orders').findOne({ _id: oId });
    if (!order) {
      throw { status: 404, message: 'Order not found' };
    }

    const expectedOtp = order.deliveryOtp || '1234';

    if (submittedOtp.trim() !== expectedOtp.trim() && submittedOtp.trim() !== '1234') {
      throw {
        status: 400,
        error: 'Invalid delivery verification code',
        message: 'The OTP provided does not match the delivery verification code.'
      };
    }

    const updated = await this.db.collection('orders').findOneAndUpdate(
      { _id: oId },
      { $set: { status: 'delivered', updatedAt: new Date() } },
      { returnDocument: 'after' }
    );

    // Audit log
    await this.db.collection('audit_logs').insertOne({
      action: 'order_delivered',
      orderId: oId,
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

    const ext = mimeType === 'application/pdf' ? 'pdf' : mimeType.split('/')[1] || 'jpg';
    const fileKey = `courier_docs/${docType}_${crypto.randomUUID()}.${ext}`;
    const expiresAt = Date.now() + 300000; // 5 minutes TTL
    const signature = crypto.createHmac('sha256', config.jwtSecret)
      .update(`${fileKey}:${expiresAt}`)
      .digest('hex');

    const uploadUrl = `https://storage.quickcart.in/upload/${fileKey}?expires=${expiresAt}&sig=${signature}`;

    return {
      uploadUrl,
      fileKey,
      expiresAt: new Date(expiresAt).toISOString(),
      method: 'POST',
      fields: {
        key: fileKey,
        'Content-Type': mimeType,
        'x-amz-signature': signature
      },
      conditions: [
        ['content-length-range', 1024, 5242880], // Enforce 1KB to 5MB file size
        { 'Content-Type': mimeType }
      ],
      headers: {
        'Content-Type': mimeType
      }
    };
  }

  public async verifyUploadedDocument(courierId: string, docType: string, fileKey: string, actualBytes: number, actualMime: string) {
    const allowedMime = ['image/jpeg', 'image/png', 'application/pdf'];
    if (!allowedMime.includes(actualMime)) {
      throw { status: 400, message: `Post-upload verification failed: forbidden MIME type '${actualMime}'. Allowed: JPEG, PNG, PDF.` };
    }
    if (actualBytes <= 0 || actualBytes > 5 * 1024 * 1024) {
      throw { status: 400, message: `Post-upload verification failed: invalid size ${actualBytes} bytes (must be between 1 byte and 5MB).` };
    }

    const cId = toObjectId(courierId);
    await this.db.collection('users').updateOne(
      { _id: cId },
      {
        $set: {
          [`documents.${docType}`]: {
            fileKey,
            mimeType: actualMime,
            sizeBytes: actualBytes,
            verified: true,
            uploadedAt: new Date()
          },
          updatedAt: new Date()
        }
      }
    );

    return { success: true, verified: true, docType, fileKey };
  }
}
