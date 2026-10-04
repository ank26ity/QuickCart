import request from 'supertest';
import WebSocket from 'ws';
import fs from 'fs';
import path from 'path';
import { setupTestContext, TestContext } from './test-helper';
import { OrderStateMachineService, OrderStatus, OrderActor } from '../src/services/order-state-machine.service';
import { sanitizeUrl, anonymizeIp } from '../src/middleware/request-logger';
import { toObjectId } from '../src/utils/id';

let totalAssertions = 0;

function assert(condition: boolean, msg: string) {
  totalAssertions++;
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

    // ── SUITE 2: Dynamic DB-Backed Delivery Config (Item 7) ─────────────────
    console.log('\n--- 2. Dynamic DB-Backed Delivery Config & Threshold ---');
    // Read default config
    const defaultCalc = await request(ctx.app)
      .post('/api/cart/calculate')
      .send({ items: [{ productId: 'p1', quantity: 1, pricePaise: 20000 }] });
    assert(defaultCalc.body.deliveryFeePaise === 4900, 'Default delivery fee 4900 paise');

    // Update config in database app_config collection
    await ctx.db.collection('app_config').updateOne(
      { key: 'delivery' },
      { $set: { freeDeliveryThresholdPaise: 60000, defaultDeliveryFeePaise: 7500 } },
      { upsert: true }
    );

    const dynamicCalc1 = await request(ctx.app)
      .post('/api/cart/calculate')
      .send({ items: [{ productId: 'p1', quantity: 1, pricePaise: 50000 }] }); // 50000 < 60000
    assert(dynamicCalc1.body.deliveryFeePaise === 7500, 'Must dynamically read ₹75 (7500 paise) fee from DB');
    assert(dynamicCalc1.body.totalPaise === 57500, 'Total must equal 57500 paise');

    const dynamicCalc2 = await request(ctx.app)
      .post('/api/cart/calculate')
      .send({ items: [{ productId: 'p1', quantity: 1, pricePaise: 65000 }] }); // 65000 >= 60000
    assert(dynamicCalc2.body.deliveryFeePaise === 0, 'Must dynamically apply free delivery above ₹600 (60000 paise)');
    console.log('  ✓ Delivery fee and free delivery threshold read dynamically from app_config collection');

    // Reset config back to default 49900 / 4900
    await ctx.db.collection('app_config').updateOne(
      { key: 'delivery' },
      { $set: { freeDeliveryThresholdPaise: 49900, defaultDeliveryFeePaise: 4900 } }
    );

    // ── SUITE 3: Authentication, Session Families & Reuse Detection (Item 6) 
    console.log('\n--- 3. Authentication & Session Family Reuse Revocation ---');
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
    const accessToken = signupRes.body.accessToken;
    const refreshToken = signupRes.body.refreshToken;
    const userId = signupRes.body.user._id;

    // Login
    const loginRes = await request(ctx.app)
      .post('/api/auth/login-request')
      .send({ email: 'testuser@quickcart.com', password: 'SecurePassword@123' });
    assert(loginRes.status === 200, 'Login succeeded');
    const rotatedRefresh1 = loginRes.body.refreshToken;

    // 1st Rotation: R1 -> R2
    const rotate1Res = await request(ctx.app)
      .post('/api/auth/refresh')
      .send({ refreshToken: rotatedRefresh1 });
    assert(rotate1Res.status === 200, 'First refresh token rotation succeeded');
    const rotatedRefresh2 = rotate1Res.body.refreshToken;

    // REUSE DETECTION: Present old rotatedRefresh1 again!
    const reuseRes = await request(ctx.app)
      .post('/api/auth/refresh')
      .send({ refreshToken: rotatedRefresh1 });
    assert(reuseRes.status === 401, 'Reused refresh token must be rejected with 401');
    assert(reuseRes.body.code === 'REFRESH_TOKEN_REUSE_DETECTED', 'Must identify token reuse');
    console.log('  ✓ Refresh token reuse detected; rejected with HTTP 401');

    // Entire session family must now be revoked: even rotatedRefresh2 must be invalid
    const followUpRes = await request(ctx.app)
      .post('/api/auth/refresh')
      .send({ refreshToken: rotatedRefresh2 });
    assert(followUpRes.status === 401, 'Session family revoked: subsequent active tokens in family invalidated');
    console.log('  ✓ Session family revocation verified: compromise causes complete invalidation');

    // NoSQL Operator Injection Security Checks
    const nosqlBodyRes = await request(ctx.app)
      .post('/api/auth/login-request')
      .send({ email: { $ne: null }, password: 'bad' });
    assert(nosqlBodyRes.status === 400, 'NoSQL operator injection in body must return HTTP 400');
    assert(nosqlBodyRes.body.error === 'Invalid request parameter', 'NoSQL error header must match');
    console.log('  ✓ NoSQL injection in body rejected with HTTP 400');

    const nosqlQueryRes = await request(ctx.app)
      .get('/api/shops?category[$ne]=grocery');
    assert(nosqlQueryRes.status === 400, 'NoSQL operator injection in query must return HTTP 400');
    console.log('  ✓ NoSQL injection in query rejected with HTTP 400');

    // ── SUITE 4: Suspended User Token Rejection (Item 6) ─────────────────────
    console.log('\n--- 4. Suspended User Token Immediate Rejection ---');
    // Issue fresh active token
    const freshLogin = await request(ctx.app)
      .post('/api/auth/login')
      .send({ email: 'testuser@quickcart.com', password: 'SecurePassword@123' });
    const activeToken = freshLogin.body.accessToken;

    // Check endpoint works with active token
    const testActiveRes = await request(ctx.app)
      .get('/api/orders')
      .set('Authorization', `Bearer ${activeToken}`);
    assert(testActiveRes.status === 200, 'Active user token accepted');

    // Admin suspends user
    await ctx.db.collection('users').updateOne(
      { email: 'testuser@quickcart.com' },
      { $set: { status: 'suspended', isSuspended: true, updatedAt: new Date() } }
    );

    // Subsequent request with the same token MUST BE REJECTED with 403 Forbidden!
    const suspendedTokenRes = await request(ctx.app)
      .get('/api/orders')
      .set('Authorization', `Bearer ${activeToken}`);
    assert(suspendedTokenRes.status === 403, `Suspended user token must return 403, got ${suspendedTokenRes.status}`);
    assert(suspendedTokenRes.body.error === 'Forbidden', 'Error must be Forbidden');
    console.log('  ✓ Suspended user token immediately rejected with HTTP 403 Forbidden');

    // Login for suspended user MUST ALSO BE REJECTED with 403
    const suspendedLoginRes = await request(ctx.app)
      .post('/api/auth/login')
      .send({ email: 'testuser@quickcart.com', password: 'SecurePassword@123' });
    assert(suspendedLoginRes.status === 403, 'Suspended user cannot login (403)');
    console.log('  ✓ Suspended user login rejected with HTTP 403 Forbidden');

    // Un-suspend user for remaining tests
    await ctx.db.collection('users').updateOne(
      { email: 'testuser@quickcart.com' },
      { $set: { status: 'active', isSuspended: false, updatedAt: new Date() } }
    );

    // ── SUITE 5: OTP Wrong-Code Lockout & Expiry (Item 6) ────────────────────
    console.log('\n--- 5. OTP Wrong-Code Lockout (3 Attempts) & Expiry ---');
    const phone = '+919988776655';
    await request(ctx.app).post('/api/auth/otp/send').send({ phone });

    // Attempt 1: Wrong code -> 400
    const otpTry1 = await request(ctx.app).post('/api/auth/otp/verify').send({ phone, otp: '9991' });
    assert(otpTry1.status === 400, 'Attempt 1 must return 400');

    // Attempt 2: Wrong code -> 400
    const otpTry2 = await request(ctx.app).post('/api/auth/otp/verify').send({ phone, otp: '9992' });
    assert(otpTry2.status === 400, 'Attempt 2 must return 400');

    // Attempt 3: Wrong code -> 429 Lockout!
    const otpTry3 = await request(ctx.app).post('/api/auth/otp/verify').send({ phone, otp: '9993' });
    assert(otpTry3.status === 429, `Attempt 3 must trigger lockout (429), got ${otpTry3.status}`);
    console.log('  ✓ 3 failed OTP attempts triggered HTTP 429 Lockout');

    // Attempt 4: Even correct code is locked out
    const otpTry4 = await request(ctx.app).post('/api/auth/otp/verify').send({ phone, otp: '1234' });
    assert(otpTry4.status === 429, 'Locked OTP rejects even correct code');
    console.log('  ✓ Locked OTP rejects further verification attempts');

    // OTP Expiry test
    await request(ctx.app).post('/api/auth/otp/send').send({ phone });
    // Manually expire OTP in DB
    await ctx.db.collection('otp_requests').updateOne(
      { phone },
      { $set: { expiresAt: new Date(Date.now() - 1000) } }
    );
    const otpExpiredRes = await request(ctx.app).post('/api/auth/otp/verify').send({ phone, otp: '1234' });
    assert(otpExpiredRes.status === 400, 'Expired OTP must be rejected with 400');
    console.log('  ✓ Expired OTP rejected with HTTP 400');

    // ── SUITE 6: Rate Limiting & 429 Header Verification (Item 6) ───────────
    console.log('\n--- 6. Rate Limiting 429 & Retry-After Header ---');
    let hitRateLimit = false;
    for (let i = 0; i < 25; i++) {
      const rlRes = await request(ctx.app)
        .post('/api/auth/login-request')
        .send({ email: 'unknown@test.com', password: 'bad' });
      if (rlRes.status === 429) {
        hitRateLimit = true;
        assert(!!rlRes.headers['retry-after'], 'HTTP 429 response must contain Retry-After header');
        console.log(`  ✓ Rate limit exceeded after ${i + 1} attempts: HTTP 429 with Retry-After: ${rlRes.headers['retry-after']}s`);
        break;
      }
    }
    assert(hitRateLimit, 'Rate limiter must enforce limit with 429');

    // ── SUITE 7: Order Validation: Quantity, Cross-Shop & Stock (Item 3) ─────
    console.log('\n--- 7. Order Validation: Quantity Limits, Cross-Shop & Stock ---');
    // Seed shops and products
    await ctx.db.collection('shops').insertMany([
      { _id: toObjectId('shop_A'), name: 'Shop Alpha', category: 'grocery', serviceRadiusKm: 5.0, isOpen: true },
      { _id: toObjectId('shop_B'), name: 'Shop Beta', category: 'grocery', serviceRadiusKm: 5.0, isOpen: true }
    ]);

    await ctx.db.collection('products').insertMany([
      { _id: toObjectId('p_alpha_1'), shopId: toObjectId('shop_A'), name: 'Product A1', sellingPricePaise: 5000, isActive: true },
      { _id: toObjectId('p_beta_1'), shopId: toObjectId('shop_B'), name: 'Product B1', sellingPricePaise: 7000, isActive: true },
      { _id: toObjectId('p_inactive'), shopId: toObjectId('shop_A'), name: 'Product Inactive', sellingPricePaise: 5000, isActive: false }
    ]);

    await ctx.db.collection('inventory').insertMany([
      { productId: toObjectId('p_alpha_1'), shopId: toObjectId('shop_A'), stock: 10, updatedAt: new Date() },
      { productId: toObjectId('p_beta_1'), shopId: toObjectId('shop_B'), stock: 10, updatedAt: new Date() },
      { productId: toObjectId('p_inactive'), shopId: toObjectId('shop_A'), stock: 10, updatedAt: new Date() }
    ]);

    const validUserToken = activeToken;

    // 1. Negative quantity -> 400
    const negQtyRes = await request(ctx.app)
      .post('/api/orders')
      .set('Authorization', `Bearer ${validUserToken}`)
      .send({
        shopId: 'shop_A',
        deliveryAddress: 'Test Address',
        items: [{ productId: 'p_alpha_1', quantity: -2 }],
        idempotencyKey: 'idemp_neg_' + Date.now()
      });
    assert(negQtyRes.status === 400, 'Negative quantity must return 400');
    console.log('  ✓ Negative quantity rejected with HTTP 400');

    // 2. Zero quantity -> 400
    const zeroQtyRes = await request(ctx.app)
      .post('/api/orders')
      .set('Authorization', `Bearer ${validUserToken}`)
      .send({
        shopId: 'shop_A',
        deliveryAddress: 'Test Address',
        items: [{ productId: 'p_alpha_1', quantity: 0 }],
        idempotencyKey: 'idemp_zero_' + Date.now()
      });
    assert(zeroQtyRes.status === 400, 'Zero quantity must return 400');
    console.log('  ✓ Zero quantity rejected with HTTP 400');

    // 3. Fractional quantity -> 400
    const fracQtyRes = await request(ctx.app)
      .post('/api/orders')
      .set('Authorization', `Bearer ${validUserToken}`)
      .send({
        shopId: 'shop_A',
        deliveryAddress: 'Test Address',
        items: [{ productId: 'p_alpha_1', quantity: 2.5 }],
        idempotencyKey: 'idemp_frac_' + Date.now()
      });
    assert(fracQtyRes.status === 400, 'Fractional quantity must return 400');
    console.log('  ✓ Fractional quantity rejected with HTTP 400');

    // 4. Exceeds max quantity (> 50) -> 400
    const maxQtyRes = await request(ctx.app)
      .post('/api/orders')
      .set('Authorization', `Bearer ${validUserToken}`)
      .send({
        shopId: 'shop_A',
        deliveryAddress: 'Test Address',
        items: [{ productId: 'p_alpha_1', quantity: 51 }],
        idempotencyKey: 'idemp_max_' + Date.now()
      });
    assert(maxQtyRes.status === 400, 'Quantity > 50 must return 400');
    console.log('  ✓ Quantity exceeding max (50) rejected with HTTP 400');

    // 5. Cross-shop items -> 400
    const crossShopRes = await request(ctx.app)
      .post('/api/orders')
      .set('Authorization', `Bearer ${validUserToken}`)
      .send({
        shopId: 'shop_A',
        deliveryAddress: 'Test Address',
        items: [
          { productId: 'p_alpha_1', quantity: 1 },
          { productId: 'p_beta_1', quantity: 1 } // belongs to shop_B!
        ],
        idempotencyKey: 'idemp_cross_' + Date.now()
      });
    assert(crossShopRes.status === 400, 'Cross-shop items must be rejected with 400');
    console.log('  ✓ Cross-shop item inclusion rejected with HTTP 400');

    // 6. Inactive product -> 400
    const inactiveRes = await request(ctx.app)
      .post('/api/orders')
      .set('Authorization', `Bearer ${validUserToken}`)
      .send({
        shopId: 'shop_A',
        deliveryAddress: 'Test Address',
        items: [{ productId: 'p_inactive', quantity: 1 }],
        idempotencyKey: 'idemp_inactive_' + Date.now()
      });
    assert(inactiveRes.status === 400, 'Inactive product must be rejected with 400');
    console.log('  ✓ Inactive product rejected with HTTP 400');

    // 7. Oversell -> 409 Conflict
    const oversellRes = await request(ctx.app)
      .post('/api/orders')
      .set('Authorization', `Bearer ${validUserToken}`)
      .send({
        shopId: 'shop_A',
        deliveryAddress: 'Test Address',
        items: [{ productId: 'p_alpha_1', quantity: 20 }], // Available: 10
        idempotencyKey: 'idemp_oversell_' + Date.now()
      });
    assert(oversellRes.status === 409, 'Oversell attempt must return 409 Conflict');
    console.log('  ✓ Oversell quantity rejected with HTTP 409 Conflict');

    // ── SUITE 8: Concurrent Two-Buyers-One-Item Race Condition (Item 3) ─────
    console.log('\n--- 8. Concurrent Race: Two Buyers Racing for 1 Single Stock ---');
    // Seed item with EXACTLY stock: 1
    await ctx.db.collection('products').insertOne({
      _id: toObjectId('p_rare_1'),
      shopId: toObjectId('shop_A'),
      name: 'Rare Item (Last Stock)',
      sellingPricePaise: 15000,
      isActive: true
    } as any);

    await ctx.db.collection('inventory').insertOne({
      productId: toObjectId('p_rare_1'),
      shopId: toObjectId('shop_A'),
      stock: 1,
      updatedAt: new Date()
    } as any);

    // Two buyers simultaneously submit orders for quantity: 1
    const orderRace1 = request(ctx.app)
      .post('/api/orders')
      .set('Authorization', `Bearer ${validUserToken}`)
      .send({
        shopId: 'shop_A',
        deliveryAddress: 'Buyer 1 Address',
        items: [{ productId: 'p_rare_1', quantity: 1 }],
        idempotencyKey: 'race_buyer_1_' + Date.now()
      });

    const orderRace2 = request(ctx.app)
      .post('/api/orders')
      .set('Authorization', `Bearer mock_jwt_customer_2`)
      .send({
        shopId: 'shop_A',
        deliveryAddress: 'Buyer 2 Address',
        items: [{ productId: 'p_rare_1', quantity: 1 }],
        idempotencyKey: 'race_buyer_2_' + Date.now()
      });

    const [raceRes1, raceRes2] = await Promise.all([orderRace1, orderRace2]);
    const statuses = [raceRes1.status, raceRes2.status].sort();

    // Exactly one must succeed (201) and one must fail (409 Conflict)
    assert(statuses[0] === 201 && statuses[1] === 409, `Expected [201, 409], got [${statuses[0]}, ${statuses[1]}]`);

    // Verify stock is exactly 0 and NEVER negative
    const finalStock = await ctx.db.collection('inventory').findOne({ productId: toObjectId('p_rare_1') });
    assert(finalStock?.stock === 0, `Final stock must be 0, got ${finalStock?.stock}`);
    console.log('  ✓ Concurrent race condition verified: exactly 1 buyer succeeded (201), 1 failed (409), final stock is 0');

    // ── SUITE 9: Idempotency Key Scoping & Payload Verification (Item 2) ─────
    console.log('\n--- 9. Idempotency Key Scoping & 422 Payload Conflict ---');
    const idempKeyShared = 'idemp_scoped_' + Date.now();

    // User A creates order with key
    const userAOrder1 = await request(ctx.app)
      .post('/api/orders')
      .set('Authorization', `Bearer ${validUserToken}`)
      .send({
        shopId: 'shop_A',
        deliveryAddress: 'Address A',
        items: [{ productId: 'p_alpha_1', quantity: 1 }],
        idempotencyKey: idempKeyShared
      });
    assert(userAOrder1.status === 201, 'User A order 1 created');
    const orderAId = userAOrder1.body._id;

    // User A resends SAME key with DIFFERENT payload -> 422 Unprocessable Entity!
    const userADiffBody = await request(ctx.app)
      .post('/api/orders')
      .set('Authorization', `Bearer ${validUserToken}`)
      .send({
        shopId: 'shop_A',
        deliveryAddress: 'Different Address Completely',
        items: [{ productId: 'p_alpha_1', quantity: 2 }],
        idempotencyKey: idempKeyShared
      });
    assert(userADiffBody.status === 422, `Same key + altered payload must return 422, got ${userADiffBody.status}`);
    console.log('  ✓ Same idempotency key with different payload rejected with HTTP 422 Unprocessable Entity');

    // User B attempts to use User A's idempotency key -> NEVER return User A's order!
    const userBOtherKey = await request(ctx.app)
      .post('/api/orders')
      .set('Authorization', `Bearer mock_jwt_customer_other`)
      .send({
        shopId: 'shop_A',
        deliveryAddress: 'Address B',
        items: [{ productId: 'p_alpha_1', quantity: 1 }],
        idempotencyKey: idempKeyShared
      });
    assert(userBOtherKey.status === 403, `User B using User A idempotency key must return 403 Forbidden, got ${userBOtherKey.status}`);
    assert(!userBOtherKey.body._id || userBOtherKey.body._id !== orderAId, 'Must NEVER return another user order');
    console.log('  ✓ User B prevented from accessing or receiving User A order via shared key (HTTP 403)');

    // Concurrent same-key test: two concurrent requests with identical key and payload
    const raceKey = 'idemp_race_' + Date.now();
    const raceOrderPayload = {
      shopId: 'shop_A',
      deliveryAddress: 'Address Race 1',
      items: [{ productId: 'p_alpha_1', quantity: 1 }],
      idempotencyKey: raceKey
    };

    const [resRace1, resRace2] = await Promise.all([
      request(ctx.app)
        .post('/api/orders')
        .set('Authorization', `Bearer ${validUserToken}`)
        .send(raceOrderPayload),
      request(ctx.app)
        .post('/api/orders')
        .set('Authorization', `Bearer ${validUserToken}`)
        .send(raceOrderPayload)
    ]);

    assert(
      (resRace1.status === 201 || resRace1.status === 200) &&
      (resRace2.status === 201 || resRace2.status === 200),
      `Concurrent same-key requests must succeed (got ${resRace1.status} and ${resRace2.status})`
    );
    assert(
      resRace1.body._id === resRace2.body._id,
      `Concurrent requests with same idempotency key must resolve to exact same order ID (${resRace1.body._id} vs ${resRace2.body._id})`
    );
    console.log('  ✓ Concurrent requests with identical idempotency key resolve safely to the exact same order');

    // ── SUITE 10: Courier Claim Hardening (Item 1) ───────────────────────────
    console.log('\n--- 10. Courier Claim: Status Filter, Approval & Unified courierId ---');
    // Seed courier users:
    const now = new Date();
    await ctx.db.collection('users').insertMany([
      {
        _id: toObjectId('courier_approved'),
        name: 'Courier Approved',
        email: 'courier_approved@quickcart.com',
        phone: '+919100000001',
        role: 'delivery',
        complianceStatus: 'approved',
        isOnline: true,
        onDuty: true,
        status: 'active',
        isActive: true,
        version: 1,
        createdAt: now,
        updatedAt: now
      },
      {
        _id: toObjectId('courier_unapproved'),
        name: 'Courier Unapproved',
        email: 'courier_unapproved@quickcart.com',
        phone: '+919100000002',
        role: 'delivery',
        complianceStatus: 'pending',
        isOnline: true,
        onDuty: true,
        status: 'active',
        isActive: true,
        version: 1,
        createdAt: now,
        updatedAt: now
      },
      {
        _id: toObjectId('courier_suspended'),
        name: 'Courier Suspended',
        email: 'courier_suspended@quickcart.com',
        phone: '+919100000003',
        role: 'delivery',
        complianceStatus: 'approved',
        isOnline: true,
        onDuty: true,
        status: 'suspended',
        isSuspended: true,
        isActive: false,
        version: 1,
        createdAt: now,
        updatedAt: now
      },
      {
        _id: toObjectId('courier_offduty'),
        name: 'Courier OffDuty',
        email: 'courier_offduty@quickcart.com',
        phone: '+919100000004',
        role: 'delivery',
        complianceStatus: 'approved',
        isOnline: false,
        onDuty: false,
        status: 'active',
        isActive: true,
        version: 1,
        createdAt: now,
        updatedAt: now
      },
      {
        _id: toObjectId('courier_approved_2'),
        name: 'Courier Approved 2',
        email: 'courier_approved_2@quickcart.com',
        phone: '+919100000005',
        role: 'delivery',
        complianceStatus: 'approved',
        isOnline: true,
        onDuty: true,
        status: 'active',
        isActive: true,
        version: 1,
        createdAt: now,
        updatedAt: now
      }
    ]);

    // Order currently in 'pending' status
    const pendingOrderId = orderAId;

    // 1. Claim while order is 'pending' -> REJECTED (must be 'ready')
    const claimPendingRes = await request(ctx.app)
      .patch(`/api/orders/${pendingOrderId}/assign`)
      .set('Authorization', 'Bearer mock_jwt_delivery_1')
      .set('x-user-role', 'delivery')
      .send({ delivery_boy_id: 'courier_approved' });
    assert(claimPendingRes.status === 400, 'Cannot claim order in pending status (must be ready)');
    console.log('  ✓ Claiming order in pending status rejected with HTTP 400 (only ready orders claimable)');

    // Advance order to 'ready'
    await ctx.db.collection('orders').updateOne(
      { _id: toObjectId(pendingOrderId) },
      { $set: { status: 'ready' } }
    );

    // 2. Unapproved courier claims -> 403
    const claimUnapproved = await request(ctx.app)
      .patch(`/api/orders/${pendingOrderId}/assign`)
      .set('Authorization', 'Bearer mock_jwt_delivery_1')
      .set('x-user-role', 'delivery')
      .send({ delivery_boy_id: 'courier_unapproved' });
    assert(claimUnapproved.status === 403, 'Unapproved courier claim rejected with 403');
    console.log('  ✓ Unapproved courier rejected with HTTP 403 Forbidden');

    // 3. Suspended courier claims -> 403
    const claimSuspended = await request(ctx.app)
      .patch(`/api/orders/${pendingOrderId}/assign`)
      .set('Authorization', 'Bearer mock_jwt_delivery_1')
      .set('x-user-role', 'delivery')
      .send({ delivery_boy_id: 'courier_suspended' });
    assert(claimSuspended.status === 403, 'Suspended courier claim rejected with 403');
    console.log('  ✓ Suspended courier rejected with HTTP 403 Forbidden');

    // 4. Off-duty courier claims -> 400
    const claimOffduty = await request(ctx.app)
      .patch(`/api/orders/${pendingOrderId}/assign`)
      .set('Authorization', 'Bearer mock_jwt_delivery_1')
      .set('x-user-role', 'delivery')
      .send({ delivery_boy_id: 'courier_offduty' });
    assert(claimOffduty.status === 400, 'Off-duty courier claim rejected with 400');
    console.log('  ✓ Off-duty courier rejected with HTTP 400 Bad Request');

    // 5. Approved, on-duty, non-suspended courier claims -> 200 OK!
    const claimApproved = await request(ctx.app)
      .patch(`/api/orders/${pendingOrderId}/assign`)
      .set('Authorization', 'Bearer mock_jwt_delivery_1')
      .set('x-user-role', 'delivery')
      .send({ delivery_boy_id: 'courier_approved' });
    assert(claimApproved.status === 200, 'Approved courier claim succeeds');
    assert(claimApproved.body.courierId === 'courier_approved', 'courierId must be set');
    assert(!claimApproved.body.delivery_boy_id, 'Duplicate delivery_boy_id must be dropped/unset');
    console.log('  ✓ Approved on-duty courier successfully claimed ready order; duplicate delivery_boy_id dropped');

    // 6. Double claim attempt: second courier claims already claimed order -> 409 Conflict
    const doubleClaimRes = await request(ctx.app)
      .patch(`/api/orders/${pendingOrderId}/assign`)
      .set('Authorization', 'Bearer mock_jwt_delivery_2')
      .set('x-user-role', 'delivery')
      .send({ delivery_boy_id: 'courier_approved_2' });
    assert(doubleClaimRes.status === 409, `Double claim attempt must return 409 Conflict, got ${doubleClaimRes.status}`);
    assert(doubleClaimRes.body.error === 'Conflict', 'Error must be Conflict');
    console.log('  ✓ Double-claim prevention: second courier rejected with HTTP 409 Conflict');

    // Presigned upload URL endpoint tests
    const presignedRes = await request(ctx.app)
      .post('/api/uploads/presigned-url')
      .set('Authorization', 'Bearer mock_jwt_delivery_1')
      .send({
        docType: 'driving_license',
        mimeType: 'image/jpeg',
        fileSize: 1024 * 500
      });
    assert(presignedRes.status === 200, 'Presigned URL generation succeeds');
    assert(presignedRes.body.uploadUrl && presignedRes.body.uploadUrl.startsWith('https://storage.quickcart.in/upload/'), 'Valid storage URL');
    assert(presignedRes.body.fileKey.includes('driving_license'), 'File key contains document type');
    assert(presignedRes.body.method === 'POST', 'Presigned upload method must be POST');
    assert(Array.isArray(presignedRes.body.conditions), 'Presigned POST conditions must be an array');
    const rangeCondition = presignedRes.body.conditions.find((c: any) => Array.isArray(c) && c[0] === 'content-length-range');
    assert(!!rangeCondition, 'Conditions must enforce content-length-range');
    console.log('  ✓ /api/uploads/presigned-url generates signed POST upload policy with content-length-range & MIME validation');

    // Post-upload document verification
    const verifyDocRes = await request(ctx.app)
      .post('/api/courier/documents/verify')
      .set('Authorization', 'Bearer mock_jwt_delivery_1')
      .send({
        courierId: 'courier_approved',
        docType: 'driving_license',
        fileKey: presignedRes.body.fileKey,
        actualBytes: 1024 * 500,
        actualMime: 'image/jpeg'
      });
    assert(verifyDocRes.status === 200, 'Post-upload verification succeeded');
    assert(verifyDocRes.body.verified === true, 'Document marked as verified');
    console.log('  ✓ Post-upload verification validates file metadata and sets verified status');

    const invalidMimeRes = await request(ctx.app)
      .post('/api/uploads/presigned-url')
      .set('Authorization', 'Bearer mock_jwt_delivery_1')
      .send({
        docType: 'script',
        mimeType: 'application/x-sh',
        fileSize: 100
      });
    assert(invalidMimeRes.status === 400, 'Invalid MIME type rejected');
    console.log('  ✓ Presigned URL rejects unauthorized MIME types with HTTP 400');

    // ── SUITE 11: WebSocket Authentication & Channel Authorization (Item 4) ─
    console.log('\n--- 11. WebSocket Authentication & Channel Authorization ---');
    // Direct broadcast only, unauthorized user cannot subscribe
    const wsUrl = `ws://127.0.0.1:${ctx.port}/ws`;

    // 1. Unauthorized customer attempts to subscribe to another user's order
    const unauthorizedWs = new WebSocket(`${wsUrl}?token=mock_jwt_customer_stranger`);
    const unauthorizedMsgs: any[] = [];

    await new Promise<void>((resolve, reject) => {
      const timer = setTimeout(() => resolve(), 1500);
      unauthorizedWs.on('open', () => {
        unauthorizedWs.send(JSON.stringify({ type: 'subscribe', channel: `order:${pendingOrderId}` }));
        clearTimeout(timer);
        resolve();
      });
      unauthorizedWs.on('error', (err) => {
        clearTimeout(timer);
        reject(err);
      });
      unauthorizedWs.on('message', (d) => unauthorizedMsgs.push(JSON.parse(d.toString())));
    });

    await new Promise((r) => setTimeout(r, 150));
    const forbiddenMsg = unauthorizedMsgs.find((m) => m.type === 'error' && m.error === 'FORBIDDEN');
    assert(!!forbiddenMsg, 'Unauthorized user subscription to order channel must be rejected with FORBIDDEN');
    unauthorizedWs.close();
    console.log('  ✓ Unauthorized WebSocket subscriber rejected with FORBIDDEN on foreign order channel');

    // 2. Authorized customer of the order subscribes -> allowed
    const authorizedWs = new WebSocket(`${wsUrl}?token=${encodeURIComponent(validUserToken)}`);
    const authorizedMsgs: any[] = [];

    await new Promise<void>((resolve, reject) => {
      const timer = setTimeout(() => resolve(), 1500);
      authorizedWs.on('open', () => {
        authorizedWs.send(JSON.stringify({ type: 'subscribe', channel: `order:${pendingOrderId}` }));
        clearTimeout(timer);
        resolve();
      });
      authorizedWs.on('error', (err) => {
        clearTimeout(timer);
        reject(err);
      });
      authorizedWs.on('message', (d) => authorizedMsgs.push(JSON.parse(d.toString())));
    });

    await new Promise((r) => setTimeout(r, 150));
    const subscribedMsg = authorizedMsgs.find((m) => m.type === 'subscribed' && m.channel === `order:${pendingOrderId}`);
    assert(!!subscribedMsg, 'Authorized customer subscription must succeed');

    // Direct broadcast delivers to authorized client
    ctx.realtimeService.broadcastOrderUpdate(pendingOrderId, 'out_for_delivery', 'courier_approved');
    await new Promise((r) => setTimeout(r, 100));

    const broadcastMsg = authorizedMsgs.find((m) => m.type === 'order_update' && m.status === 'out_for_delivery');
    assert(!!broadcastMsg, 'Direct broadcast delivered to authorized subscriber');
    authorizedWs.close();
    console.log('  ✓ Authorized customer successfully subscribed and received direct broadcast');

    // 3. Suspended user WebSocket closure
    const suspendWs = new WebSocket(`${wsUrl}?token=${encodeURIComponent(validUserToken)}`);
    let suspendClosedWithPolicy = false;
    let receivedCode = 0;

    await new Promise<void>((resolve) => {
      suspendWs.on('open', async () => {
        await ctx.db.collection('users').updateOne(
          { email: 'testuser@quickcart.com' },
          { $set: { status: 'suspended', isSuspended: true } }
        );
        await ctx.realtimeService.disconnectSuspendedClients();
      });
      suspendWs.on('close', (code) => {
        receivedCode = code;
        if (code === 1008) suspendClosedWithPolicy = true;
        resolve();
      });
      setTimeout(() => resolve(), 1000);
    });

    await ctx.db.collection('users').updateOne(
      { email: 'testuser@quickcart.com' },
      { $set: { status: 'active', isSuspended: false } }
    );

    assert(suspendClosedWithPolicy || receivedCode === 1008, `Suspended user WebSocket must close with code 1008, got ${receivedCode}`);
    console.log('  ✓ Suspended user WebSocket automatically closed with code 1008 (Policy Violation)');

    // ── SUITE 12: Logging Privacy: URL Sanitization & IP Masking (Item 5) ───
    console.log('\n--- 12. Logging Privacy: URL Query/Coord Stripping & IP Masking ---');
    // Test sanitizeUrl
    const urlWithCoordsAndQuery = '/api/shops?lat=28.613938&lng=77.209021&category=grocery';
    const cleanUrl = sanitizeUrl(urlWithCoordsAndQuery);
    assert(cleanUrl === '/api/shops', `URL must have query string stripped: ${cleanUrl}`);

    const urlWithEmbeddedCoords = '/api/shops/28.613938/77.209021/route';
    const cleanEmbedded = sanitizeUrl(urlWithEmbeddedCoords);
    assert(!cleanEmbedded.includes('28.613938') && cleanEmbedded.includes('[REDACTED_COORD]'), 'Coordinates stripped');
    console.log(`  ✓ sanitizeUrl: '${urlWithCoordsAndQuery}' -> '${cleanUrl}'`);
    console.log(`  ✓ sanitizeUrl: '${urlWithEmbeddedCoords}' -> '${cleanEmbedded}'`);

    // Test anonymizeIp
    const ipv4 = '192.168.1.142';
    const maskedIpv4 = anonymizeIp(ipv4);
    assert(maskedIpv4 === '192.168.***.***', `IPv4 should mask last 2 octets, got ${maskedIpv4}`);

    const ipv6 = '2001:0db8:85a3:0000:0000:8a2e:0370:7334';
    const maskedIpv6 = anonymizeIp(ipv6);
    assert(maskedIpv6.includes('****:****'), 'IPv6 masked');
    console.log(`  ✓ anonymizeIp: '${ipv4}' -> '${maskedIpv4}'`);
    console.log(`  ✓ anonymizeIp: '${ipv6}' -> '${maskedIpv6}'`);

    // ── SUITE 13: Order State Machine JSON Spec Parity Test (Item 8) ────────
    console.log('\n--- 13. State Machine Specification Parity with core/order_transitions.json ---');
    const specPath = path.resolve(__dirname, '../../core/order_transitions.json');
    assert(fs.existsSync(specPath), `Specification file must exist at ${specPath}`);
    const spec = JSON.parse(fs.readFileSync(specPath, 'utf8'));

    assert(Array.isArray(spec.transitions), 'Transitions array required');
    for (const t of spec.transitions) {
      for (const act of t.allowedActors) {
        const check = OrderStateMachineService.canTransition(
          t.from as OrderStatus,
          t.to as OrderStatus,
          act as OrderActor
        );
        assert(check.allowed, `Transition ${t.from} -> ${t.to} by actor ${act} must be permitted by OrderStateMachineService`);
      }
    }
    console.log(`  ✓ Verified all ${spec.transitions.length} transitions in core/order_transitions.json against OrderStateMachineService`);

    const duration = ((Date.now() - startMs) / 1000).toFixed(2);
    console.log('\n====================================================');
    console.log(`ALL 13 BACKEND TEST SUITES PASSED CLEANLY (${duration}s)`);
    console.log(`Verified ${totalAssertions} assertions across 13 test suites.`);
    console.log('====================================================');
  } finally {
    await ctx.close();
  }
}

runTestSuite().catch((err) => {
  console.error('\n[FATAL TEST ERROR]:', err);
  process.exit(1);
});
