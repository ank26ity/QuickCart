/**
 * @file Metrics.qml
 * @brief Layout metrics, touch targets, and sizing constants singleton.
 * @layer Presentation (QML Singleton)
 * @tests Covered by tst_responsive.qml and test_contrast
 */

pragma Singleton
pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: root

    // ── Touch Target Standards (WCAG 2.5.5 AAA / 2.5.8 AA) ────────────────
    readonly property real minTouchTarget: 48
    readonly property real touchTargetMedium: 52
    readonly property real touchTargetLarge: 60

    // ── Icon Sizes ────────────────────────────────────────────────────────
    readonly property real iconSmall: 16
    readonly property real iconMedium: 24
    readonly property real iconLarge: 32
    readonly property real iconXLarge: 48

    // ── Content Width Constraints ─────────────────────────────────────────
    readonly property real contentMaxWidth: 1280
    readonly property real formMaxWidth: 440
    readonly property real modalMaxWidth: 520
    readonly property real detailPanelWidth: 380

    // ── Shell Dimensions ──────────────────────────────────────────────────
    readonly property real sidebarWidth: 260
    readonly property real navRailWidth: 72
    readonly property real bottomNavHeight: 64
    readonly property real headerHeight: 70
}
