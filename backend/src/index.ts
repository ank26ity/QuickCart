export * from './server';
export * from './app';
export * from './config';
export * from './db/connection';
export * from './services/order-state-machine.service';
export * from './services/auth.service';
export * from './services/order.service';
export * from './services/courier.service';
export * from './services/admin.service';
export * from './services/realtime.service';

if (require.main === module) {
  require('./server');
}
