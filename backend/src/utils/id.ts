import { ObjectId } from 'mongodb';
import crypto from 'crypto';

export function toObjectId(id: string | ObjectId): ObjectId {
  if (id instanceof ObjectId) return id;
  const str = String(id).trim();
  if (ObjectId.isValid(str) && str.length === 24) {
    return new ObjectId(str);
  }
  // Deterministic 24-character hex ObjectId from string ID
  const hash = crypto.createHash('md5').update(str).digest('hex').substring(0, 24);
  return new ObjectId(hash);
}
