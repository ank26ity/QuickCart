pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"
import "../responsive"

Item {
    id: deliveryView

    property var orderModel: null
    property bool isOnline: true
    property string activeOtpOrderId: ""
    property string activeOtpInput: ""

    Component.onCompleted: {
        if (deliveryView.orderModel) deliveryView.orderModel.fetchOrders(deliveryView.isOnline)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Responsive.gutter
        spacing: Responsive.gutter

        // Duty Switch Header Card
        GlassCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 74
            cardRadius: Theme.radiusLarge

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.space16
                spacing: Theme.space16

                ColumnLayout {
                    spacing: Theme.space2
                    Text {
                        text: "COURIER DISPATCH CONSOLE"
                        font.pixelSize: Theme.fontSmall
                        font.weight: Font.Bold
                        color: Theme.textMuted
                    }
                    RowLayout {
                        spacing: Theme.space8
                        Rectangle {
                            Layout.preferredWidth: 10
                            Layout.preferredHeight: 10
                            radius: 5
                            color: deliveryView.isOnline ? Theme.primary : Theme.danger
                        }
                        Text {
                            text: "Duty Status: " + (deliveryView.isOnline ? "ONLINE" : "OFFLINE")
                            font.pixelSize: Theme.fontSubheading
                            font.weight: Font.Bold
                            color: deliveryView.isOnline ? Theme.primary : Theme.danger
                        }
                    }
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

        // Trip Earnings Metrics Grid (Responsive 2 to 3 columns)
        RowLayout {
            Layout.fillWidth: true
            spacing: Responsive.gutter

            GlassCard {
                id: tripEarningsCard
                Layout.fillWidth: true
                Layout.preferredHeight: 88
                cardRadius: Theme.radiusMedium

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: Theme.space4
                    Text { text: "Total Earnings"; font.pixelSize: Theme.fontSmall; color: Theme.textMuted; Layout.alignment: Qt.AlignHCenter }
                    Text {
                        text: "₹" + (deliveryView.orderModel ? (deliveryView.orderModel.count * 50).toFixed(2) : "0.00")
                        font.pixelSize: Theme.fontTitle
                        font.weight: Font.Bold
                        color: Theme.primary
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            GlassCard {
                id: completedDeliveriesCard
                Layout.fillWidth: true
                Layout.preferredHeight: 88
                cardRadius: Theme.radiusMedium

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: Theme.space4
                    Text { text: "Completed Trips"; font.pixelSize: Theme.fontSmall; color: Theme.textMuted; Layout.alignment: Qt.AlignHCenter }
                    Text {
                        text: deliveryView.orderModel ? deliveryView.orderModel.count.toString() : "0"
                        font.pixelSize: Theme.fontTitle
                        font.weight: Font.Bold
                        color: Theme.secondary
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            GlassCard {
                id: acceptanceRateCard
                visible: !Responsive.isCompact
                Layout.fillWidth: true
                Layout.preferredHeight: 88
                cardRadius: Theme.radiusMedium

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: Theme.space4
                    Text { text: "Acceptance Rate"; font.pixelSize: Theme.fontSmall; color: Theme.textMuted; Layout.alignment: Qt.AlignHCenter }
                    Text {
                        text: "98.5%"
                        font.pixelSize: Theme.fontTitle
                        font.weight: Font.Bold
                        color: Theme.warning
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }

        // Section Title & Counter
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "🛵 Active Dispatch & Available Jobs"
                font.pixelSize: Theme.fontHeading
                font.weight: Font.Bold
                color: Theme.textPrimary
            }
            Item { Layout.fillWidth: true }
            Text {
                text: (deliveryView.orderModel ? deliveryView.orderModel.count : 0) + " available in 3km radius"
                font.pixelSize: Theme.fontSmall
                color: Theme.textMuted
            }
        }

        // Jobs Marketplace List View
        ListView {
            id: jobsListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.space8
            model: deliveryView.orderModel

            delegate: Item {
                id: deliveryJobDelegate
                width: jobsListView.width
                implicitHeight: jobCard.implicitHeight + 8

                required property string orderId
                required property string shopName
                required property string address
                required property string status

                GlassCard {
                    id: jobCard
                    width: parent.width
                    implicitHeight: jobInnerLayout.implicitHeight + 24
                    strokeColor: deliveryJobDelegate.status === "delivering" ? Theme.primary : Theme.border
                    cardRadius: Theme.radiusMedium

                    ColumnLayout {
                        id: jobInnerLayout
                        anchors.fill: parent
                        anchors.margins: Theme.space16
                        spacing: Theme.space12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space16

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space4

                                RowLayout {
                                    spacing: Theme.space8
                                    Text {
                                        text: "Order #" + deliveryJobDelegate.orderId
                                        font.pixelSize: Theme.fontSubheading
                                        font.weight: Font.Bold
                                        color: Theme.textPrimary
                                    }
                                    Rectangle {
                                        Layout.preferredWidth: 70
                                        Layout.preferredHeight: 20
                                        radius: Theme.radiusSmall
                                        color: deliveryJobDelegate.status === "delivering" ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.2) : Qt.rgba(Theme.secondary.r, Theme.secondary.g, Theme.secondary.b, 0.2)
                                        border.color: deliveryJobDelegate.status === "delivering" ? Theme.primary : Theme.secondary
                                        Text {
                                            anchors.centerIn: parent
                                            text: deliveryJobDelegate.status === "delivering" ? "EN ROUTE" : "READY"
                                            font.pixelSize: 10
                                            font.weight: Font.Bold
                                            color: deliveryJobDelegate.status === "delivering" ? Theme.primary : Theme.secondary
                                        }
                                    }
                                }

                                Text {
                                    text: "🏬 Pickup: " + deliveryJobDelegate.shopName
                                    font.pixelSize: Theme.fontSmall
                                    color: Theme.textSecondary
                                }
                                Text {
                                    text: "📍 Drop-off: " + deliveryJobDelegate.address
                                    font.pixelSize: Theme.fontSmall
                                    color: Theme.textMuted
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: "Payout: ₹50.00 (Guaranteed)"
                                    font.pixelSize: Theme.fontBody
                                    color: Theme.primary
                                    font.weight: Font.Bold
                                }
                            }

                            // Desktop Action Area
                            ColumnLayout {
                                visible: !Responsive.isCompact
                                spacing: Theme.space8
                                Layout.alignment: Qt.AlignVCenter

                                CustomButton {
                                    text: deliveryJobDelegate.status === "delivering" ? "Enter Delivery OTP" : "Accept Delivery Job"
                                    variant: deliveryJobDelegate.status === "delivering" ? "primary" : "secondary"
                                    onClicked: {
                                        if (deliveryJobDelegate.status === "delivering") {
                                            otpDialog.targetOrderId = deliveryJobDelegate.orderId
                                            otpDialog.open()
                                        } else {
                                            if (deliveryView.orderModel) deliveryView.orderModel.updateOrderStatus(deliveryJobDelegate.orderId, "delivering")
                                        }
                                    }
                                }
                            }
                        }

                        // Mobile Full-Width Action Button
                        CustomButton {
                            visible: Responsive.isCompact
                            Layout.fillWidth: true
                            text: deliveryJobDelegate.status === "delivering" ? "Enter Delivery OTP" : "Accept Delivery Job"
                            variant: deliveryJobDelegate.status === "delivering" ? "primary" : "secondary"
                            onClicked: {
                                if (deliveryJobDelegate.status === "delivering") {
                                    otpDialog.targetOrderId = deliveryJobDelegate.orderId
                                    otpDialog.open()
                                } else {
                                    if (deliveryView.orderModel) deliveryView.orderModel.updateOrderStatus(deliveryJobDelegate.orderId, "delivering")
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // OTP Confirmation Dialog for Secure Delivery Handoff
    Dialog {
        id: otpDialog
        property string targetOrderId: ""
        title: "Verify Delivery OTP"
        modal: true
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 380)

        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
            radius: Theme.radiusLarge
        }

        header: Item {
            implicitHeight: 48
            Text {
                anchors.left: parent.left
                anchors.leftMargin: Theme.space16
                anchors.verticalCenter: parent.verticalCenter
                text: "🔐 Confirm Delivery Handoff"
                font.pixelSize: Theme.fontSubheading
                font.weight: Font.Bold
                color: Theme.textPrimary
            }
        }

        ColumnLayout {
            width: parent.width
            spacing: Theme.space12

            Text {
                text: "Ask customer for the 4-digit verification code to complete Order #" + otpDialog.targetOrderId
                font.pixelSize: Theme.fontSmall
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            CustomTextField {
                id: otpInputField
                Layout.fillWidth: true
                placeholderText: "Enter 4-digit OTP"
                echoMode: TextInput.Normal
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space8

                CustomButton {
                    Layout.fillWidth: true
                    text: "Cancel"
                    variant: "outline"
                    onClicked: {
                        otpInputField.text = ""
                        otpDialog.close()
                    }
                }

                CustomButton {
                    Layout.fillWidth: true
                    text: "Verify & Complete"
                    variant: "primary"
                    enabled: otpInputField.text.length >= 4
                    onClicked: {
                        if (deliveryView.orderModel) {
                            deliveryView.orderModel.updateOrderStatus(otpDialog.targetOrderId, "delivered")
                        }
                        otpInputField.text = ""
                        otpDialog.close()
                    }
                }
            }
        }
    }
}
