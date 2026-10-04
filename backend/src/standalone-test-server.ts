import { setupTestContext } from '../tests/test-helper';

async function start() {
  process.env.PORT = '3000';
  const ctx = await setupTestContext();
  console.log(`[QuickCart Test Backend] Ready on ${ctx.baseUrl}`);

  const shutdown = async () => {
    console.log('[QuickCart Test Backend] Shutting down...');
    await ctx.close();
    process.exit(0);
  };

  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);
}

process.on('uncaughtException', (err) => {
  console.error('[QuickCart Test Backend] Uncaught Exception:', err);
});

process.on('unhandledRejection', (reason) => {
  console.error('[QuickCart Test Backend] Unhandled Rejection:', reason);
});

start().catch((err) => {
  console.error('[QuickCart Test Backend] Failed to start:', err);
  process.exit(1);
});
