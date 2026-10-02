import { Db, ObjectId } from 'mongodb';
import { BaseRepository } from './base.repository';
import { Shop } from '../types/models';

export interface ShopDistanceResult extends Shop {
  distanceMeters: number;
  distanceKm: number;
}

export class ShopRepository extends BaseRepository<Shop> {
  constructor(db: Db) {
    super(db, 'shops');
  }

  /**
   * Spatial 2dsphere aggregation finding open shops within a specified radius (default 3km)
   * Sorted ascending by distance
   */
  public async findShopsWithinRadius(
    lng: number,
    lat: number,
    radiusKm: number = 3.0,
    category?: string
  ): Promise<ShopDistanceResult[]> {
    const maxDistanceMeters = radiusKm * 1000;

    const pipeline: any[] = [
      {
        $geoNear: {
          near: {
            type: 'Point',
            coordinates: [lng, lat]
          },
          distanceField: 'distanceMeters',
          maxDistance: maxDistanceMeters,
          spherical: true,
          query: {
            isOpen: true,
            deletedAt: null
          }
        }
      }
    ];

    if (category && category !== 'All' && category !== '') {
      pipeline.push({
        $match: { category: category }
      });
    }

    pipeline.push({
      $addFields: {
        distanceKm: { $round: [{ $divide: ['$distanceMeters', 1000] }, 2] }
      }
    });

    pipeline.push({
      $sort: { distanceMeters: 1 }
    });

    return (await this.collection.aggregate(pipeline).toArray()) as ShopDistanceResult[];
  }
}
