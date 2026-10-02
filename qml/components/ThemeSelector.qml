pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import "../theme"

RowLayout {
    id: selectorRoot

    spacing: Theme.space8

    Repeater {
        model: [
            { id: "system", label: "System", iconType: "system" },
            { id: "light", label: "Light", iconType: "light" },
            { id: "dark", label: "Dark", iconType: "dark" }
        ]

        delegate: Rectangle {
            id: optRect
            required property var modelData
            required property int index

            readonly property bool isSelected: Theme.mode === optRect.modelData.id

            implicitWidth: optRow.implicitWidth + Theme.space24
            implicitHeight: 34
            radius: Theme.radiusMedium

            color: optRect.isSelected ? Theme.primary : (hoverArea.containsMouse ? Theme.surfaceVariant : "transparent")
            border.color: optRect.isSelected ? Theme.primary : Theme.border
            border.width: 1

            Accessible.role: Accessible.Button
            Accessible.name: optRect.modelData.label + " Theme"
            Accessible.description: "Set appearance mode to " + optRect.modelData.label
            Accessible.focusable: true

            Behavior on color { ColorAnimation { duration: Theme.durationFast } }
            Behavior on border.color { ColorAnimation { duration: Theme.durationFast } }

            RowLayout {
                id: optRow
                anchors.centerIn: parent
                spacing: Theme.space8

                // Tokenized vector icon
                Item {
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16

                    Canvas {
                        id: iconCanvas
                        anchors.fill: parent
                        readonly property color iconColor: optRect.isSelected ? Theme.onPrimary : Theme.textPrimary

                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.reset();
                            ctx.strokeStyle = iconCanvas.iconColor;
                            ctx.fillStyle = iconCanvas.iconColor;
                            ctx.lineWidth = 1.5;
                            ctx.lineCap = "round";
                            ctx.lineJoin = "round";

                            if (optRect.modelData.iconType === "system") {
                                // Laptop / Screen
                                ctx.strokeRect(1, 2, 14, 9);
                                ctx.beginPath();
                                ctx.moveTo(0, 13);
                                ctx.lineTo(16, 13);
                                ctx.stroke();
                            } else if (optRect.modelData.iconType === "light") {
                                // Sun: central circle + rays
                                ctx.beginPath();
                                ctx.arc(8, 8, 3.5, 0, 2 * Math.PI);
                                ctx.fill();
                                ctx.beginPath();
                                ctx.moveTo(8, 1); ctx.lineTo(8, 3);
                                ctx.moveTo(8, 13); ctx.lineTo(8, 15);
                                ctx.moveTo(1, 8); ctx.lineTo(3, 8);
                                ctx.moveTo(13, 8); ctx.lineTo(15, 8);
                                ctx.stroke();
                            } else if (optRect.modelData.iconType === "dark") {
                                // Crescent moon
                                ctx.beginPath();
                                ctx.arc(8, 8, 5.5, -0.6 * Math.PI, 0.7 * Math.PI, false);
                                ctx.arc(9.5, 7, 4.5, 0.7 * Math.PI, -0.6 * Math.PI, true);
                                ctx.closePath();
                                ctx.fill();
                            }
                        }

                        Connections {
                            target: optRect
                            function onIsSelectedChanged() { iconCanvas.requestPaint(); }
                        }
                    }
                }

                Text {
                    text: optRect.modelData.label
                    font.pixelSize: Theme.fontSmall
                    font.weight: optRect.isSelected ? Font.Bold : Font.Normal
                    color: optRect.isSelected ? Theme.onPrimary : Theme.textPrimary
                }
            }

            MouseArea {
                id: hoverArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    Theme.setMode(optRect.modelData.id)
                }
            }
        }
    }
}
