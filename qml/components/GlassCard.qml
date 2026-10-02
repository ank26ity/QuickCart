pragma ComponentBehavior: Bound
import QtQuick
import "../theme"

Rectangle {
    id: glassCard

    property color strokeColor: Theme.surfaceBorder
    property real cardRadius: Theme.radiusLarge

    color: Theme.surfaceGlass
    radius: glassCard.cardRadius
    border.color: glassCard.strokeColor
    border.width: 1

    Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    Behavior on border.color { ColorAnimation { duration: Theme.durationFast } }
}
