import express, { Express, Request, Response, NextFunction } from 'express';
import cors from 'cors';
import { Db, MongoClient } from 'mongodb';
import { requestLogger } from './middleware/request-logger';
import { noSqlSanitizer } from './middleware/nosql-sanitizer';
import { generalLimiter } from './middleware/rate-limiter';
import { createApiRouter } from './routes/api.routes';
import { RealtimeService } from './services/realtime.service';

export function createApp(db: Db, client: MongoClient, realtimeService?: RealtimeService): Express {
  const app = express();

  app.use(cors({ origin: '*', methods: ['GET', 'POST', 'PATCH', 'PUT', 'DELETE', 'OPTIONS'] }));
  app.use(express.json({ limit: '10mb' }));
  app.use(express.urlencoded({ extended: true }));

  // Security & Observability Middlewares
  app.use(requestLogger);
  app.use(noSqlSanitizer);
  app.use('/api', generalLimiter);

  // Mount API Router
  app.use('/api', createApiRouter(db, client, realtimeService));

  // 404 Handler
  app.use((req: Request, res: Response) => {
    res.status(404).json({
      error: 'Not Found',
      message: `Endpoint ${req.method} ${req.originalUrl} not found`,
      statusCode: 404
    });
  });

  // Global Error Handler
  app.use((err: any, req: Request, res: Response, next: NextFunction) => {
    const statusCode = err.status || err.statusCode || 500;
    res.status(statusCode).json({
      error: err.error || 'Internal Server Error',
      message: err.message || 'An unexpected error occurred',
      statusCode
    });
  });

  return app;
}
