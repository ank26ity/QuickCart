pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: headerBar

    property var authService: null

    implicitHeight: 70
    color: Theme.surfaceHeader
    border.color: Theme.border
    border.width: 1

    signal brandClicked()
    signal logoutClicked()

    Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    Behavior on border.color { ColorAnimation { duration: Theme.durationFast } }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.space24
        anchors.rightMargin: Theme.space24
        spacing: Theme.space16

        // Brand Title
        RowLayout {
            spacing: Theme.space12

            MouseArea {
                Layout.fillHeight: true
                implicitWidth: brandRow.implicitWidth
                cursorShape: Qt.PointingHandCursor
                onClicked: headerBar.brandClicked()

                RowLayout {
                    id: brandRow
                    spacing: Theme.space12

                    Rectangle {
                        Layout.preferredWidth: 38
                        Layout.preferredHeight: 38
                        radius: Theme.radiusMedium
                        color: Theme.primary

                        Text {
                            anchors.centerIn: parent
                            text: "⚡"
                            font.pixelSize: 20
                        }
                    }

                    ColumnLayout {
                        spacing: 0
                        Text {
                            text: "QuickShopp"
                            font.pixelSize: Theme.fontHeading
                            font.weight: Font.Bold
                            color: Theme.textPrimary
                        }
                        Text {
                            text: "Premium Hyperlocal Delivery"
                            font.pixelSize: Theme.fontSmall
                            color: Theme.textSecondary
                        }
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Live Theme Selector (System / Light / Dark)
        ThemeSelector {
            Layout.alignment: Qt.AlignVCenter
        }

        // User Profile & Role Info
        RowLayout {
            spacing: Theme.space16
            visible: headerBar.authService ? headerBar.authService.isLoggedIn : false

            // Role Badge
            Rectangle {
                Layout.preferredWidth: roleText.implicitWidth + Theme.space20
                Layout.preferredHeight: 28
                radius: Theme.radiusFull
                color: {
                    var role = headerBar.authService ? headerBar.authService.userRole : ""
                    if (role === "admin") return Qt.rgba(0.93, 0.26, 0.26, 0.15)
                    if (role === "shopkeeper") return Qt.rgba(0.96, 0.62, 0.04, 0.15)
                    if (role === "delivery") return Qt.rgba(0.38, 0.40, 0.94, 0.15)
                    return Qt.rgba(0.06, 0.72, 0.50, 0.15)
                }
                border.color: {
                    var role = headerBar.authService ? headerBar.authService.userRole : ""
                    if (role === "admin") return Theme.danger
                    if (role === "shopkeeper") return Theme.warning
                    if (role === "delivery") return Theme.secondary
                    return Theme.primary
                }

                Text {
                    id: roleText
                    anchors.centerIn: parent
                    text: headerBar.authService ? headerBar.authService.userRole.toUpperCase() : ""
                    font.pixelSize: Theme.fontSmall
                    font.weight: Font.Bold
                    color: {
                        var role = headerBar.authService ? headerBar.authService.userRole : ""
                        if (role === "admin") return Theme.danger
                        if (role === "shopkeeper") return Theme.warning
                        if (role === "delivery") return Theme.secondary
                        return Theme.primary
                    }
                }
            }

            Text {
                text: headerBar.authService ? headerBar.authService.userName : ""
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
                color: Theme.textPrimary
            }

            CustomButton {
                text: "Logout"
                variant: "outline"
                implicitHeight: 36
                onClicked: headerBar.logoutClicked()
            }
        }
    }
}
