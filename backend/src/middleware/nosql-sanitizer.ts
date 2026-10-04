import { Request, Response, NextFunction } from 'express';

function hasForbiddenKeys(obj: any): boolean {
  if (!obj || typeof obj !== 'object') {
    return false;
  }

  for (const key of Object.keys(obj)) {
    if (key.includes('$') || key.includes('.')) {
      return true;
    }
    if (typeof obj[key] === 'object' && hasForbiddenKeys(obj[key])) {
      return true;
    }
  }

  return false;
}

export function noSqlSanitizer(req: Request, res: Response, next: NextFunction): void {
  if (hasForbiddenKeys(req.body) || hasForbiddenKeys(req.query) || hasForbiddenKeys(req.params)) {
    res.status(400).json({
      error: 'Invalid request parameter',
      message: 'NoSQL operator injection detected. Prohibited keys ($ or .) are not allowed.',
      statusCode: 400
    });
    return;
  }

  next();
}
