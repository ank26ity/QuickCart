import dotenv from 'dotenv';
import path from 'path';

dotenv.config({ path: path.resolve(__dirname, '../../.env') });

export const config = {
  port: parseInt(process.env.PORT || '3000', 10),
  nodeEnv: process.env.NODE_ENV || 'development',
  mongoUri: process.env.MONGODB_URI || 'mongodb://127.0.0.1:27017,127.0.0.1:27018,127.0.0.1:27019/quickcart?replicaSet=rs0',
  dbName: process.env.DB_NAME || 'quickcart',
  redisUri: process.env.REDIS_URI || 'redis://127.0.0.1:6379',
  jwtSecret: process.env.JWT_SECRET || 'quickcart_dev_jwt_secret_key_minimum_32_bytes_long_12345',
  jwtRefreshSecret: process.env.JWT_REFRESH_SECRET || 'quickcart_dev_refresh_secret_key_minimum_32_bytes_67890',
  jwtAccessExpiry: '15m',
  jwtRefreshExpiryDays: 7,
  freeDeliveryThresholdPaise: 49900, // ₹499.00
  defaultDeliveryFeePaise: 4900,     // ₹49.00
  rateLimitMaxRequests: 100,
  rateLimitWindowMs: 60 * 1000
};
