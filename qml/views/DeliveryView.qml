pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: deliveryView

    property var orderModel: null
    property bool isOnline: true

    Component.onCompleted: {
        if (deliveryView.orderModel) deliveryView.orderModel.fetchOrders(deliveryView.isOnline)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // Duty Switch Header Card
        GlassCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 70

            RowLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 16

                ColumnLayout {
                    spacing: 2
                    Text { text: "COURIER DISPATCH SYSTEM"; font.pixelSize: 11; font.weight: Font.Bold; color: "#94a3b8" }
                    Text { text: "Duty Status: " + (deliveryView.isOnline ? "ONLINE" : "OFFLINE"); font.pixelSize: 16; font.weight: Font.Bold; color: deliveryView.isOnline ? "#10b981" : "#ef4444" }
                }

                Item { Layout.fillWidth: true }

                Switch {
                    id: dutySwitch
                    checked: deliveryView.isOnline
                    onCheckedChanged: {
                        deliveryView.isOnline = dutySwitch.checked
                        if (deliveryView.orderModel) deliveryView.orderModel.fetchOrders(dutySwitch.checked)
                    }
                }
            }
        }

        // Trip Earnings Metrics Grid
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            GlassCard {
                id: tripEarningsCard
                Layout.fillWidth: true
                Layout.preferredHeight: 90

                ColumnLayout {
                    anchors.centerIn: tripEarningsCard
                    spacing: 4
                    Text { text: "Total Trip Earnings"; font.pixelSize: 12; color: "#94a3b8"; Layout.alignment: Qt.AlignHCenter }
                    Text { text: "₹" + (deliveryView.orderModel ? (deliveryView.orderModel.count * 50).toFixed(2) : "0.00"); font.pixelSize: 22; font.weight: Font.Bold; color: "#10b981"; Layout.alignment: Qt.AlignHCenter }
                }
            }

            GlassCard {
                id: completedDeliveriesCard
                Layout.fillWidth: true
                Layout.preferredHeight: 90

                ColumnLayout {
                    anchors.centerIn: completedDeliveriesCard
                    spacing: 4
                    Text { text: "Completed Deliveries"; font.pixelSize: 12; color: "#94a3b8"; Layout.alignment: Qt.AlignHCenter }
                    Text { text: deliveryView.orderModel ? deliveryView.orderModel.count.toString() : "0"; font.pixelSize: 22; font.weight: Font.Bold; color: "#6366f1"; Layout.alignment: Qt.AlignHCenter }
                }
            }
        }

        Text { text: "🛵 Available Ready Delivery Jobs"; font.pixelSize: 18; font.weight: Font.Bold; color: "#f8fafc" }

        // Jobs Marketplace List View
        ListView {
            id: jobsListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: deliveryView.orderModel

            delegate: Item {
                id: deliveryJobDelegate
                width: jobsListView.width
                height: 120

                required property string orderId
                required property string shopName
                required property string address
                required property string status

                GlassCard {
                    anchors.fill: parent
                    anchors.bottomMargin: 8
                    strokeColor: "#334155"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text { text: "Order #" + deliveryJobDelegate.orderId; font.pixelSize: 16; font.weight: Font.Bold; color: "#f8fafc" }
                            Text { text: "Pickup Store: " + deliveryJobDelegate.shopName; font.pixelSize: 13; color: "#94a3b8" }
                            Text { text: "Delivery Address: " + deliveryJobDelegate.address; font.pixelSize: 13; color: "#cbd5e1" }
                            Text { text: "Trip Earnings: ₹50.00"; font.pixelSize: 13; color: "#10b981"; font.weight: Font.Bold }
                        }

                        CustomButton {
                            text: deliveryJobDelegate.status === "delivering" ? "Mark Delivered" : "Accept Delivery Job"
                            variant: deliveryJobDelegate.status === "delivering" ? "primary" : "secondary"
                            onClicked: {
                                const nextStatus = deliveryJobDelegate.status === "delivering" ? "delivered" : "delivering"
                                if (deliveryView.orderModel) deliveryView.orderModel.updateOrderStatus(deliveryJobDelegate.orderId, nextStatus)
                            }
                        }

                    }
                }
            }
        }
    }
}

