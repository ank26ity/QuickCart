# QuickCart Windows Desktop Distribution & Packaging Specification

## 1. Supported Platform Scope
* **Target OS**: Windows 10 (Version 2004 / Build 19041+) and Windows 11 (x64)
* **Architecture**: x86_64 / x64
* **Distribution Formats**: 
  - Standard EXE Installer via **Inno Setup 6** (`QuickCart_Setup_x64.exe`)
  - **MSIX Package** for Microsoft Store / Enterprise Intune deployment

---

## 2. Windows DPAPI Security Architecture
QuickCart stores sensitive authentication credentials and tokens strictly via **Windows DPAPI** (`CryptProtectData` and `CryptUnprotectData` from `wincrypt.h` linked to `Crypt32.lib`):
* Key material is encrypted using the logged-in Windows user's logon credentials and backed by hardware TPM whenever available.
* Encrypted vaults are isolated to `%LOCALAPPDATA%\QuickCart\dpapi_<keyhash>.dat`.
* CI runs on `windows-latest` validate DPAPI encryption, round-trip decryption, and session token clearing via `test_securestorage.exe`.

---

## 3. High-DPI Scaling & Display Precision
Windows displays feature fractional scaling (125%, 150%, 175%, 200%).
* **Scale Factor Rounding**: Configured in `main.cpp` via:
  ```cpp
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
  ```
* **Per-Monitor V2 DPI Awareness**: Declared in application manifest to prevent blurry text when dragging between monitors with different scaling factors.
* **Vector Glyphs & SVG Icons**: All UI iconography renders crisply across integer and non-integer device pixel ratios.

---

## 4. Code-Signing Specification

### Production Code Signing Plan (Azure Trusted Signing or Hardware EV Token)
Windows SmartScreen requires binaries to be signed by a trusted certificate authority:

```powershell
# 1. Sign application executable
signtool.exe sign /v /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /sha1 <CERT_THUMBPRINT> build/Release/QuickCartApp.exe

# 2. Build Inno Setup installer
iscc.exe packaging/windows/installer.iss

# 3. Sign the installer executable
signtool.exe sign /v /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /sha1 <CERT_THUMBPRINT> dist/QuickCart_Setup_x64.exe

# 4. Verify signature validity
signtool.exe verify /pa /v dist/QuickCart_Setup_x64.exe
```

---

## 5. Desktop-Class Console Layouts
* **Merchant Hub (`ShopkeeperView.qml`)**: Multi-column master-detail layout displaying store profile, live inventory table, quick action controls, and order dispatch workflows simultaneously without tab switching on displays >= 1024px.
* **Administrator Console (`AdminView.qml`)**: Full-width KPI dashboard with 4-card metric counters, quick platform actions, tabbed system monitors (Overview, Users, Shops), and filterable table views.
