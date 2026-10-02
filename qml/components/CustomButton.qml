pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import "../theme"

Button {
    id: control

    property string variant: "primary" // "primary", "secondary", "danger", "outline"
    property color customColor: Theme.primary
    property real customRadius: Theme.radiusMedium

    implicitWidth: Math.max(100, contentItem ? contentItem.implicitWidth + Theme.space32 : 100)
    implicitHeight: 44

    contentItem: Text {
        text: control.text
        font.pixelSize: Theme.fontBody
        font.weight: Font.DemiBold
        color: {
            if (!control.enabled) return Theme.textMuted
            if (control.variant === "primary") return Theme.onPrimary
            if (control.variant === "danger") return Theme.onDanger
            if (control.variant === "secondary") return Theme.onSecondary
            if (control.variant === "outline") return Theme.textPrimary
            return Theme.onPrimary
        }
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: control.customRadius
        color: {
            if (!control.enabled) return Theme.surfaceVariant
            if (control.variant === "primary") return control.hovered ? Theme.primaryHover : Theme.primary
            if (control.variant === "danger") return control.hovered ? Theme.dangerHover : Theme.danger
            if (control.variant === "secondary") return control.hovered ? Theme.secondaryHover : Theme.secondary
            if (control.variant === "outline") return control.hovered ? Theme.surfaceVariant : "transparent"
            return control.customColor
        }
        border.color: {
            if (control.variant === "outline") return control.hovered ? Theme.primary : Theme.border
            return "transparent"
        }
        border.width: control.variant === "outline" ? 1 : 0

        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.durationFast } }
    }
}
