pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: shopkeeperView

    property var orderModel: null
    property var shopModel: null
    property var networkManager: null

    Component.onCompleted: {
        if (shopkeeperView.orderModel) shopkeeperView.orderModel.fetchOrders()
    }

    RowLayout {
        anchors.fill: shopkeeperView
        anchors.margins: 16
        spacing: 16

        // Left Column: Store Details & Inventory Management
        GlassCard {
            id: storeProfileCard
            Layout.preferredWidth: Math.min(shopkeeperView.width * 0.4, 420)
            Layout.fillHeight: true

            ScrollView {
                id: storeProfileScrollView
                anchors.fill: storeProfileCard
                anchors.margins: 16
                clip: true

                ColumnLayout {
                    width: storeProfileScrollView.width
                    spacing: 16

                    Text { text: "🏪 My Store Profile"; font.pixelSize: 20; font.weight: Font.Bold; color: "#f8fafc" }

                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 4
                        Text { text: "Store Name"; color: "#cbd5e1"; font.pixelSize: 12 }
                        CustomTextField { id: storeNameField; Layout.fillWidth: true; placeholderText: "Organic Fresh Mart" }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 4
                        Text { text: "Category"; color: "#cbd5e1"; font.pixelSize: 12 }
                        ComboBox {
                            id: storeCatCombo
                            Layout.fillWidth: true
                            model: ["Grocery", "Restaurant", "Pharmacy", "Electronics"]
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 4
                        Text { text: "Address"; color: "#cbd5e1"; font.pixelSize: 12 }
                        CustomTextField { id: storeAddrField; Layout.fillWidth: true; placeholderText: "Street 42, Sector 12" }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 4
                        Text { text: "Store Description"; color: "#cbd5e1"; font.pixelSize: 12 }
                        CustomTextField { id: storeDescField; Layout.fillWidth: true; placeholderText: "Fresh organic food & snacks" }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 4
                        Text { text: "Banner Image URL"; color: "#cbd5e1"; font.pixelSize: 12 }
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

                }
            }
        }

        // Right Column: Orders & Product Management Console
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            Text { text: "📦 Order Fulfillment Console"; font.pixelSize: 20; font.weight: Font.Bold; color: "#f8fafc" }

            // Active Orders List View
            ListView {
                id: merchantOrdersList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: shopkeeperView.orderModel

                delegate: Item {
                    id: orderDelegateItem
                    width: merchantOrdersList.width
                    height: 110

                    required property string orderId
                    required property string address
                    required property double total
                    required property string status

                    GlassCard {
                        id: orderCard
                        anchors.fill: orderDelegateItem
                        anchors.bottomMargin: 8
                        strokeColor: "#334155"

                        RowLayout {
                            anchors.fill: orderCard
                            anchors.margins: 16
                            spacing: 16

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Text { text: "Order #" + orderDelegateItem.orderId; font.pixelSize: 15; font.weight: Font.Bold; color: "#f8fafc" }
                                Text { text: "Deliver to: " + orderDelegateItem.address; font.pixelSize: 12; color: "#94a3b8" }
                                Text { text: "Total Amount: ₹" + orderDelegateItem.total.toFixed(2); font.pixelSize: 13; color: "#10b981"; font.weight: Font.Bold }
                            }

                            // Order Status Pill
                            Rectangle {
                                id: statusPill
                                Layout.preferredWidth: 100
                                Layout.preferredHeight: 32
                                radius: 16
                                color: orderDelegateItem.status === "pending" ? Qt.rgba(0.96, 0.62, 0.04, 0.2) : (orderDelegateItem.status === "preparing" ? Qt.rgba(0.38, 0.40, 0.94, 0.2) : Qt.rgba(0.06, 0.72, 0.50, 0.2))
                                border.color: orderDelegateItem.status === "pending" ? "#f59e0b" : (orderDelegateItem.status === "preparing" ? "#6366f1" : "#10b981")

                                Text {
                                    anchors.centerIn: statusPill
                                    text: orderDelegateItem.status.toUpperCase()
                                    color: orderDelegateItem.status === "pending" ? "#f59e0b" : (orderDelegateItem.status === "preparing" ? "#6366f1" : "#10b981")
                                    font.pixelSize: 11
                                    font.weight: Font.Bold
                                }
                            }

                            // Status Action Button
                            CustomButton {
                                text: orderDelegateItem.status === "pending" ? "Start Preparing" : (orderDelegateItem.status === "preparing" ? "Mark Ready" : "Completed")
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

