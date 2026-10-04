# QuickCart - High Performance Multi-Role Commerce Client

QuickCart is a modern, reactive, multi-role quick-commerce platform built with C++17, Qt 6.8+ (QML), and high-security native hardware integrations.

---

## 1. Platform Support Matrix

QuickCart officially targets and ships on **Android**, **iOS**, and **Windows**. Linux is **NOT** a supported runtime platform, and macOS serves solely as a local developer environment and iOS simulator/test host.

| Platform | Shipped? | Built on | Tested how |
| :--- | :--- | :--- | :--- |
| **Android** | **Yes** (APK / AAB) | Linux CI Host (`ubuntu-24.04`) | Native target compile & APK generation; Keystore unit tests; planned emulator smoke test |
| **iOS** | **Yes** (IPA / TestFlight) | macOS Runner (`macos-14`) | Simulator build (unsigned) in CI; Apple Keychain unit tests; planned TestFlight lane |
| **Windows** | **Yes** (MSIX / Inno Setup) | Windows Runner (`windows-latest`) | Windows MSVC build + CTest (DPAPI & responsive UI golden tests) + windeployqt packaging |
| **macOS** | **No** (Dev Host Only) | Local Apple Silicon / Intel Mac | Local dev & iOS host; CTest + QML lint + ASan/UBSan + coverage |
| **Linux** | **No** (Unsupported) | Ubuntu 24.04 (CI Tooling Only) | Non-app CI infrastructure only: backend Node/MongoDB tests, lint/format, Android build host |

> [!IMPORTANT]
> **Qt on iOS Static Linking & Licensing**: Qt for iOS links statically. Development teams must consult Qt's licensing documentation regarding LGPLv3 compliance (e.g., providing object files for relinking or commercial licensing) prior to public App Store submission.

---

## 2. Hardware Security Architecture

All sensitive authentication credentials, refresh token families, and payment secrets are stored strictly within OS hardware-backed secure enclaves with **zero unauthenticated file fallbacks**:
* **iOS / macOS**: Apple Keychain Services (`SecItemAdd`, `SecItemCopyMatching`, `SecItemDelete`) backed by Apple Secure Enclave.
* **Windows**: Windows Data Protection API (**DPAPI** - `CryptProtectData`, `CryptUnprotectData` via `wincrypt.h` linked to `Crypt32.lib`) with user-credential encryption and TPM protection.
* **Android**: Android Keystore Provider with hardware-backed AES-256 keys inside TEE / StrongBox encrypting payloads via authenticated AES-256-GCM.

---

## 3. High-DPI Scaling & Desktop Consoles

* **Desktop Consoles**: Dedicated multi-column layouts for **Merchant Hub** (`ShopkeeperView.qml`) and **Administrator Console** (`AdminView.qml`) with side-by-side KPI monitors, tables, and live order queues on viewports >= 1024px.
* **Fractional DPI Scaling**: High-DPI scale factor passthrough enabled in `main.cpp` via `QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough)` for crisp rendering across 125%, 150%, 175%, and 200% Windows scaling.

---

## 4. Local Development & Testing

### Building Locally (macOS Developer Host)
```bash
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Running CTest Suites & Golden Checks
```bash
ctest --test-dir build --output-on-failure
```

### Running QML Linter
```bash
qmllint -I qml -I qml/theme -I qml/responsive -I qml/navigation -I qml/i18n -I qml/icons -I qml/config -I qml/components \
  qml/main.qml qml/theme/Theme.qml qml/theme/Metrics.qml qml/responsive/Responsive.qml qml/navigation/Router.qml \
  qml/i18n/Strings.qml qml/icons/Icons.qml qml/config/Config.qml qml/components/AppScaffold.qml \
  qml/components/CustomButton.qml qml/components/CustomTextField.qml qml/components/GlassCard.qml \
  qml/components/HeaderBar.qml qml/components/ThemeSelector.qml qml/views/AdminView.qml \
  qml/views/AuthView.qml qml/views/CustomerView.qml qml/views/DeliveryView.qml qml/views/ShopkeeperView.qml
```
