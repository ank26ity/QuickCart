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
