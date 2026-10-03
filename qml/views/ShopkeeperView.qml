pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"
import "../responsive"

Item {
    id: shopkeeperView

    property var orderModel: null
    property var shopModel: null
    property var networkManager: null
    property string activeMobileTab: "orders" // "orders" or "store"

    Component.onCompleted: {
        if (shopkeeperView.orderModel) shopkeeperView.orderModel.fetchOrders()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Responsive.gutter
        spacing: Responsive.gutter

        // Header with metrics and mobile tab switcher
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space12

            ColumnLayout {
                spacing: Theme.space2
                Text {
                    text: "🏪 Merchant Operations Hub"
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.Bold
                    color: Theme.textPrimary
                }
                Text {
                    text: "Manage store catalog, inventory, and live order fulfillment"
                    font.pixelSize: Theme.fontSmall
                    color: Theme.textSecondary
                }
            }

            Item { Layout.fillWidth: true }

            // Mobile Tab Switcher (Visible only on compact screens)
            RowLayout {
                visible: Responsive.isCompact
                spacing: Theme.space8

                CustomButton {
                    text: "Live Orders"
                    variant: shopkeeperView.activeMobileTab === "orders" ? "primary" : "outline"
                    onClicked: shopkeeperView.activeMobileTab = "orders"
                }

                CustomButton {
                    text: "Store Info"
                    variant: shopkeeperView.activeMobileTab === "store" ? "primary" : "outline"
                    onClicked: shopkeeperView.activeMobileTab = "store"
                }
            }
        }

        // Main Responsive Content Area
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Responsive.gutter

            // ── Left Column: Store Profile & Inventory Management ──────────────
            // On desktop: always visible; on mobile: visible when activeMobileTab === "store"
            GlassCard {
                id: storeProfileCard
                visible: !Responsive.isCompact || shopkeeperView.activeMobileTab === "store"
                Layout.preferredWidth: Responsive.isDesktop ? Math.min(shopkeeperView.width * 0.38, 420) : -1
                Layout.fillWidth: Responsive.isCompact
                Layout.fillHeight: true

                ScrollView {
                    id: storeProfileScrollView
                    anchors.fill: storeProfileCard
                    anchors.margins: Theme.space16
                    clip: true

                    ColumnLayout {
                        width: storeProfileScrollView.width
                        spacing: Theme.space16

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "🏪 Store Profile"
                                font.pixelSize: Theme.fontHeading
                                font.weight: Font.Bold
                                color: Theme.textPrimary
                            }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                Layout.preferredWidth: 64
                                Layout.preferredHeight: 24
                                radius: Theme.radiusSmall
                                color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.2)
                                border.color: Theme.primary
                                Text {
                                    anchors.centerIn: parent
                                    text: "OPEN"
                                    color: Theme.primary
                                    font.pixelSize: Theme.fontSmall
                                    font.weight: Font.Bold
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true; spacing: Theme.space4
                            Text { text: "Store Name"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                            CustomTextField { id: storeNameField; Layout.fillWidth: true; placeholderText: "Organic Fresh Mart" }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true; spacing: Theme.space4
                            Text { text: "Category"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                            ComboBox {
                                id: storeCatCombo
                                Layout.fillWidth: true
                                model: ["Grocery", "Restaurant", "Pharmacy", "Electronics"]
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true; spacing: Theme.space4
                            Text { text: "Address"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                            CustomTextField { id: storeAddrField; Layout.fillWidth: true; placeholderText: "Street 42, Sector 12" }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true; spacing: Theme.space4
                            Text { text: "Store Description"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                            CustomTextField { id: storeDescField; Layout.fillWidth: true; placeholderText: "Fresh organic food & snacks" }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true; spacing: Theme.space4
                            Text { text: "Banner Image URL"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                            CustomTextField { id: storeBannerField; Layout.fillWidth: true; placeholderText: "https://images.unsplash.com/..." }
                        }

                        CustomButton {
                            Layout.fillWidth: true
                            text: "Update Profile (GPS Verify)"
                            variant: "primary"
                            onClicked: {
                                const payload = {
                                    "name": storeNameField.text,
                                    "category": storeCatCombo.currentText,
                                    "address": storeAddrField.text,
                                    "description": storeDescField.text,
                                    "image": storeBannerField.text,
                                    "latitude": 28.6139,
                                    "longitude": 77.2090
                                }
                                if (shopkeeperView.networkManager) {
                                    shopkeeperView.networkManager.post("/api/shops", payload, function(ok, doc, err) {
                                        if (shopkeeperView.shopModel) shopkeeperView.shopModel.fetchShops()
                                    })
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: Theme.divider
                        }

                        // Merchant Key Performance Indicators
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space8

                            Text {
                                text: "Operational Stats"
                                font.pixelSize: Theme.fontSubheading
                                font.weight: Font.DemiBold
                                color: Theme.textPrimary
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space8

                                GlassCard {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 64
                                    ColumnLayout {
                                        anchors.centerIn: parent; spacing: 2
                                        Text { text: "Rating"; font.pixelSize: Theme.fontSmall; color: Theme.textMuted }
                                        Text { text: "★ 4.9"; font.pixelSize: Theme.fontSubheading; font.weight: Font.Bold; color: Theme.warning }
                                    }
                                }

                                GlassCard {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 64
                                    ColumnLayout {
                                        anchors.centerIn: parent; spacing: 2
                                        Text { text: "Avg Prep"; font.pixelSize: Theme.fontSmall; color: Theme.textMuted }
                                        Text { text: "8 mins"; font.pixelSize: Theme.fontSubheading; font.weight: Font.Bold; color: Theme.primary }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ── Right Column: Order Fulfillment Console ─────────────────────────
            // On desktop: always visible; on mobile: visible when activeMobileTab === "orders"
            ColumnLayout {
                visible: !Responsive.isCompact || shopkeeperView.activeMobileTab === "orders"
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.space16

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "📦 Live Order Queue"
                        font.pixelSize: Theme.fontHeading
                        font.weight: Font.Bold
                        color: Theme.textPrimary
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: (shopkeeperView.orderModel ? shopkeeperView.orderModel.count : 0) + " orders awaiting fulfillment"
                        font.pixelSize: Theme.fontSmall
                        color: Theme.textMuted
                    }
                }

                // Active Orders List View
                ListView {
                    id: merchantOrdersList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: shopkeeperView.orderModel
                    spacing: Theme.space8

                    delegate: Item {
                        id: orderDelegateItem
                        width: merchantOrdersList.width
                        implicitHeight: orderCard.implicitHeight + 8

                        required property string orderId
                        required property string address
                        required property var totalPaise
                        required property string status

                        GlassCard {
                            id: orderCard
                            width: parent.width
                            implicitHeight: orderInnerLayout.implicitHeight + 24
                            strokeColor: Theme.border

                            ColumnLayout {
                                id: orderInnerLayout
                                anchors.fill: parent
                                anchors.margins: Theme.space12
                                spacing: Theme.space12

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: Theme.space12

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: Theme.space4

                                        Text {
                                            text: "Order #" + orderDelegateItem.orderId
                                            font.pixelSize: Theme.fontSubheading
                                            font.weight: Font.Bold
                                            color: Theme.textPrimary
                                        }
                                        Text {
                                            text: "Deliver to: " + orderDelegateItem.address
                                            font.pixelSize: Theme.fontSmall
                                            color: Theme.textMuted
                                            wrapMode: Text.WordWrap
                                            Layout.fillWidth: true
                                        }
                                        Text {
                                            text: "Total: " + Theme.formatPaise(orderDelegateItem.totalPaise)
                                            font.pixelSize: Theme.fontBody
                                            color: Theme.primary
                                            font.weight: Font.Bold
                                        }
                                    }

                                    // Order Status Pill
                                    Rectangle {
                                        id: statusPill
                                        Layout.preferredWidth: 100
                                        Layout.preferredHeight: 32
                                        radius: Theme.radiusSmall
                                        color: {
                                            if (orderDelegateItem.status === "pending") return Qt.rgba(Theme.warning.r, Theme.warning.g, Theme.warning.b, 0.2)
                                            if (orderDelegateItem.status === "preparing") return Qt.rgba(Theme.secondary.r, Theme.secondary.g, Theme.secondary.b, 0.2)
                                            return Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.2)
                                        }
                                        border.color: {
                                            if (orderDelegateItem.status === "pending") return Theme.warning
                                            if (orderDelegateItem.status === "preparing") return Theme.secondary
                                            return Theme.primary
                                        }

                                        Text {
                                            anchors.centerIn: parent
                                            text: orderDelegateItem.status.toUpperCase()
                                            color: {
                                                if (orderDelegateItem.status === "pending") return Theme.warning
                                                if (orderDelegateItem.status === "preparing") return Theme.secondary
                                                return Theme.primary
                                            }
                                            font.pixelSize: Theme.fontSmall
                                            font.weight: Font.Bold
                                        }
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Item { Layout.fillWidth: true }

                                    // Status Action Button
                                    CustomButton {
                                        text: {
                                            if (orderDelegateItem.status === "pending") return "Start Preparing"
                                            if (orderDelegateItem.status === "preparing") return "Mark Ready"
                                            return "Completed"
                                        }
                                        variant: orderDelegateItem.status === "pending" ? "primary" : "secondary"
                                        enabled: orderDelegateItem.status !== "ready" && orderDelegateItem.status !== "delivering" && orderDelegateItem.status !== "delivered"
                                        onClicked: {
                                            const next = orderDelegateItem.status === "pending" ? "preparing" : "ready"
                                            if (shopkeeperView.orderModel) shopkeeperView.orderModel.updateOrderStatus(orderDelegateItem.orderId, next)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
