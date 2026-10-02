pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import "../theme"

TextField {
    id: control

    placeholderTextColor: Theme.inputPlaceholder
    color: Theme.textPrimary
    selectByMouse: true
    implicitHeight: 44
    font.pixelSize: Theme.fontBody

    background: Rectangle {
        radius: Theme.radiusMedium
        color: Theme.inputBackground
        border.color: control.activeFocus ? Theme.inputBorderFocus : Theme.inputBorder
        border.width: 1

        Behavior on border.color { ColorAnimation { duration: Theme.durationFast } }
    }
}
