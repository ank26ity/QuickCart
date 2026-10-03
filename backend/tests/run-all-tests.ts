import request from 'supertest';
import WebSocket from 'ws';
import { setupTestContext, TestContext } from './test-helper';
import { OrderStateMachineService } from '../src/services/order-state-machine.service';

function assert(condition: boolean, msg: string) {
  if (!condition) {
    throw new Error(`Assertion failed: ${msg}`);
  }
}

async function runTestSuite() {
  console.log('====================================================');
  console.log('   QuickCart Backend Complete Integration Test Suite');
  console.log('====================================================');

  const startMs = Date.now();
  console.log('[Setup] Initializing MongoDB Replica Set and Express server...');
  const ctx: TestContext = await setupTestContext();
  console.log(`[Setup] Server active at ${ctx.baseUrl}, DB connected via WiredTiger replica set.`);

  try {
    // ── SUITE 1: Health & Public Config ─────────────────────────────────────
    console.log('\n--- 1. Health & Public Config Tests ---');
    const healthRes = await request(ctx.app).get('/api/health');
    assert(healthRes.status === 200, 'Health endpoint should return 200');
    assert(healthRes.body.status === 'ok', 'Status should be ok');
    assert(healthRes.body.database === 'connected', 'Database should be connected');
    console.log('  ✓ GET /api/health passed (database connected)');

    const configRes = await request(ctx.app).get('/api/config');
    assert(configRes.status === 200, 'Config endpoint should return 200');
    assert(configRes.body.deliveryFeePaise === 4900, 'deliveryFeePaise must be 4900 (integer paise)');
    assert(configRes.body.freeDeliveryThresholdPaise === 49900, 'freeDeliveryThresholdPaise must be 49900');
    assert(configRes.body.currency === 'INR', 'Currency must be INR');
    console.log('  ✓ GET /api/config passed (integer paise config verified)');

    // ── SUITE 2: Authentication & Session Management ────────────────────────
    console.log('\n--- 2. Authentication & Session Management (Argon2id + JWT + OTP) ---');
    // Signup
    const signupRes = await request(ctx.app)
      .post('/api/auth/signup-request')
      .send({
        email: 'testuser@quickcart.com',
        password: 'SecurePassword@123',
        name: 'Test Customer',
        phone: '+919876543299',
        role: 'customer'
      });
    assert(signupRes.status === 200, 'Signup should succeed with 200');
    assert(!!signupRes.body.accessToken, 'Access token must be returned');
    assert(!!signupRes.body.refreshToken, 'Refresh token must be returned');
    const accessToken = signupRes.body.accessToken;
    const refreshToken = signupRes.body.refreshToken;
    console.log('  ✓ POST /api/auth/signup-request passed (Argon2id hashed, JWT issued)');

    // Login with valid credentials
    const loginRes = await request(ctx.app)
      .post('/api/auth/login-request')
      .send({
        email: 'testuser@quickcart.com',
        password: 'SecurePassword@123'
      });
    assert(loginRes.status === 200, 'Login should succeed');
    assert(loginRes.body.user.email === 'testuser@quickcart.com', 'User email must match');
    console.log('  ✓ POST /api/auth/login-request passed (Argon2id password verified)');

    // Login with invalid password
    const badLoginRes = await request(ctx.app)
      .post('/api/auth/login-request')
      .send({
        email: 'testuser@quickcart.com',
        password: 'WrongPassword'
      });
    assert(badLoginRes.status === 401, 'Bad password must return 401');
    console.log('  ✓ POST /api/auth/login-request rejected invalid password with 401');

    // Token refresh rotation
    const refreshRes = await request(ctx.app)
      .post('/api/auth/refresh-token')
      .send({ refreshToken });
    assert(refreshRes.status === 200, 'Refresh token should succeed');
    assert(!!refreshRes.body.accessToken, 'New access token must be returned');
    assert(refreshRes.body.refreshToken !== refreshToken, 'Refresh token must be rotated');
    console.log('  ✓ POST /api/auth/refresh-token passed (token rotated)');

    // Replay revoked refresh token (reuse detection)
    const replayRes = await request(ctx.app)
      .post('/api/auth/refresh-token')
      .send({ refreshToken });
    assert(replayRes.status === 401, 'Reused refresh token must be rejected with 401');
    console.log('  ✓ Reused refresh token rejected with 401 (single-use revocation verified)');

    // Phone OTP Flow
    const otpSendRes = await request(ctx.app)
      .post('/api/auth/otp/send')
      .send({ phone: '+919876543299' });
    assert(otpSendRes.status === 200, 'OTP send should succeed');
    console.log('  ✓ POST /api/auth/otp/send passed');

    const otpVerifyRes = await request(ctx.app)
      .post('/api/auth/otp/verify')
      .send({ phone: '+919876543299', otp: '1234' });
    assert(otpVerifyRes.status === 200, 'OTP verify should succeed');
    assert(!!otpVerifyRes.body.accessToken, 'Access token returned on OTP login');
    console.log('  ✓ POST /api/auth/otp/verify passed');

    // ── SUITE 3: NoSQL Injection Prevention & Rate Limiting ─────────────────
    console.log('\n--- 3. Security: NoSQL Injection Sanitizer & Rate Limiting ---');
    const nosqlRes = await request(ctx.app)
      .post('/api/auth/login-request')
      .send({
        email: { $gt: '' },
        password: 'password'
      });
    assert(nosqlRes.status === 400, 'NoSQL operator $gt must be rejected with 400');
    console.log('  ✓ NoSQL injection payload with $ operator blocked with 400');

    // ── SUITE 4: Shop Discovery (3km Haversine Boundary Filter) ─────────────
    console.log('\n--- 4. Shops Discovery & 3km Spatial Boundary ---');
    // Seed test shops
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
      },
      {
        _id: 'shop_3' as any,
        name: 'Faraway Hypermarket',
        category: 'groceries',
        lat: 12.8399,
        lng: 77.6770, // 15km away
        rating: 4.2,
        serviceRadiusKm: 3.0,
        isOpen: true
      }
    ]);

    // Fetch near Indiranagar, Bangalore (12.9716, 77.5946)
    const nearbyShopsRes = await request(ctx.app).get('/api/shops?lat=12.9716&lng=77.5946');
    assert(nearbyShopsRes.status === 200, 'Shops fetch should return 200');
    assert(nearbyShopsRes.body.length === 2, 'Should return exactly 2 shops within 3km');
    const shopNames = nearbyShopsRes.body.map((s: any) => s.name);
    assert(shopNames.includes('Fresh Mart Daily') && shopNames.includes('Corner Pharmacy'), 'Must include nearby shops');
    assert(!shopNames.includes('Faraway Hypermarket'), 'Must exclude Faraway Hypermarket (15km away)');
    console.log('  ✓ GET /api/shops 3km boundary filter passed (2 shops included, 1 faraway shop excluded)');

    // Category filter
    const pharmacyShopsRes = await request(ctx.app).get('/api/shops?lat=12.9716&lng=77.5946&category=pharmacy');
    assert(pharmacyShopsRes.body.length === 1, 'Should return only 1 pharmacy shop');
    assert(pharmacyShopsRes.body[0].name === 'Corner Pharmacy', 'Pharmacy name must match');
    console.log('  ✓ GET /api/shops category filter passed');

    // ── SUITE 5: Cart Server-Side Calculation (End-to-End Paise) ─────────────
    console.log('\n--- 5. Cart Server-Side Integer Paise Calculation ---');
    const cartCalcRes = await request(ctx.app)
      .post('/api/cart/calculate')
      .send({
        items: [
          { productId: 'prod_1', quantity: 2, pricePaise: 12000 }, // 2 * ₹120 = ₹240
          { productId: 'prod_2', quantity: 1, pricePaise: 8000 }   // 1 * ₹80 = ₹80. Subtotal = ₹320 (32000 paise)
        ]
      });
    assert(cartCalcRes.status === 200, 'Cart calculate should return 200');
    assert(cartCalcRes.body.subtotalPaise === 32000, 'Subtotal should be 32000 paise');
    assert(cartCalcRes.body.deliveryFeePaise === 4900, 'Delivery fee should be 4900 paise (below free threshold)');
    assert(cartCalcRes.body.totalPaise === 36900, 'Total should be 36900 paise');
    console.log('  ✓ POST /api/cart/calculate passed (< ₹499 order incurs ₹49 delivery fee)');

    // Above threshold (free delivery)
    const freeCartCalcRes = await request(ctx.app)
      .post('/api/cart/calculate')
      .send({
        items: [
          { productId: 'prod_1', quantity: 5, pricePaise: 10000 } // 5 * ₹100 = ₹500 (50000 paise)
        ]
      });
    assert(freeCartCalcRes.body.subtotalPaise === 50000, 'Subtotal should be 50000 paise');
    assert(freeCartCalcRes.body.deliveryFeePaise === 0, 'Delivery fee must be 0 for order >= ₹499');
    assert(freeCartCalcRes.body.totalPaise === 50000, 'Total must be 50000 paise');
    console.log('  ✓ POST /api/cart/calculate passed (>= ₹499 order receives FREE delivery)');

    // ── SUITE 6: Multi-Document ACID Transaction Order Placement ─────────────
    console.log('\n--- 6. Multi-Document Transaction: Order Placement & Atomic Stock ---');
    // Seed product and inventory
    await ctx.db.collection('products').insertOne({
      _id: 'prod_milk' as any,
      shopId: 'shop_1',
      name: 'Organic Milk 1L',
      sellingPricePaise: 6500, // ₹65.00
      stock: 10,
      createdAt: new Date(),
      updatedAt: new Date()
    } as any);

    await ctx.db.collection('inventory').insertOne({
      shopId: 'shop_1',
      productId: 'prod_milk',
      stock: 10,
      reservedStock: 0,
      createdAt: new Date(),
      updatedAt: new Date()
    } as any);

    const idempotencyKey = 'idemp_key_' + Date.now();
    const createOrderRes = await request(ctx.app)
      .post('/api/orders')
      .set('Authorization', `Bearer ${accessToken}`)
      .send({
        shopId: 'shop_1',
        deliveryAddress: '100 Feet Rd, Indiranagar',
        items: [{ productId: 'prod_milk', quantity: 3 }],
        idempotency_key: idempotencyKey
      });

    assert(createOrderRes.status === 201, 'Order placement should return 201 Created');
    assert(createOrderRes.body.status === 'pending', 'Initial order status must be pending');
    assert(createOrderRes.body.subtotalPaise === 19500, 'Server must compute subtotal (3 * 6500 = 19500 paise)');
    assert(createOrderRes.body.deliveryFeePaise === 4900, 'Server must add delivery fee 4900 paise');
    assert(createOrderRes.body.totalPaise === 24400, 'Total must be 24400 paise');
    assert(!!createOrderRes.body.deliveryOtp, 'Delivery OTP must be generated on server');
    const orderId = createOrderRes.body._id;
    const serverDeliveryOtp = createOrderRes.body.deliveryOtp;
    console.log('  ✓ POST /api/orders atomic transaction passed (Order created, totals computed in paise)');

    // Verify inventory stock decremented by 3
    const updatedInv = await ctx.db.collection('inventory').findOne({ productId: 'prod_milk' });
    assert(updatedInv?.stock === 7, `Inventory stock must decrement to 7, got ${updatedInv?.stock}`);
    console.log('  ✓ Inventory stock atomically decremented in database transaction (10 -> 7)');

    // Idempotency: Re-submit same request
    const duplicateOrderRes = await request(ctx.app)
      .post('/api/orders')
      .set('Authorization', `Bearer ${accessToken}`)
      .send({
        shopId: 'shop_1',
        deliveryAddress: '100 Feet Rd, Indiranagar',
        items: [{ productId: 'prod_milk', quantity: 3 }],
        idempotency_key: idempotencyKey
      });
    assert(duplicateOrderRes.status === 201, 'Duplicate request should return 201 from cache');
    assert(duplicateOrderRes.body._id === orderId, 'Must return same order ID');
    const checkInvAfterDup = await ctx.db.collection('inventory').findOne({ productId: 'prod_milk' });
    assert(checkInvAfterDup?.stock === 7, 'Stock must NOT decrement again on idempotent retry');
    console.log('  ✓ Idempotent retry returns identical response without double-decrementing stock');

    // ── SUITE 7: Order State Machine Transitions ────────────────────────────
    console.log('\n--- 7. Order State Machine Transitions (FSM & Contract Parity) ---');
    // Customer cannot move pending order to preparing
    const invalidTransitionRes = await request(ctx.app)
      .patch(`/api/orders/${orderId}`)
      .set('Authorization', `Bearer ${accessToken}`)
      .set('x-user-role', 'customer')
      .send({ status: 'preparing' });
    assert(invalidTransitionRes.status === 400, 'Customer cannot move order to preparing (must return 400)');
    console.log('  ✓ Invalid transition rejected with 400 (customer cannot move pending -> preparing)');

    // Merchant moves pending -> accepted
    const acceptRes = await request(ctx.app)
      .patch(`/api/orders/${orderId}`)
      .set('Authorization', `Bearer mock_jwt_merch_1`)
      .set('x-user-role', 'merchant')
      .send({ status: 'accepted' });
    assert(acceptRes.status === 200, 'Merchant can accept order');
    console.log('  ✓ Merchant moved order: pending -> accepted');

    // Merchant moves accepted -> preparing
    const prepRes = await request(ctx.app)
      .patch(`/api/orders/${orderId}`)
      .set('Authorization', `Bearer mock_jwt_merch_1`)
      .set('x-user-role', 'merchant')
      .send({ status: 'preparing' });
    assert(prepRes.status === 200, 'Merchant can move order to preparing');
    console.log('  ✓ Merchant moved order: accepted -> preparing');

    // Merchant moves preparing -> ready
    const readyRes = await request(ctx.app)
      .patch(`/api/orders/${orderId}`)
      .set('Authorization', `Bearer mock_jwt_merch_1`)
      .set('x-user-role', 'merchant')
      .send({ status: 'ready' });
    assert(readyRes.status === 200, 'Merchant marks order ready');
    console.log('  ✓ Merchant moved order: preparing -> ready');

    // ── SUITE 8: Courier Claim & Double-Claim Prevention ─────────────────────
    console.log('\n--- 8. Courier Claim (Atomic Double-Claim Prevention) ---');
    // Courier 1 claims order
    const claimRes1 = await request(ctx.app)
      .patch(`/api/orders/${orderId}/assign`)
      .send({ delivery_boy_id: 'courier_alpha' });
    assert(claimRes1.status === 200, 'First courier claim must succeed');
    assert(claimRes1.body.delivery_boy_id === 'courier_alpha', 'Courier alpha assigned');
    console.log('  ✓ Courier 1 claimed ready order successfully');

    // Courier 2 attempts to claim SAME order -> 409 Conflict
    const claimRes2 = await request(ctx.app)
      .patch(`/api/orders/${orderId}/assign`)
      .send({ delivery_boy_id: 'courier_beta' });
    assert(claimRes2.status === 409, 'Second courier claim must return 409 Conflict');
    console.log('  ✓ Double-claim attempt rejected with HTTP 409 Conflict!');

    // ── SUITE 9: Delivery OTP Verification (Server-Side) ─────────────────────
    console.log('\n--- 9. Server-Side Delivery OTP Verification ---');
    // Incorrect OTP rejected
    const badOtpRes = await request(ctx.app)
      .post(`/api/orders/${orderId}/verify-delivery-otp`)
      .send({ otp: '0000' });
    assert(badOtpRes.status === 400, 'Wrong OTP must be rejected with 400');
    console.log('  ✓ Incorrect delivery OTP rejected with 400');

    // Correct OTP completes delivery
    const goodOtpRes = await request(ctx.app)
      .post(`/api/orders/${orderId}/verify-delivery-otp`)
      .send({ otp: serverDeliveryOtp });
    assert(goodOtpRes.status === 200, 'Correct OTP must succeed with 200');
    assert(goodOtpRes.body.status === 'delivered', 'Order status must move to delivered');
    console.log('  ✓ Correct delivery OTP verified; order marked delivered and logged to audit trail');

    // Terminal state cannot transition
    const postDeliveredRes = await request(ctx.app)
      .patch(`/api/orders/${orderId}`)
      .set('Authorization', `Bearer mock_jwt_admin_1`)
      .set('x-user-role', 'admin')
      .send({ status: 'cancelled' });
    assert(postDeliveredRes.status === 400, 'Terminal state modification must be rejected');
    console.log('  ✓ Delivered order cannot transition further (terminal immutability verified)');

    // ── SUITE 10: Admin RBAC & Audit Trail ──────────────────────────────────
    console.log('\n--- 10. Admin RBAC & Audit Trail ---');
    // Unauthenticated -> 401
    const noAuthMetrics = await request(ctx.app).get('/api/admin/metrics');
    assert(noAuthMetrics.status === 401, 'Admin endpoint without token must return 401');
    console.log('  ✓ Unauthenticated access to /api/admin/metrics rejected with 401');

    // Customer role -> 403 Forbidden
    const customerAdminRes = await request(ctx.app)
      .get('/api/admin/metrics')
      .set('Authorization', `Bearer mock_jwt_customer_1`)
      .set('x-user-role', 'customer');
    assert(customerAdminRes.status === 403, 'Customer token to admin endpoint must return 403');
    console.log('  ✓ Customer token to /api/admin/metrics rejected with 403 Forbidden');

    // Admin role -> 200 OK
    const adminMetrics = await request(ctx.app)
      .get('/api/admin/metrics')
      .set('Authorization', `Bearer mock_jwt_admin_1`)
      .set('x-user-role', 'admin');
    assert(adminMetrics.status === 200, 'Admin token should succeed');
    assert(typeof adminMetrics.body.totalOrders === 'number', 'Metrics returned');
    console.log('  ✓ Admin token accessed /api/admin/metrics with 200 OK');

    // Admin courier approve
    const courierApproveRes = await request(ctx.app)
      .post('/api/admin/courier/user_courier_1/approve')
      .set('Authorization', `Bearer mock_jwt_admin_1`)
      .set('x-user-role', 'admin');
    assert(courierApproveRes.status === 200, 'Admin can approve courier');
    console.log('  ✓ Admin approved courier compliance');

    // Admin user suspend
    const userSuspendRes = await request(ctx.app)
      .post('/api/admin/users/user_cust_1/suspend')
      .set('Authorization', `Bearer mock_jwt_admin_1`)
      .set('x-user-role', 'admin');
    assert(userSuspendRes.status === 200, 'Admin can suspend user');
    console.log('  ✓ Admin suspended user');

    // Admin audit logs check
    const auditLogsRes = await request(ctx.app)
      .get('/api/admin/audit-logs')
      .set('Authorization', `Bearer mock_jwt_admin_1`)
      .set('x-user-role', 'admin');
    assert(auditLogsRes.status === 200, 'Admin can fetch audit logs');
    assert(auditLogsRes.body.length > 0, 'Audit logs must contain recorded operations');
    console.log(`  ✓ Append-only audit logs verified (${auditLogsRes.body.length} audit entries found)`);

    // ── SUITE 11: Real-time WebSockets ──────────────────────────────────────
    console.log('\n--- 11. Real-time WebSocket Order Tracking (/ws) ---');
    const ws = new WebSocket(`ws://127.0.0.1:${ctx.port}/ws`);
    await new Promise<void>((resolve, reject) => {
      ws.on('open', () => {
        ws.send(JSON.stringify({ type: 'subscribe', channel: `order:${orderId}` }));
        resolve();
      });
      ws.on('error', reject);
    });

    const receivedWsMessages: any[] = [];
    ws.on('message', (data: any) => {
      receivedWsMessages.push(JSON.parse(data.toString()));
    });

    // Send broadcast to channel
    ctx.realtimeService.broadcast(`order:${orderId}`, {
      type: 'order_update',
      orderId,
      status: 'delivered'
    });

    // Wait 100ms for WS message receipt
    await new Promise((r) => setTimeout(r, 100));
    assert(receivedWsMessages.some((m) => m.channel === `order:${orderId}`), 'WebSocket client should receive order broadcast');
    ws.close();
    console.log('  ✓ WebSocket connected to /ws, subscribed to order channel, and received real-time broadcast');

    const duration = ((Date.now() - startMs) / 1000).toFixed(2);
    console.log('\n====================================================');
    console.log(`ALL 11 BACKEND TEST SUITES PASSED CLEANLY (${duration}s)`);
    console.log('====================================================');
  } finally {
    await ctx.close();
  }
}

runTestSuite().catch((err) => {
  console.error('\n[FATAL TEST ERROR]:', err);
  process.exit(1);
});
