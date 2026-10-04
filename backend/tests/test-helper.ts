import { MongoMemoryReplSet } from 'mongodb-memory-server';
import { MongoClient, Db } from 'mongodb';
import http from 'http';
import { createApp } from '../src/app';
import { RealtimeService } from '../src/services/realtime.service';
import { runMigration001 } from '../src/db/migrations/001_create_collections_and_schemas';
import { seedDatabase } from '../src/db/seeds/seed';
import { DatabaseManager } from '../src/db/connection';

export interface TestContext {
  replSet?: MongoMemoryReplSet;
  client: MongoClient;
  db: Db;
  app: any;
  server: http.Server;
  realtimeService: RealtimeService;
  port: number;
  baseUrl: string;
  close: () => Promise<void>;
}

export async function setupTestContext(): Promise<TestContext> {
  let uri = process.env.MONGODB_URI;
  let replSet: MongoMemoryReplSet | undefined;

  if (!uri) {
    // Spin up real in-memory replica set for multi-document ACID transactions
    replSet = await MongoMemoryReplSet.create({
      replSet: { count: 1, storageEngine: 'wiredTiger' }
    });
    uri = replSet.getUri();
  }

  const client = new MongoClient(uri, {
    maxPoolSize: 50,
    retryWrites: true,
    w: 'majority'
  });
  await client.connect();
  const db = client.db('quickcart_test');

  // Run migrations & seed data
  await runMigration001(db);
  await seedDatabase(db);

  const realtimeService = new RealtimeService(db);
  const app = createApp(db, client, realtimeService);
  const server = http.createServer(app);

  realtimeService.init(server);

  const targetPort = parseInt(process.env.PORT || '0', 10);
  await new Promise<void>((resolve) => {
    server.listen(targetPort, '0.0.0.0', () => resolve());
  });

  const address = server.address() as any;
  const port = address.port;
  const baseUrl = `http://127.0.0.1:${port}/api`;

  return {
    replSet,
    client,
    db,
    app,
    server,
    realtimeService,
    port,
    baseUrl,
    close: async () => {
      realtimeService.close();
      await new Promise<void>((resolve) => server.close(() => resolve()));
      await client.close();
      if (replSet) {
        await replSet.stop();
      }
    }
  };
}
