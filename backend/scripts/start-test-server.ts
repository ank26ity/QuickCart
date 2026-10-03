import { setupTestContext } from '../tests/test-helper';

async function main() {
  console.log('[TestServer] Booting real backend server on port 3000...');
  process.env.PORT = '3000';
  const ctx = await setupTestContext();

  // Seed sample shops and products
  await ctx.db.collection('shops').insertMany([
    {
      _id: 'shop_1' as any,
      name: 'Fresh Mart Daily',
      category: 'groceries',
      lat: 12.9716,
      lng: 77.5946,
      rating: 4.8,
      serviceRadiusKm: 3.0,
      isOpen: true
    },
    {
      _id: 'shop_2' as any,
      name: 'Corner Pharmacy',
      category: 'pharmacy',
      lat: 12.9750,
      lng: 77.5980,
      rating: 4.9,
      serviceRadiusKm: 3.0,
      isOpen: true
    }
  ]);

  console.log(`[TestServer] Real backend ready at ${ctx.baseUrl} (port 3000).`);

  const shutdown = async () => {
    console.log('[TestServer] Shutting down...');
    await ctx.close();
    process.exit(0);
  };

  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);
}

main().catch((err) => {
  console.error('[TestServer] Error:', err);
  process.exit(1);
});
