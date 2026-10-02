pragma Singleton
pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: root

    // Reference to active window width/height, updated dynamically from main window
    property real windowWidth: 1200
    property real windowHeight: 800

    // Safe area insets (for notches, camera cutouts, home indicators)
    property real safeAreaTop: 0
    property real safeAreaBottom: 0
    property real safeAreaLeft: 0
    property real safeAreaRight: 0

    // Virtual keyboard state
    property real keyboardHeight: 0
    readonly property bool isKeyboardVisible: keyboardHeight > 0

    // ── Breakpoint Scale ───────────────────────────────────────────────────
    readonly property bool isCompact: windowWidth < 600
    readonly property bool isMedium: windowWidth >= 600 && windowWidth < 840
    readonly property bool isExpanded: windowWidth >= 840 && windowWidth < 1200
    readonly property bool isExtraLarge: windowWidth >= 1200

    // Convenience helpers
    readonly property bool isMobile: isCompact
    readonly property bool isTablet: isMedium
    readonly property bool isDesktop: isExpanded || isExtraLarge

    // ── Grid & Layout Metrics ──────────────────────────────────────────────
    readonly property int columns: isCompact ? 1 : (isMedium ? 2 : (isExpanded ? 3 : 4))
    readonly property real gutter: isCompact ? 12 : (isMedium ? 16 : 24)
    readonly property real contentMaxWidth: 1280
    readonly property real minTouchTarget: 48 // Minimum WCAG 48dp touch target

    // Navigation Shell Dimensions
    readonly property real sidebarWidth: 260
    readonly property real navRailWidth: 72
    readonly property real bottomNavHeight: 64

    function dp(val: real): real {
        return val // Density-independent logical units in Qt Quick
    }
}
