import { Db, ObjectId } from 'mongodb';
import { BaseRepository, RepositoryError } from './base.repository';
import { InventoryItem } from '../types/models';

export class InventoryRepository extends BaseRepository<InventoryItem> {
  constructor(db: Db) {
    super(db, 'inventory');
  }

  /**
   * Atomic conditional stock reservation
   * Ensures that two concurrent checkouts for the last item cannot oversell
   */
  public async reserveStock(shopId: ObjectId, productId: ObjectId, quantity: number): Promise<boolean> {
    const result = await this.collection.findOneAndUpdate(
      {
        shopId,
        productId,
        stock: { $gte: quantity },
        deletedAt: null
      },
      {
        $inc: {
          stock: -quantity,
          reservedStock: quantity
        },
        $set: {
          updatedAt: new Date()
        }
      },
      { returnDocument: 'after' }
    );

    return result !== null;
  }

  /**
   * Release reserved stock back to active inventory (e.g. timeout or cancelled checkout)
   */
  public async releaseReservation(shopId: ObjectId, productId: ObjectId, quantity: number): Promise<boolean> {
    const result = await this.collection.findOneAndUpdate(
      {
        shopId,
        productId,
        reservedStock: { $gte: quantity }
      },
      {
        $inc: {
          stock: quantity,
          reservedStock: -quantity
        },
        $set: {
          updatedAt: new Date()
        }
      },
      { returnDocument: 'after' }
    );

    return result !== null;
  }

  /**
   * Finalize reservation upon successful order placement
   */
  public async commitReservation(shopId: ObjectId, productId: ObjectId, quantity: number): Promise<boolean> {
    const result = await this.collection.findOneAndUpdate(
      {
        shopId,
        productId,
        reservedStock: { $gte: quantity }
      },
      {
        $inc: {
          reservedStock: -quantity
        },
        $set: {
          updatedAt: new Date()
        }
      },
      { returnDocument: 'after' }
    );

    return result !== null;
  }
}
