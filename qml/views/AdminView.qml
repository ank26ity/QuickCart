pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"
import "../responsive"

Item {
    id: adminView

    property var shopModel: null
    property var orderModel: null
    property var networkManager: null
    property string activeTab: "overview" // "overview", "users", "shops"

    Component.onCompleted: {
        if (adminView.shopModel) adminView.shopModel.fetchShops()
        if (adminView.orderModel) adminView.orderModel.fetchOrders()
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width - Responsive.gutter * 2
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: Responsive.gutter
            spacing: Responsive.gutter

            // Admin Header & Sub-Navigation Tabs
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space12

                ColumnLayout {
                    spacing: Theme.space2
                    RowLayout {
                        spacing: Theme.space8
                        Text {
                            text: "🛡️ Administrator Console"
                            font.pixelSize: Theme.fontTitle
                            font.weight: Font.Bold
                            color: Theme.textPrimary
                        }
                        Rectangle {
                            Layout.preferredWidth: 64
                            Layout.preferredHeight: 22
                            radius: Theme.radiusSmall
                            color: Qt.rgba(Theme.danger.r, Theme.danger.g, Theme.danger.b, 0.2)
                            border.color: Theme.danger
                            Text {
                                anchors.centerIn: parent
                                text: "ROOT"
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                color: Theme.danger
                            }
                        }
                    }
                    Text {
                        text: "Platform analytics, role permissions, and merchant infrastructure"
                        font.pixelSize: Theme.fontSmall
                        color: Theme.textSecondary
                    }
                }

                Item { Layout.fillWidth: true }

                // Desktop / Tablet Tab Switcher
                RowLayout {
                    spacing: Theme.space8

                    CustomButton {
                        text: "Overview"
                        variant: adminView.activeTab === "overview" ? "primary" : "outline"
                        onClicked: adminView.activeTab = "overview"
                    }
                    CustomButton {
                        text: "Users"
                        variant: adminView.activeTab === "users" ? "primary" : "outline"
                        onClicked: adminView.activeTab = "users"
                    }
                    CustomButton {
                        text: "Shops"
                        variant: adminView.activeTab === "shops" ? "primary" : "outline"
                        onClicked: adminView.activeTab = "shops"
                    }
                }
            }

            // Overview Tab: System KPI Metrics (1 to 4 columns based on screen width)
            GridLayout {
                Layout.fillWidth: true
                columns: Responsive.isCompact ? 1 : (Responsive.isMedium ? 2 : 4)
                rowSpacing: Responsive.gutter
                columnSpacing: Responsive.gutter

                // Metric 1: Partner Shops
                GlassCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    cardRadius: Theme.radiusMedium

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: Theme.space4
                        Text { text: "🏪 Partner Stores"; font.pixelSize: Theme.fontSmall; color: Theme.textMuted; Layout.alignment: Qt.AlignHCenter }
                        Text {
                            text: adminView.shopModel ? adminView.shopModel.count.toString() : "0"
                            font.pixelSize: Theme.fontDisplay
                            font.weight: Font.Bold
                            color: Theme.primary
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Metric 2: Active Orders
                GlassCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    cardRadius: Theme.radiusMedium

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: Theme.space4
                        Text { text: "📦 In-Flight Orders"; font.pixelSize: Theme.fontSmall; color: Theme.textMuted; Layout.alignment: Qt.AlignHCenter }
                        Text {
                            text: adminView.orderModel ? adminView.orderModel.count.toString() : "0"
                            font.pixelSize: Theme.fontDisplay
                            font.weight: Font.Bold
                            color: Theme.warning
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Metric 3: Gross Platform GMV
                GlassCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    cardRadius: Theme.radiusMedium

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: Theme.space4
                        Text { text: "💳 Gross GMV"; font.pixelSize: Theme.fontSmall; color: Theme.textMuted; Layout.alignment: Qt.AlignHCenter }
                        Text {
                            text: "₹" + (adminView.orderModel ? (adminView.orderModel.count * 250).toFixed(2) : "0.00")
                            font.pixelSize: Theme.fontDisplay
                            font.weight: Font.Bold
                            color: Theme.secondary
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Metric 4: Platform Health
                GlassCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    cardRadius: Theme.radiusMedium

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: Theme.space4
                        Text { text: "⚡ System Latency"; font.pixelSize: Theme.fontSmall; color: Theme.textMuted; Layout.alignment: Qt.AlignHCenter }
                        Text {
                            text: "14ms"
                            font.pixelSize: Theme.fontDisplay
                            font.weight: Font.Bold
                            color: Theme.primary
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }
            }

            // User Management Section
            GlassCard {
                Layout.fillWidth: true
                visible: adminView.activeTab === "users" || adminView.activeTab === "overview"
                cardRadius: Theme.radiusLarge

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.space20
                    spacing: Theme.space16

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "👤 Registered Platform Users & RBAC Provisioning"
                            font.pixelSize: Theme.fontSubheading
                            font.weight: Font.Bold
                            color: Theme.textPrimary
                        }
                        Item { Layout.fillWidth: true }
                    }

                    // Form Fields (Responsive Flow / Grid)
                    GridLayout {
                        Layout.fillWidth: true
                        columns: Responsive.isCompact ? 1 : (Responsive.isMedium ? 2 : 4)
                        rowSpacing: Theme.space8
                        columnSpacing: Theme.space8

                        CustomTextField { id: adminNewName; Layout.fillWidth: true; placeholderText: "Full Name" }
                        CustomTextField { id: adminNewEmail; Layout.fillWidth: true; placeholderText: "Email Address" }
                        CustomTextField { id: adminNewPhone; Layout.fillWidth: true; placeholderText: "Phone Number" }
                        ComboBox {
                            id: adminNewRole
                            Layout.fillWidth: true
                            model: ["customer", "shopkeeper", "delivery", "admin"]
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.fillWidth: true }

                        CustomButton {
                            text: "+ Provision New User"
                            variant: "primary"
                            onClicked: {
                                var payload = {
                                    "name": adminNewName.text,
                                    "email": adminNewEmail.text,
                                    "phone": adminNewPhone.text,
                                    "role": adminNewRole.currentText,
                                    "password": "Password123"
                                }
                                if (adminView.networkManager) {
                                    adminView.networkManager.post("/api/admin/users", payload, function(ok, doc, err) {
                                        adminNewName.text = ""
                                        adminNewEmail.text = ""
                                        adminNewPhone.text = ""
                                    })
                                }
                            }
                        }
                    }
                }
            }

            // Shop Management & Geofencing Section
            GlassCard {
                Layout.fillWidth: true
                visible: adminView.activeTab === "shops" || adminView.activeTab === "overview"
                cardRadius: Theme.radiusLarge

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.space20
                    spacing: Theme.space16

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "🏬 Store Infrastructure & GPS Coordinates"
                            font.pixelSize: Theme.fontSubheading
                            font.weight: Font.Bold
                            color: Theme.textPrimary
                        }
                        Item { Layout.fillWidth: true }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: Responsive.isCompact ? 1 : (Responsive.isMedium ? 2 : 4)
                        rowSpacing: Theme.space8
                        columnSpacing: Theme.space8

                        CustomTextField { id: adminShopName; Layout.fillWidth: true; placeholderText: "Store Trade Name" }
                        ComboBox {
                            id: adminShopCat
                            Layout.fillWidth: true
                            model: ["grocery", "restaurant", "pharmacy", "electronics"]
                        }
                        CustomTextField { id: adminShopLat; Layout.fillWidth: true; text: "28.6139"; placeholderText: "Latitude" }
                        CustomTextField { id: adminShopLng; Layout.fillWidth: true; text: "77.2090"; placeholderText: "Longitude" }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.fillWidth: true }

                        CustomButton {
                            text: "+ Register Partner Store"
                            variant: "primary"
                            onClicked: {
                                var payload = {
                                    "name": adminShopName.text,
                                    "category": adminShopCat.currentText,
                                    "latitude": parseFloat(adminShopLat.text),
                                    "longitude": parseFloat(adminShopLng.text)
                                }
                                if (adminView.networkManager) {
                                    adminView.networkManager.post("/api/admin/shops", payload, function(ok, doc, err) {
                                        if (adminView.shopModel) adminView.shopModel.fetchShops()
                                        adminShopName.text = ""
                                    })
                                }
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: Responsive.gutter }
        }
    }
}
