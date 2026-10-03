import http from 'http';
import { DatabaseManager } from './db/connection';
import { createApp } from './app';
import { RealtimeService } from './services/realtime.service';
import { config } from './config';

export async function startServer() {
  const dbManager = DatabaseManager.getInstance();
  const db = await dbManager.connect(config.mongoUri, config.dbName);
  const client = dbManager.getClient();

  const realtimeService = new RealtimeService(db);
  const app = createApp(db, client, realtimeService);
  const server = http.createServer(app);

  realtimeService.init(server);

  return new Promise<{ server: http.Server; close: () => Promise<void> }>((resolve) => {
    server.listen(config.port, () => {
      console.log(`[QuickCart Backend] Server listening on port ${config.port} (${config.nodeEnv})`);
      console.log(`[QuickCart Backend] WebSocket server active at ws://localhost:${config.port}/ws`);

      resolve({
        server,
        close: async () => {
          realtimeService.close();
          await new Promise<void>((r) => server.close(() => r()));
          await dbManager.disconnect();
        }
      });
    });
  });
}

if (require.main === module) {
  startServer().catch((err) => {
    console.error('[QuickCart Backend] Startup failure:', err);
    process.exit(1);
  });
}
