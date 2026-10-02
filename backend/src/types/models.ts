import { ObjectId } from 'mongodb';

export type UserRole = 'customer' | 'shopkeeper' | 'delivery' | 'admin';
export type OrderStatus = 'pending' | 'preparing' | 'ready' | 'out_for_delivery' | 'delivered' | 'cancelled' | 'rejected';
export type PaymentStatus = 'pending' | 'authorized' | 'captured' | 'failed' | 'refunded';
export type PaymentMethod = 'upi' | 'card' | 'netbanking' | 'cod' | 'wallet';
export type DocVerificationStatus = 'pending' | 'verified' | 'rejected';
export type VehicleType = 'bicycle' | 'scooter' | 'motorcycle' | 'auto' | 'car';

export interface BaseDocument {
  _id: ObjectId;
  createdAt: Date;
  updatedAt: Date;
  version: number;
  deletedAt?: Date | null;
}

export interface GeoPoint {
  type: 'Point';
  coordinates: [number, number]; // [longitude, latitude]
}

export interface User extends BaseDocument {
  name: string;
  email: string;
  phone: string;
  passwordHash: string;
  role: UserRole;
  isActive: boolean;
  shopId?: ObjectId | null; // For shopkeeper
  defaultAddressId?: ObjectId | null;
}

export interface Address extends BaseDocument {
  userId: ObjectId;
  label: 'home' | 'work' | 'other';
  addressLine1: string;
  addressLine2?: string;
  landmark?: string;
  city: string;
  postalCode: string;
  location: GeoPoint;
  isDefault: boolean;
}

export interface Shop extends BaseDocument {
  name: string;
  ownerId: ObjectId;
  category: string;
  description: string;
  image: string;
  address: string;
  location: GeoPoint;
  rating: number;
  totalRatings: number;
  isOpen: boolean;
  openingTime: string; // e.g. "08:00"
  closingTime: string; // e.g. "23:00"
  serviceRadiusKm: number; // default 3.0
}

export interface Category extends BaseDocument {
  name: string;
  slug: string;
  description?: string;
  icon?: string;
  parentId?: ObjectId | null;
  path: string; // Materialized path, e.g. ",grocery,dairy,"
  displayOrder: number;
  isActive: boolean;
}

export interface ProductVariant {
  variantId: string;
  title: string; // e.g. "500ml", "1L"
  sku: string;
  pricePaise: number;
  mrpPaise: number;
  weightGrams?: number;
}

export interface ProductModifier {
  name: string; // e.g. "Extra Cheese"
  pricePaise: number;
}

export interface Product extends BaseDocument {
  shopId: ObjectId;
  categoryId: ObjectId;
  name: string;
  description: string;
  image: string;
  mrpPaise: number; // Money stored in paise (100 paise = 1 INR)
  sellingPricePaise: number;
  gstRatePercent: number; // e.g. 5, 12, 18
  isVeg: boolean;
  unit: string; // e.g. "pack", "kg", "bottle"
  variants: ProductVariant[];
  modifiers: ProductModifier[];
  isActive: boolean;
}

export interface InventoryItem extends BaseDocument {
  shopId: ObjectId;
  productId: ObjectId;
  stock: number;
  reservedStock: number; // Held during checkout
  lowStockThreshold: number; // default 5
}

export interface OrderItemSnapshot {
  productId: ObjectId;
  name: string;
  quantity: number;
  unitPricePaise: number;
  mrpPaise: number;
  gstAmountPaise: number;
  totalPricePaise: number;
  image?: string;
  selectedVariant?: string;
}

export interface Order extends BaseDocument {
  orderNumber: string; // e.g. "QC-20261002-8821"
  customerId: ObjectId;
  shopId: ObjectId;
  courierId?: ObjectId | null;
  status: OrderStatus;
  items: OrderItemSnapshot[];
  deliveryAddress: {
    addressLine: string;
    landmark?: string;
    city: string;
    location: GeoPoint;
    contactPhone: string;
  };
  pricing: {
    itemSubtotalPaise: number;
    deliveryFeePaise: number;
    surgeFeePaise: number;
    discountPaise: number;
    totalGstPaise: number;
    finalTotalPaise: number;
  };
  paymentStatus: PaymentStatus;
  paymentMethod: PaymentMethod;
  paymentId?: ObjectId | null;
  pickupOtp?: string;
  deliveryOtp?: string;
  idempotencyKey: string;
}

export interface OrderEvent extends BaseDocument {
  orderId: ObjectId;
  fromStatus?: OrderStatus;
  toStatus: OrderStatus;
  actorId: ObjectId;
  actorRole: UserRole;
  notes?: string;
  timestamp: Date;
}

export interface Courier extends BaseDocument {
  userId: ObjectId;
  vehicleType: VehicleType;
  licenseNumber: string;
  rcNumber: string;
  isOnline: boolean;
  isAssigned: boolean;
  currentOrderId?: ObjectId | null;
  currentLocation?: GeoPoint;
  totalEarningsPaise: number;
  totalTripsCompleted: number;
  rating: number;
}

export interface CourierDocument extends BaseDocument {
  courierId: ObjectId;
  docType: 'driving_license' | 'vehicle_rc' | 'national_id';
  docNumber: string;
  fileUrl: string; // Pre-signed S3 reference
  verificationStatus: DocVerificationStatus;
  rejectionReason?: string;
  verifiedBy?: ObjectId | null;
  verifiedAt?: Date | null;
}

export interface IdempotencyKeyRecord {
  _id: ObjectId;
  key: string;
  userId: ObjectId;
  endpoint: string;
  requestHash: string;
  responseStatus: number;
  responseBody: any;
  createdAt: Date;
  expiresAt: Date;
}
