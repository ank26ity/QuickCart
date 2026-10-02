import { MongoClient, Db } from 'mongodb';

export class DatabaseManager {
  private static instance: DatabaseManager;
  private client: MongoClient | null = null;
  private db: Db | null = null;

  private constructor() {}

  public static getInstance(): DatabaseManager {
    if (!DatabaseManager.instance) {
      DatabaseManager.instance = new DatabaseManager();
    }
    return DatabaseManager.instance;
  }

  public async connect(uri?: string, dbName: string = 'quickcart'): Promise<Db> {
    if (this.db) return this.db;

    const mongoUri = uri || process.env.MONGODB_URI || 'mongodb://127.0.0.1:27017,127.0.0.1:27018,127.0.0.1:27019/quickcart?replicaSet=rs0';

    this.client = new MongoClient(mongoUri, {
      maxPoolSize: 100,
      minPoolSize: 10,
      maxIdleTimeMS: 30000,
      connectTimeoutMS: 8000,
      retryWrites: true,
      retryReads: true,
      w: 'majority',
    });

    await this.client.connect();
    this.db = this.client.db(dbName);
    console.log(`[MongoDB] Connected successfully to database: ${dbName} (Replica Set)`);

    process.on('SIGINT', async () => {
      await this.disconnect();
      process.exit(0);
    });

    process.on('SIGTERM', async () => {
      await this.disconnect();
      process.exit(0);
    });

    return this.db;
  }

  public getDb(): Db {
    if (!this.db) {
      throw new Error('Database not connected. Call connect() first.');
    }
    return this.db;
  }

  public getClient(): MongoClient {
    if (!this.client) {
      throw new Error('MongoClient not connected. Call connect() first.');
    }
    return this.client;
  }

  public async disconnect(): Promise<void> {
    if (this.client) {
      await this.client.close();
      this.client = null;
      this.db = null;
      console.log('[MongoDB] Connection pool closed.');
    }
  }
}
