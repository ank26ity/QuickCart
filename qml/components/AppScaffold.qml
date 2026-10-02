pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../responsive"

Item {
    id: scaffoldRoot

    property var authService: null
    property var permissionManager: null
    property int currentNavIndex: 0
    default property alias content: contentSlot.data

    signal brandClicked()
    signal logoutClicked()
    signal navIndexChanged(int index)

    readonly property var currentNavModel: {
        var role = (scaffoldRoot.authService && scaffoldRoot.authService.userRole) ? scaffoldRoot.authService.userRole.toLowerCase() : "customer"
        if (role === "admin") {
            return [
                { label: "Overview", icon: "🛡️", id: 0 },
                { label: "Users & RBAC", icon: "👥", id: 1 },
                { label: "Partner Stores", icon: "🏬", id: 2 }
            ]
        } else if (role === "shopkeeper") {
            return [
                { label: "Storefront", icon: "🏪", id: 0 },
                { label: "Order Queue", icon: "📦", id: 1 },
                { label: "Inventory", icon: "🏷️", id: 2 }
            ]
        } else if (role === "delivery") {
            return [
                { label: "Active Jobs", icon: "🛵", id: 0 },
                { label: "Trip History", icon: "📋", id: 1 },
                { label: "Earnings", icon: "💰", id: 2 }
            ]
        }
        return [
            { label: "Marketplace", icon: "🏬", id: 0 },
            { label: "My Orders", icon: "📦", id: 1 },
            { label: "Profile", icon: "👤", id: 2 }
        ]
    }

    // Update Responsive window dimensions dynamically
    Binding {
        target: Responsive
        property: "windowWidth"
        value: scaffoldRoot.width
    }
    Binding {
        target: Responsive
        property: "windowHeight"
        value: scaffoldRoot.height
    }

    // ── Main Shell Layout ──────────────────────────────────────────────────
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 1. Desktop Persistent Sidebar (Width >= 840dp)
        Rectangle {
            id: sidebar
            objectName: "sidebar"
            visible: Responsive.isDesktop && (scaffoldRoot.authService ? scaffoldRoot.authService.isLoggedIn : false)
            Layout.fillHeight: true
            Layout.preferredWidth: Responsive.sidebarWidth
            color: Theme.surface
            border.color: Theme.border
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.space16
                spacing: Theme.space16

                // Brand Header
                RowLayout {
                    spacing: Theme.space12
                    Rectangle {
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                        radius: Theme.radiusMedium
                        color: Theme.primary
                        Text {
                            anchors.centerIn: parent
                            text: "⚡"
                            font.pixelSize: 18
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
                            text: "Hyperlocal Express"
                            font.pixelSize: Theme.fontSmall
                            color: Theme.textSecondary
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.divider
                }

                // Nav Links
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.space8

                    Repeater {
                        model: scaffoldRoot.currentNavModel

                        delegate: Rectangle {
                            id: navItemRect
                            required property var modelData
                            required property int index

                            Layout.fillWidth: true
                            implicitHeight: Responsive.minTouchTarget
                            radius: Theme.radiusMedium
                            color: scaffoldRoot.currentNavIndex === navItemRect.modelData.id ? Theme.surfaceVariant : (navHover.containsMouse ? Theme.surfaceVariant : "transparent")
                            border.color: scaffoldRoot.currentNavIndex === navItemRect.modelData.id ? Theme.primary : "transparent"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.space16
                                anchors.rightMargin: Theme.space16
                                spacing: Theme.space12

                                Text {
                                    text: navItemRect.modelData.icon
                                    font.pixelSize: 16
                                }
                                Text {
                                    text: navItemRect.modelData.label
                                    font.pixelSize: Theme.fontBody
                                    font.weight: scaffoldRoot.currentNavIndex === navItemRect.modelData.id ? Font.Bold : Font.Normal
                                    color: scaffoldRoot.currentNavIndex === navItemRect.modelData.id ? Theme.primary : Theme.textPrimary
                                }
                            }

                            MouseArea {
                                id: navHover
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    scaffoldRoot.currentNavIndex = navItemRect.modelData.id
                                    scaffoldRoot.navIndexChanged(navItemRect.modelData.id)
                                }
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // Theme Toggle
                ThemeSelector {
                    Layout.fillWidth: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.divider
                }

                // User Info & Logout
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.space12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Text {
                            text: scaffoldRoot.authService ? scaffoldRoot.authService.userName : ""
                            font.pixelSize: Theme.fontBody
                            font.weight: Font.DemiBold
                            color: Theme.textPrimary
                            elide: Text.ElideRight
                        }
                        Text {
                            text: scaffoldRoot.authService ? scaffoldRoot.authService.userRole.toUpperCase() : ""
                            font.pixelSize: Theme.fontSmall
                            color: Theme.primary
                        }
                    }

                    CustomButton {
                        text: "Logout"
                        variant: "outline"
                        implicitHeight: 36
                        onClicked: scaffoldRoot.logoutClicked()
                    }
                }
            }
        }

        // 2. Tablet Navigation Rail (600dp <= Width < 840dp)
        Rectangle {
            id: navRail
            objectName: "navRail"
            visible: Responsive.isTablet && (scaffoldRoot.authService ? scaffoldRoot.authService.isLoggedIn : false)
            Layout.fillHeight: true
            Layout.preferredWidth: Responsive.navRailWidth
            color: Theme.surface
            border.color: Theme.border
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: Theme.space16
                anchors.bottomMargin: Theme.space16
                spacing: Theme.space16

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    radius: Theme.radiusMedium
                    color: Theme.primary
                    Text {
                        anchors.centerIn: parent
                        text: "⚡"
                        font.pixelSize: 18
                    }
                }

                Repeater {
                    model: scaffoldRoot.currentNavModel

                    delegate: Rectangle {
                        id: railItem
                        required property var modelData
                        required property int index

                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        radius: Theme.radiusMedium
                        color: scaffoldRoot.currentNavIndex === railItem.modelData.id ? Theme.surfaceVariant : "transparent"
                        border.color: scaffoldRoot.currentNavIndex === railItem.modelData.id ? Theme.primary : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: railItem.modelData.icon
                            font.pixelSize: 20
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                scaffoldRoot.currentNavIndex = railItem.modelData.id
                                scaffoldRoot.navIndexChanged(railItem.modelData.id)
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                CustomButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: "⏻"
                    variant: "outline"
                    implicitWidth: 40
                    implicitHeight: 40
                    onClicked: scaffoldRoot.logoutClicked()
                }
            }
        }

        // 3. Central Application Viewport & Mobile Bottom Navigation
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Header Bar (Visible on mobile and tablet, or for unauthenticated state)
            HeaderBar {
                id: topBar
                objectName: "topBar"
                visible: Responsive.isCompact || !scaffoldRoot.authService || !scaffoldRoot.authService.isLoggedIn
                Layout.fillWidth: true
                authService: scaffoldRoot.authService
                onBrandClicked: scaffoldRoot.brandClicked()
                onLogoutClicked: scaffoldRoot.logoutClicked()
            }

            // Dynamic Content Slot
            Item {
                id: contentSlot
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
            }

            // Mobile Bottom Navigation Bar (< 600dp)
            Rectangle {
                id: bottomNav
                objectName: "bottomNav"
                visible: Responsive.isCompact && (scaffoldRoot.authService ? scaffoldRoot.authService.isLoggedIn : false)
                Layout.fillWidth: true
                implicitHeight: Responsive.bottomNavHeight
                color: Theme.surface
                border.color: Theme.border
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    Repeater {
                        model: scaffoldRoot.currentNavModel

                        delegate: Item {
                            id: bNavDelegate
                            required property var modelData
                            required property int index

                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2

                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: bNavDelegate.modelData.icon
                                    font.pixelSize: 18
                                }
                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: bNavDelegate.modelData.label
                                    font.pixelSize: Theme.fontSmall
                                    font.weight: scaffoldRoot.currentNavIndex === bNavDelegate.modelData.id ? Font.Bold : Font.Normal
                                    color: scaffoldRoot.currentNavIndex === bNavDelegate.modelData.id ? Theme.primary : Theme.textSecondary
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    scaffoldRoot.currentNavIndex = bNavDelegate.modelData.id
                                    scaffoldRoot.navIndexChanged(bNavDelegate.modelData.id)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
