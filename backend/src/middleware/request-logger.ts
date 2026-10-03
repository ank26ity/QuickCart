import { Request, Response, NextFunction } from 'express';
import crypto from 'crypto';

export interface RequestLogMeta {
  requestId: string;
  method: string;
  url: string;
  statusCode?: number;
  durationMs?: number;
  ip?: string;
  userAgent?: string;
  userRole?: string;
}

export function requestLogger(req: Request, res: Response, next: NextFunction): void {
  const startTime = process.hrtime();
  const requestId = (req.headers['x-request-id'] as string) || crypto.randomUUID();
  req.headers['x-request-id'] = requestId;
  res.setHeader('x-request-id', requestId);

  res.on('finish', () => {
    const diff = process.hrtime(startTime);
    const durationMs = (diff[0] * 1e3 + diff[1] * 1e-6);

    const logEntry: RequestLogMeta = {
      requestId,
      method: req.method,
      url: req.originalUrl || req.url,
      statusCode: res.statusCode,
      durationMs: parseFloat(durationMs.toFixed(2)),
      ip: req.ip || req.socket.remoteAddress,
      userAgent: req.headers['user-agent'],
      userRole: (req as any).user?.role
    };

    // Structured JSON log without PII, passwords, or Authorization tokens
    if (process.env.NODE_ENV !== 'test') {
      console.log(JSON.stringify(logEntry));
    }
  });

  next();
}
