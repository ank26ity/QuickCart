# QuickCart System Architecture Documentation

---

## 1. Architectural Layers & Unidirectional Dependency Model

Dependencies in QuickCart strictly point **downward**. High-level presentation layers never directly mutate low-level database or raw socket constructs without passing through the intermediate architectural layers.

```mermaid
graph TD
    subgraph UI_Layer [1. Presentation Layer (QML)]
        Views[Screens / Views]
        Components[Atomic App Components]
        Singletons[QML Singletons: Theme, Metrics, Responsive, Router, Strings, Icons, Config]
    end

    subgraph ViewModel_Layer [2. ViewModels & Controllers (C++)]
        ShopModel[ShopModel]
        ProductModel[ProductModel]
        OrderModel[OrderModel]
        CartManager[CartManager]
        AuthService[AuthService]
        ThemeManager[ThemeManager]
        PermissionManager[PermissionManager]
    end

    subgraph Service_Layer [3. Domain Services & Security (C++)]
        SecureStorage[SecureStorage (Keychain / Vault)]
        CircuitBreaker[CircuitBreaker]
        NetworkManager[NetworkManager]
        ApiClient[ApiClient (Typed DTOs & Retries)]
    end

    subgraph Core_Layer [4. Core Infrastructure (C++)]
        Logging[StructuredLogger]
        AppConfig[AppConfig]
        Result[Result<T, AppError>]
        Validators[Validators (Email, Phone, GPS, Haversine, OTP)]
        OrderStateMachine[OrderStateMachine (FSM)]
    end

    UI_Layer -->|Binds to Properties & Invokes Slots| ViewModel_Layer
    ViewModel_Layer -->|Calls Methods| Service_Layer
    Service_Layer -->|Consumes| Core_Layer
```

---

## 2. Layer Definitions & Dependency Rules

| Layer | Location | Responsibilities | Allowed Dependencies | Prohibited Dependencies |
| :--- | :--- | :--- | :--- | :--- |
| **Presentation (QML)** | `qml/views/`, `qml/components/` | Declarative UI rendering, user interaction triggers, animations. Strictly zero business logic. | QML singletons (`Theme`, `Responsive`, `Router`, `Strings`), exposed C++ ViewModels. | Direct network calls, direct storage access, complex computations. |
| **QML Singletons** | `qml/{theme,responsive,navigation,i18n,icons,config}/` | Global app design tokens, screen breakpoints, routing state, localized strings, vector icons. | Base Qt Quick primitives, C++ Singletons (`AppConfig`, `ThemeManager`). | Visual components, view states. |
| **ViewModels (C++)** | `models/`, `services/` | State holding, data transformation, presentation models (`QAbstractListModel`), input validation. | Services, Repositories, Core Utilities. | QML types, raw UI pointers. |
| **Security & Services** | `security/`, `api/` | RBAC validation, hardware keychain storage, resilient HTTP client, circuit breaking, typed ApiClient. | Core layer (`Result`, `AppConfig`, `StructuredLogger`, `Validators`). | Presentation layer, ViewModels. |
| **Core Infrastructure** | `core/` | Global config, logging categories, monadic error handling (`Result<T, AppError>`), domain validators, order FSM. | Pure C++ standard library, Qt Core. | Any higher layer. |

---

## 3. "Where Do I Put New Code?" Guide

* **New Visual Control** (e.g. `AppCard`, `AppRatingBar`):
  $\rightarrow$ Place in `qml/components/`. Must consume tokens from `Theme` and metrics from `Metrics`. Must never hardcode pixel sizes or colors.
* **New Role Screen / Page** (e.g. `MerchantInventoryView`):
  $\rightarrow$ Place in `qml/views/`. Must bind to C++ ViewModels; must register routes in `Router.qml`.
* **New User-Facing String**:
  $\rightarrow$ Place in `qml/i18n/Strings.qml` wrapped in `qsTr(...)`. Never hardcode raw strings in views.
* **New UI Icon**:
  $\rightarrow$ Place vector rendering in `qml/icons/Icons.qml`.
* **New REST API Endpoint or DTO**:
  $\rightarrow$ Define typed DTOs and requests in `api/apiclient.h/.cpp` with structured `AppError` mapping.
* **New Business Rule / State Transition**:
  $\rightarrow$ Define in `core/orderstatemachine.h/.cpp` or `core/validators.h/.cpp`.

---

## 4. Build, Test, and Run Instructions

### 4.1 Prerequisites
* CMake 3.16+
* Clang / AppleClang with C++17 support
* Qt 6.5+ (Qt Core, Gui, Quick, QuickControls2, Network, Concurrent, Test)

### 4.2 Building QuickCart
```bash
# Configure build
cmake -B build -S . -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt

# Compile application and test targets
cmake --build build
```

### 4.3 Running Automated Tests (CTest)
```bash
# Run all unit tests via CTest
ctest --test-dir build --output-on-failure

# Or run individual test suites directly:
./build/test_contrast
./build/test_thememanager
./build/test_result
./build/test_validators
./build/test_orderstatemachine
./build/test_permissionmanager
./build/test_securestorage
./build/test_apiclient
./build/test_auth_flow
./build/test_customer_cart_flow
./build/test_merchant_flow
./build/test_courier_flow
./build/test_network_resilience
```

### 4.4 Running Static Analysis & Linting
```bash
# Run qmllint on all QML files
qmllint qml/theme/Theme.qml qml/responsive/Responsive.qml qml/components/AppScaffold.qml qml/main.qml
```

### 4.5 Running the Application
```bash
./build/QuickCartApp.app/Contents/MacOS/QuickCartApp
```

---

## 5. Platform Scope & SecureStorage Architecture

### 5.1 Supported Platform Matrix
QuickCart targets strictly **Android**, **iOS**, and **Windows**. Linux is **NOT** a supported runtime platform, and macOS is utilized solely as a local developer environment and iOS simulator/test host.

| Platform | Shipped? | Built on | Tested how |
| :--- | :--- | :--- | :--- |
| **Android** | **Yes** (APK / AAB) | Linux CI Host (Ubuntu 24.04) | Native build & APK packaging; Keystore unit test; planned emulator smoke test |
| **iOS** | **Yes** (IPA / TestFlight) | macOS Runner (macOS 14) | Simulator build (unsigned) in CI; Apple Keychain unit tests; planned TestFlight release lane |
| **Windows** | **Yes** (MSIX / Inno Setup) | Windows Runner (`windows-latest`) | Windows MSVC build + CTest (DPAPI & responsive UI golden tests) + windeployqt packaging |
| **macOS** | **No** (Dev Host Only) | Local Apple Silicon / Intel Mac | Local dev & iOS host; CTest + QML lint + ASan/UBSan + coverage |
| **Linux** | **No** (Unsupported) | Ubuntu 24.04 (CI Tooling Only) | Non-app CI infrastructure only: backend Node/MongoDB tests, formatting, Android build |

> [!NOTE]
> **Static Linking & Licensing Notice**: Qt for iOS links statically. Development teams must consult Qt's licensing documentation regarding LGPLv3 compliance (e.g. providing object files for relinking or commercial licensing) prior to public App Store submission.

### 5.2 Platform Hardware Storage Backends
QuickCart eliminates all unauthenticated file-based fallbacks (`.qc_master.key` / `~/.quickcart/key.bin`), routing credential and token storage strictly to hardware-backed OS secure enclaves:
* **macOS & iOS**: Apple Keychain Services (`SecItemAdd`, `SecItemCopyMatching`, `SecItemDelete`) with dynamic application-namespaced service identifiers (`QuickCartSecureVault_<appName>`) and Secure Enclave hardware binding.
* **Windows**: Windows Data Protection API (**DPAPI** - `CryptProtectData`, `CryptUnprotectData` via `wincrypt.h` linked to `Crypt32.lib`) with user-credential encryption and TPM protection.
* **Android**: Android Keystore Provider generating hardware-backed AES-256 keys inside TEE / StrongBox to encrypt credentials with AES-256-GCM.

### 5.3 Cryptographic Specification & Key Source
1. **Cipher**:
   * **Algorithm**: Advanced Encryption Standard with 256-bit key length (`AES-256`).
   * **Mode**: Galois/Counter Mode (`GCM`), providing Authenticated Encryption with Associated Data (AEAD).
   * **Initialization Vector (IV)**: Cryptographically secure 96-bit (12-byte) pseudo-random nonce generated per encryption via OpenSSL CSPRNG (`RAND_bytes`).
   * **Authentication Tag**: 128-bit (16-byte) Galois message authentication code generated by `EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag)`.
   * **Binary Layout**: `[12-byte IV] + [16-byte GCM Tag] + [Ciphertext]`

2. **Master Key Source**:
   * **Generation**: 256-bit cryptographically secure random key generated via OS CSPRNG (`RAND_bytes`).
   * **Storage**: Held directly inside OS hardware keystores (Apple Keychain, Windows DPAPI, Android Keystore).
   * **Non-Derivability**: **Never derived from machine GUID**, MAC address, hostname, or predictable static system properties.

3. **Passphrase-Based Key Derivation (PBKDF2)**:
   * When user passwords or PINs are provided for local key unlocking, key derivation enforces **100,000 iterations** of `PBKDF2-HMAC-SHA256` (`PKCS5_PBKDF2_HMAC`) paired with unique 128-bit cryptographic salts.

4. **Integrity & Tamper Detection**:
   * Decryption verifies the 16-byte GCM tag using `EVP_CTRL_GCM_SET_TAG` during `EVP_DecryptFinal_ex`.
   * Any bit modification to either the ciphertext, the IV, or the tag causes decryption to abort and return an empty buffer with an audit warning (`qcStorage`), preventing ciphertext tampering and bit-flipping attacks.

---

## 6. OWASP API Security Top 10 (2023) Compliance & Hardening Matrix

| OWASP Category | Vulnerability Description | QuickCart Implementation & Status | Verified Controls & Metrics |
| :--- | :--- | :--- | :--- |
| **API1:2023 Broken Object Level Authorization (BOLA)** | User accesses unauthorized resources by tampering with IDs in URIs. | **ENFORCED**<br>`authenticateJwt` extracts `userId` and `role`. MongoDB lookups strictly scope queries: customers can only access orders where `customerId == req.user.userId`. Cross-user access returns HTTP 403 Forbidden. | Single ID format (`toObjectId`); zero ambiguous `$or` lookups; automated test in Suite 9. |
| **API2:2023 Broken Authentication** | Compromised tokens, credential stuffing, lack of session family rotation. | **ENFORCED**<br>Dual-token architecture: short-lived JWT access tokens (15m) + cryptographic refresh token family rotation. Reused refresh token immediately revokes all descendant tokens in the family (`REFRESH_TOKEN_REUSE_DETECTED`, 401). Suspended users immediately rejected with 403. | bcrypt salt rounds = 12; PBKDF2 100k rounds on client vault; automated reuse revocation test in Suite 3; suspended token test in Suite 4. |
| **API3:2023 Broken Object Property Level Authorization (BOPLA)** | Mass assignment or sensitive property exposure (`is_admin`, `courierId`). | **ENFORCED**<br>Strict payload schema whitelisting. Deprecated or insecure fields (e.g. `delivery_boy_id`) are stripped; courier claim only accepts authorized on-duty couriers via atomic filter. | Prohibited properties filtered before DB query; courier assignment unsets duplicate legacy fields; verified in Suite 10. |
| **API4:2023 Unrestricted Resource Consumption** | Denial of Service via brute force, large uploads, or unthrottled requests. | **ENFORCED**<br>Tiered IP and user-based rate limiters with standard `Retry-After` header:<br>• Login: 5 requests / min (HTTP 429)<br>• OTP Send: 3 requests / 15 min<br>• OTP Verify: 3 failed attempts lockout (HTTP 429)<br>• General API: 60 requests / min<br>• Max upload size: 5MB enforced via presigned POST `content-length-range`. | Memory store in integration test; HTTP 429 with `Retry-After: 60s` verified in Suite 6; 3-attempt OTP lockout verified in Suite 5. |
| **API5:2023 Broken Function Level Authorization (BFLA)** | Regular user calls administrative or merchant state transitions. | **ENFORCED**<br>Role-based access middleware (`requireRole('admin')`, `requireRole('merchant')`, `requireRole('delivery')`). State transitions validate actor role against `order_transitions.json` 15-transition matrix. | 403 Forbidden on role mismatch; 400 Bad Request on invalid transition actor; verified in Suites 10, 11, and 13. |
| **API6:2023 Unrestricted Access to Sensitive Business Flows** | Order spamming, double courier claiming, inventory overselling. | **ENFORCED**<br>• Mandatory client-generated UUID idempotency key.<br>• Compound unique index `{ customerId: 1, idempotencyKey: 1 }` on `orders`.<br>• In-transaction SHA-256 canonical payload hash comparison (mismatch yields HTTP 422).<br>• Atomic `$inc: { stock: -qty }` prevents negative inventory.<br>• Atomic `status: 'ready', courierId: null` claim filter prevents double courier claim (returns HTTP 409 Conflict). | Concurrent single-stock race condition verified in Suite 8; same-key payload alteration 422 in Suite 9; double claim 409 in Suite 10. |
| **API7:2023 Server Side Request Forgery (SSRF)** | Exploitation of outgoing webhooks or image fetchers to attack internal network. | **ENFORCED**<br>Storage URLs strictly point to isolated S3 CDN bucket (`storage.quickcart.in`). No arbitrary remote URLs accepted for server-side fetching. | Presigned upload architecture offloads file transfer directly to object storage; zero server-side remote fetches. |
| **API8:2023 Security Misconfiguration** | Unsanitized NoSQL injection, error stack leakage, PII exposure in logs. | **ENFORCED**<br>• `noSqlSanitizer` recursively inspects body, query, and params; rejects any key containing `$` or `.` with HTTP 400.<br>• `requestLogger` anonymizes IPv4 (`192.168.***.***`) and IPv6, strips sensitive query parameters (`lat`, `lng`), and redacts URI coordinate paths. | Verified in Suite 3 (body and query NoSQL injection) and Suite 12 (privacy logging). |
| **API9:2023 Improper Inventory Management** | Zombie / shadow API endpoints, unversioned routes, lack of contract. | **ENFORCED**<br>Contract-first development with complete OpenAPI 3.0.3 specification (`docs/openapi.yaml`). All endpoints versioned under `/api/`. Single unified courier identifier (`courierId`). Legacy `/api/auth/login-request` and `/api/auth/login` fully documented. | Parity verified in `test_contract_backend.cpp` against OpenAPI contract schemas. |
| **API10:2023 Unsafe Consumption of APIs** | Blind trust in external S3 uploads, third-party webhook payloads. | **ENFORCED**<br>• S3 presigned POST policy with strict MIME whitelist (`image/jpeg`, `image/png`, `application/pdf`) and `content-length-range` [1024, 5242880].<br>• Post-upload verification endpoint (`/api/courier/documents/verify`) confirms file metadata before updating courier profile status to pending review. | Verified in Suite 10 (presigned POST policy + post-upload verification). |

### 6.1 Production Infrastructure Architecture Roadmap (TODOs)

The following security controls are architecturally planned for cloud deployment and documented as production deployment prerequisites:

1. **TLS Termination & HSTS Enforcement (TODO)**:
   - Deploy behind AWS ALB / Cloudflare / Nginx reverse proxy terminating TLS 1.3 exclusively.
   - Enforce HTTP Strict Transport Security (`Strict-Transport-Security: max-age=31536000; includeSubDomains; preload`).
2. **Helmet Security Headers Middleware (TODO)**:
   - Integrate `helmet` in production Express stack:
     - `Content-Security-Policy: default-src 'self'`
     - `X-Frame-Options: DENY`
     - `X-Content-Type-Options: nosniff`
     - `Referrer-Policy: strict-origin-when-cross-origin`
3. **Secrets Vault Management (TODO)**:
   - Transition production secrets (`JWT_SECRET`, MongoDB URI, TLS private keys) from `.env` files to AWS Secrets Manager / HashiCorp Vault with automated 90-day rotation.
4. **Distributed Redis Cluster for Rate Limiting & Pub/Sub (TODO)**:
   - Replace in-memory rate limiting and WebSocket client maps with a Redis Cluster (`ioredis` + `rate-limiter-flexible`).
   - Implement Redis Pub/Sub channels to enable transparent horizontal auto-scaling of backend Node.js worker pods.
5. **Automated OpenAPI Response Validation (TODO)**:
   - Integrate runtime OpenAPI response validation middleware (`express-openapi-validator`) to ensure all backend responses strictly adhere to `docs/openapi.yaml` schemas in production staging.



