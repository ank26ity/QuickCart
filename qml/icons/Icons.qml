/**
 * @file Icons.qml
 * @brief Centralized icon registry providing semantic SVG paths and glyph identifiers.
 * @layer Presentation (QML Singleton)
 */

pragma Singleton
pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: root

    // ── Semantic Icon Glyphs ───────────────────────────────────────────────
    readonly property string shop: "🏬"
    readonly property string cart: "🛒"
    readonly property string order: "📦"
    readonly property string user: "👤"
    readonly property string location: "📍"
    readonly property string lightning: "⚡"
    readonly property string star: "★"
    readonly property string check: "✓"
    readonly property string close: "✕"
    readonly property string back: "←"
    readonly property string power: "⏻"

    // ── System / Mode Icons ────────────────────────────────────────────────
    readonly property string systemMode: "system"
    readonly property string lightMode: "light"
    readonly property string darkMode: "dark"
}
