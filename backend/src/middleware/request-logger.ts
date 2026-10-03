import { Request, Response, NextFunction } from 'express';
import crypto from 'crypto';

export interface RequestLogMeta {
  timestamp: string;
  requestId: string;
  method: string;
  url: string;
  statusCode?: number;
  durationMs?: number;
  clientIpAnonymized?: string;
  userAgent?: string;
  userRole?: string;
}

export function sanitizeUrl(rawUrl: string): string {
  // 1. Strip query string (removes query params like ?lat=...&lng=..., tokens, filters)
  const pathWithoutQuery = rawUrl.split('?')[0];

  // 2. Strip any embedded latitude/longitude float coordinates in path
  return pathWithoutQuery.replace(/[-+]?\d{1,3}\.\d{4,}/g, '[REDACTED_COORD]');
}

export function anonymizeIp(rawIp?: string): string {
  if (!rawIp) return 'unknown';

  const ip = rawIp.replace(/^::ffff:/, ''); // Strip IPv4-mapped IPv6 prefix

  if (ip === '127.0.0.1' || ip === '::1') {
    return '127.0.***.***';
  }

  // IPv4 masking: zero out last two octets (e.g. 192.168.1.42 -> 192.168.***.***)
  if (ip.includes('.')) {
    const parts = ip.split('.');
    if (parts.length === 4) {
      return `${parts[0]}.${parts[1]}.***.***`;
    }
  }

  // IPv6 masking: keep prefix, mask rest
  if (ip.includes(':')) {
    const parts = ip.split(':');
    return `${parts.slice(0, 2).join(':')}:****:****`;
  }

  return 'anonymized';
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
      timestamp: new Date().toISOString(),
      requestId,
      method: req.method,
      url: sanitizeUrl(req.originalUrl || req.url),
      statusCode: res.statusCode,
      durationMs: parseFloat(durationMs.toFixed(2)),
      clientIpAnonymized: anonymizeIp(req.ip || req.socket.remoteAddress),
      userAgent: req.headers['user-agent'] ? String(req.headers['user-agent']).slice(0, 80) : undefined,
      userRole: (req as any).user?.role
    };

    // Structured JSON log without PII, passwords, coordinates, or Authorization tokens
    if (process.env.NODE_ENV !== 'test') {
      console.log(JSON.stringify(logEntry));
    }
  });

  next();
}
