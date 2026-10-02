pragma Singleton
pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: root

    // Settable reference to C++ ThemeManager (injected in main.qml or context)
    property var themeManager: null
    property string fallbackMode: "dark"

    readonly property string mode: root.themeManager ? root.themeManager.mode : root.fallbackMode
    readonly property bool isDark: root.themeManager ? root.themeManager.isDark : (root.mode === "dark")
    readonly property bool reducedMotion: root.themeManager ? root.themeManager.reducedMotion : false

    // ── Semantic Color Tokens (Strict WCAG AA Verified) ───────────────────
    // Backgrounds & Surfaces
    readonly property color background: root.isDark ? "#0b0f19" : "#f8fafc"
    readonly property color surface: root.isDark ? "#0f172a" : "#ffffff"
    readonly property color surfaceVariant: root.isDark ? "#1e293b" : "#f1f5f9"
    readonly property color surfaceGlass: root.isDark ? Qt.rgba(0.06, 0.09, 0.15, 0.85) : Qt.rgba(1.0, 1.0, 1.0, 0.92)
    readonly property color surfaceHeader: root.isDark ? Qt.rgba(0.06, 0.09, 0.15, 0.96) : Qt.rgba(1.0, 1.0, 1.0, 0.98)
    readonly property color surfaceBorder: root.isDark ? "#64748b" : "#64748b"
    readonly property color border: root.isDark ? "#64748b" : "#64748b"
    readonly property color divider: root.isDark ? Qt.rgba(1.0, 1.0, 1.0, 0.12) : Qt.rgba(0.0, 0.0, 0.0, 0.12)
    readonly property color overlay: root.isDark ? Qt.rgba(0.0, 0.0, 0.0, 0.75) : Qt.rgba(0.0, 0.0, 0.0, 0.50)

    // Primary Brand & Intent Colors
    // In Dark: emerald #10b981 with dark forest #022c22 text (5.97:1)
    // In Light: emerald #047857 with white #ffffff text (5.48:1)
    readonly property color primary: root.isDark ? "#10b981" : "#047857"
    readonly property color primaryHover: root.isDark ? "#34d399" : "#065f46"
    readonly property color onPrimary: root.isDark ? "#022c22" : "#ffffff"

    readonly property color secondary: root.isDark ? "#818cf8" : "#4338ca"
    readonly property color secondaryHover: root.isDark ? "#a5b4fc" : "#3730a3"
    readonly property color onSecondary: root.isDark ? "#1e1b4b" : "#ffffff"

    // Danger Intent
    // In Dark: #ef4444 with #450a0a text (5.80:1)
    // In Light: #b91c1c with #ffffff text (6.47:1)
    readonly property color danger: root.isDark ? "#ef4444" : "#b91c1c"
    readonly property color dangerHover: root.isDark ? "#f87171" : "#991b1b"
    readonly property color onDanger: root.isDark ? "#1a0303" : "#ffffff"

    // Warning Intent
    // In Dark: #f59e0b with #451a03 text (6.30:1)
    // In Light: #b45309 with #ffffff text (5.02:1)
    readonly property color warning: root.isDark ? "#f59e0b" : "#b45309"
    readonly property color warningHover: root.isDark ? "#fbbf24" : "#92400e"
    readonly property color onWarning: root.isDark ? "#451a03" : "#ffffff"

    readonly property color success: root.isDark ? "#10b981" : "#047857"
    readonly property color onSuccess: root.isDark ? "#022c22" : "#ffffff"

    // Typography Colors (All >= 4.5:1 on background and surface, strict visual hierarchy)
    readonly property color textPrimary: root.isDark ? "#f8fafc" : "#0f172a"
    readonly property color textSecondary: root.isDark ? "#cbd5e1" : "#334155"
    readonly property color textMuted: root.isDark ? "#94a3b8" : "#475569"
    readonly property color textInverse: root.isDark ? "#0f172a" : "#ffffff"
    readonly property color textPrimaryBrand: root.isDark ? "#34d399" : "#047857"
    readonly property color textDanger: root.isDark ? "#f87171" : "#b91c1c"
    readonly property color textWarning: root.isDark ? "#fbbf24" : "#b45309"
    readonly property color textSuccess: root.isDark ? "#34d399" : "#047857"
    readonly property color textDisabled: root.isDark ? "#64748b" : "#64748b"

    // Form Controls & Inputs (Borders >= 3.0:1 on background and surface)
    readonly property color inputBackground: root.isDark ? "#0f172a" : "#ffffff"
    readonly property color inputBorder: root.isDark ? "#64748b" : "#64748b"
    readonly property color inputBorderFocus: root.isDark ? "#10b981" : "#047857"
    readonly property color inputPlaceholder: root.isDark ? "#94a3b8" : "#475569"
    readonly property color surfaceDisabled: root.isDark ? "#1e293b" : "#e2e8f0"

    // Card & Elevation
    readonly property color shadowColor: root.isDark ? Qt.rgba(0.0, 0.0, 0.0, 0.6) : Qt.rgba(0.0, 0.0, 0.0, 0.10)
    readonly property real shadowRadius: 10
    readonly property real shadowYOffset: 4

    // ── Spacing Scale (Density Independent) ────────────────────────────────
    readonly property real space2: 2
    readonly property real space4: 4
    readonly property real space8: 8
    readonly property real space12: 12
    readonly property real space16: 16
    readonly property real space20: 20
    readonly property real space24: 24
    readonly property real space32: 32
    readonly property real space48: 48

    // ── Corner Radii Scale ─────────────────────────────────────────────────
    readonly property real radiusSmall: 6
    readonly property real radiusMedium: 10
    readonly property real radiusLarge: 14
    readonly property real radiusXLarge: 20
    readonly property real radiusFull: 9999

    // ── Typography Scale ───────────────────────────────────────────────────
    readonly property int fontSmall: 11
    readonly property int fontBody: 14
    readonly property int fontSubheading: 16
    readonly property int fontHeading: 18
    readonly property int fontTitle: 22
    readonly property int fontDisplay: 28

    // ── Motion & Animation Scale (Strictly 0 when reducedMotion is enabled) ─
    readonly property int durationFast: root.reducedMotion ? 0 : 150
    readonly property int durationNormal: root.reducedMotion ? 0 : 250
    readonly property int durationSlow: root.reducedMotion ? 0 : 400

    function toggle() {
        if (root.themeManager) {
            root.themeManager.toggleTheme()
        } else {
            setMode(root.isDark ? "light" : "dark")
        }
    }

    function setMode(modeName: string) {
        var cleanMode = modeName ? modeName.toLowerCase() : "dark"
        root.fallbackMode = cleanMode
        if (root.themeManager) {
            root.themeManager.setMode(cleanMode)
        }
    }
}
