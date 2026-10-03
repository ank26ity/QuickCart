import { WebSocketServer, WebSocket } from 'ws';
import { Db, ChangeStream } from 'mongodb';
import { Server as HttpServer } from 'http';

interface SubscriptionClient {
  ws: WebSocket;
  channels: Set<string>;
  resumeToken?: any;
}

export class RealtimeService {
  private wss: WebSocketServer | null = null;
  private clients = new Set<SubscriptionClient>();
  private orderChangeStream: ChangeStream | null = null;

  constructor(private db: Db) {}

  public init(server: HttpServer): void {
    this.wss = new WebSocketServer({ server, path: '/ws' });

    this.wss.on('connection', (ws: WebSocket) => {
      const client: SubscriptionClient = {
        ws,
        channels: new Set<string>()
      };
      this.clients.add(client);

      ws.on('message', (message: string) => {
        try {
          const data = JSON.parse(message.toString());
          if (data.type === 'subscribe' && data.channel) {
            client.channels.add(data.channel);
            ws.send(JSON.stringify({ type: 'subscribed', channel: data.channel }));
          } else if (data.type === 'unsubscribe' && data.channel) {
            client.channels.delete(data.channel);
            ws.send(JSON.stringify({ type: 'unsubscribed', channel: data.channel }));
          } else if (data.type === 'ping') {
            ws.send(JSON.stringify({ type: 'pong', timestamp: Date.now() }));
          }
        } catch {
          // Ignore malformed payloads
        }
      });

      ws.on('close', () => {
        this.clients.delete(client);
      });
    });

    // Start MongoDB change streams if replica set is active
    this.startChangeStreams();
  }

  private async startChangeStreams(): Promise<void> {
    try {
      this.orderChangeStream = this.db.collection('orders').watch([], { fullDocument: 'updateLookup' });
      this.orderChangeStream.on('change', (change: any) => {
        if (change.operationType === 'update' || change.operationType === 'insert') {
          const doc = change.fullDocument;
          const orderId = doc._id.toString();
          this.broadcast(`order:${orderId}`, {
            type: 'order_update',
            orderId,
            status: doc.status,
            courierId: doc.delivery_boy_id || doc.courierId,
            timestamp: new Date().toISOString()
          });
        }
      });
    } catch {
      // Standalone mongod might not support change streams without replica set; graceful fallback
    }
  }

  public broadcast(channel: string, payload: any): void {
    const message = JSON.stringify({ channel, data: payload });
    for (const client of this.clients) {
      if (client.channels.has(channel) && client.ws.readyState === WebSocket.OPEN) {
        client.ws.send(message);
      }
    }
  }

  public close(): void {
    if (this.orderChangeStream) {
      this.orderChangeStream.close();
    }
    if (this.wss) {
      this.wss.close();
    }
  }
}
