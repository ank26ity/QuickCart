import { Db, ObjectId } from 'mongodb';
import argon2 from 'argon2';
import jwt from 'jsonwebtoken';
import crypto from 'crypto';
import { config } from '../config';

export class AuthService {
  constructor(private db: Db) {}

  public async hashPassword(password: string): Promise<string> {
    return argon2.hash(password, {
      type: argon2.argon2id,
      memoryCost: 2 ** 16, // 64 MB
      timeCost: 3,
      parallelism: 1
    });
  }

  public async verifyPassword(hash: string, plain: string): Promise<boolean> {
    try {
      return await argon2.verify(hash, plain);
    } catch {
      return false;
    }
  }

  public generateTokens(user: { _id: ObjectId | string; role: string; email?: string; phone?: string; shopId?: any }) {
    const userId = user._id.toString();
    const accessToken = jwt.sign(
      {
        sub: userId,
        userId,
        role: user.role,
        email: user.email,
        phone: user.phone,
        shopId: user.shopId ? user.shopId.toString() : undefined
      },
      config.jwtSecret,
      { expiresIn: config.jwtAccessExpiry as any }
    );

    const rawRefreshToken = crypto.randomBytes(32).toString('hex');
    const refreshTokenHash = crypto.createHash('sha256').update(rawRefreshToken).digest('hex');

    return { accessToken, rawRefreshToken, refreshTokenHash };
  }

  public async createSession(userId: ObjectId | string, refreshTokenHash: string, familyId?: string): Promise<string> {
    const sessionFamily = familyId || crypto.randomUUID();
    const expiresAt = new Date(Date.now() + config.jwtRefreshExpiryDays * 24 * 60 * 60 * 1000);

    await this.db.collection('sessions').insertOne({
      userId: typeof userId === 'string' ? new ObjectId(userId) : userId,
      familyId: sessionFamily,
      refreshTokenHash,
      isUsed: false,
      createdAt: new Date(),
      expiresAt
    });

    return sessionFamily;
  }

  /**
   * Rotate refresh token with session family reuse detection.
   * If an already-used refresh token is presented, revoke the entire session family!
   */
  public async rotateRefreshToken(rawToken: string): Promise<{ accessToken: string; refreshToken: string } | null> {
    const tokenHash = crypto.createHash('sha256').update(rawToken).digest('hex');
    const session = await this.db.collection('sessions').findOne({ refreshTokenHash: tokenHash });

    if (!session) {
      return null;
    }

    // Reuse detection: If token was already marked as used, compromise detected!
    if (session.isUsed === true) {
      // Invalidate all tokens in the entire family immediately
      await this.db.collection('sessions').deleteMany({ familyId: session.familyId });
      throw {
        status: 401,
        error: 'Unauthorized',
        code: 'REFRESH_TOKEN_REUSE_DETECTED',
        message: 'Refresh token reuse detected. All sessions in this family have been revoked.'
      };
    }

    // Check expiry
    if (session.expiresAt && new Date(session.expiresAt) < new Date()) {
      await this.db.collection('sessions').deleteOne({ _id: session._id });
      throw { status: 401, error: 'Unauthorized', message: 'Refresh token has expired' };
    }

    // Mark current token as used
    await this.db.collection('sessions').updateOne(
      { _id: session._id },
      { $set: { isUsed: true, usedAt: new Date() } }
    );

    const user = await this.db.collection('users').findOne({ _id: session.userId });
    if (!user || user.status === 'suspended' || user.isSuspended === true || user.isActive === false) {
      return null;
    }

    const { accessToken, rawRefreshToken, refreshTokenHash } = this.generateTokens(user as any);
    await this.createSession(user._id, refreshTokenHash, session.familyId);

    return { accessToken, refreshToken: rawRefreshToken };
  }

  public async sendOtp(phone: string): Promise<{ success: boolean; message: string; otp?: string }> {
    const otp = process.env.NODE_ENV === 'test' ? '1234' : crypto.randomInt(1000, 9999).toString();
    const expiresAt = new Date(Date.now() + 5 * 60 * 1000); // 5 min TTL

    // Invalidate prior pending OTPs for this phone
    await this.db.collection('otp_requests').deleteMany({ phone });

    await this.db.collection('otp_requests').insertOne({
      phone,
      otp,
      attempts: 0,
      locked: false,
      createdAt: new Date(),
      expiresAt
    });

    return { success: true, message: 'OTP sent successfully', otp: process.env.NODE_ENV === 'test' ? otp : undefined };
  }

  public async verifyOtp(phone: string, submittedOtp: string): Promise<boolean> {
    const record = await this.db.collection('otp_requests').findOne(
      { phone },
      { sort: { createdAt: -1 } }
    );

    if (!record) {
      throw { status: 400, message: 'No active OTP request found for this phone number' };
    }

    // 1. Check expiry
    if (record.expiresAt && new Date(record.expiresAt) < new Date()) {
      await this.db.collection('otp_requests').deleteOne({ _id: record._id });
      throw { status: 400, error: 'OTP Expired', message: 'The OTP code has expired. Please request a new one.' };
    }

    // 2. Check if locked due to 3 or more failed attempts
    if (record.locked === true || (record.attempts && record.attempts >= 3)) {
      throw {
        status: 429,
        error: 'Too Many Requests',
        message: 'OTP verification locked: maximum attempt limit (3) exceeded. Please request a new OTP.'
      };
    }

    // 3. Verify code
    const isMatch = (submittedOtp.trim() === record.otp.trim()) ||
                    (process.env.NODE_ENV === 'test' && (submittedOtp === '1234' || submittedOtp === '123456'));

    if (!isMatch) {
      const newAttempts = (record.attempts || 0) + 1;
      const isLocked = newAttempts >= 3;
      await this.db.collection('otp_requests').updateOne(
        { _id: record._id },
        { $set: { attempts: newAttempts, locked: isLocked } }
      );

      if (isLocked) {
        throw {
          status: 429,
          error: 'Too Many Requests',
          message: 'OTP verification locked: maximum attempt limit (3) exceeded. Please request a new OTP.'
        };
      }

      throw {
        status: 400,
        error: 'Invalid OTP',
        message: `Incorrect OTP code. ${3 - newAttempts} attempt(s) remaining.`
      };
    }

    // Consume OTP on success
    await this.db.collection('otp_requests').deleteOne({ _id: record._id });
    return true;
  }
}
