import { Collection, Db, Filter, ObjectId, UpdateFilter, Document } from 'mongodb';
import { BaseDocument } from '../types/models';

export interface CursorPaginatedResult<T> {
  data: T[];
  nextCursor: string | null;
  hasMore: boolean;
  totalCount?: number;
}

export class RepositoryError extends Error {
  constructor(
    public readonly code: 'NOT_FOUND' | 'VERSION_CONFLICT' | 'DUPLICATE_KEY' | 'VALIDATION_FAILED' | 'UNAUTHORIZED' | 'DATABASE_ERROR',
    message: string,
    public readonly originalError?: any
  ) {
    super(message);
    this.name = 'RepositoryError';
  }
}

export abstract class BaseRepository<T extends BaseDocument> {
  protected collection: Collection<T>;

  constructor(protected db: Db, protected collectionName: string) {
    this.collection = db.collection<T>(collectionName);
  }

  public async findById(id: ObjectId, enforceOwnerId?: ObjectId, ownerField: string = 'userId'): Promise<T | null> {
    const filter: any = { _id: id, deletedAt: null };
    if (enforceOwnerId) {
      filter[ownerField] = enforceOwnerId;
    }
    return (await this.collection.findOne(filter as Filter<T>)) as T | null;
  }

  public async create(docData: Omit<T, '_id' | 'createdAt' | 'updatedAt' | 'version' | 'deletedAt'>): Promise<T> {
    const now = new Date();
    const newDoc = {
      ...docData,
      _id: new ObjectId(),
      version: 1,
      createdAt: now,
      updatedAt: now,
      deletedAt: null
    } as unknown as T;

    try {
      await this.collection.insertOne(newDoc as any);
      return newDoc;
    } catch (err: any) {
      this.handleMongoError(err);
    }
  }

  public async updateWithOptimisticLock(
    id: ObjectId,
    expectedVersion: number,
    updateFields: Partial<Omit<T, '_id' | 'createdAt' | 'updatedAt' | 'version'>>
  ): Promise<T> {
    const now = new Date();
    const filter = {
      _id: id,
      version: expectedVersion,
      deletedAt: null
    } as unknown as Filter<T>;

    const update = {
      $set: {
        ...updateFields,
        updatedAt: now
      },
      $inc: {
        version: 1
      }
    } as unknown as UpdateFilter<T>;

    try {
      const result = await this.collection.findOneAndUpdate(filter, update, { returnDocument: 'after' });
      if (!result) {
        // Verify if document exists or version conflicted
        const existing = await this.collection.findOne({ _id: id } as Filter<T>);
        if (!existing) {
          throw new RepositoryError('NOT_FOUND', `Document with id ${id.toHexString()} not found.`);
        }
        throw new RepositoryError('VERSION_CONFLICT', `Concurrent modification detected for document ${id.toHexString()}. Expected version ${expectedVersion}, found ${existing.version}.`);
      }
      return result as T;
    } catch (err: any) {
      if (err instanceof RepositoryError) throw err;
      this.handleMongoError(err);
    }
  }

  public async softDelete(id: ObjectId, enforceOwnerId?: ObjectId, ownerField: string = 'userId'): Promise<boolean> {
    const filter: any = { _id: id, deletedAt: null };
    if (enforceOwnerId) {
      filter[ownerField] = enforceOwnerId;
    }

    const result = await this.collection.updateOne(filter, {
      $set: { deletedAt: new Date(), updatedAt: new Date() }
    } as any);

    if (result.matchedCount === 0) {
      throw new RepositoryError('NOT_FOUND', `Document ${id.toHexString()} not found or access denied.`);
    }
    return result.modifiedCount > 0;
  }

  /**
   * Cursor-based pagination avoiding expensive skip/limit
   */
  public async findWithCursorPagination(
    filter: Filter<T> = {},
    cursor?: string | null,
    limit: number = 20,
    sortField: keyof T = 'createdAt',
    sortAscending: boolean = false
  ): Promise<CursorPaginatedResult<T>> {
    const safeLimit = Math.min(Math.max(1, limit), 100);
    const queryFilter: any = { ...filter, deletedAt: null };

    if (cursor) {
      const cursorVal = sortField === '_id' ? new ObjectId(cursor) : new Date(cursor);
      queryFilter[sortField] = sortAscending ? { $gt: cursorVal } : { $lt: cursorVal };
    }

    const sortOrder = sortAscending ? 1 : -1;
    const items = await this.collection
      .find(queryFilter)
      .sort({ [sortField]: sortOrder } as any)
      .limit(safeLimit + 1)
      .toArray();

    const hasMore = items.length > safeLimit;
    const resultData = (hasMore ? items.slice(0, safeLimit) : items) as T[];

    let nextCursor: string | null = null;
    if (hasMore && resultData.length > 0) {
      const lastItem: any = resultData[resultData.length - 1];
      const val = lastItem[sortField];
      nextCursor = val instanceof Date ? val.toISOString() : (val instanceof ObjectId ? val.toHexString() : String(val));
    }

    return {
      data: resultData,
      nextCursor,
      hasMore
    };
  }

  protected handleMongoError(err: any): never {
    if (err.code === 11000) {
      throw new RepositoryError('DUPLICATE_KEY', 'A unique constraint violation occurred.', err);
    }
    if (err.code === 121) {
      throw new RepositoryError('VALIDATION_FAILED', 'Document failed $jsonSchema validation rules.', err);
    }
    throw new RepositoryError('DATABASE_ERROR', err.message || 'An unexpected database error occurred.', err);
  }
}
