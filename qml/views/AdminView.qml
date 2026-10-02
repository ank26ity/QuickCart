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

    onShopModelChanged: {
        if (adminView.shopModel) adminView.shopModel.fetchShops()
    }

    onOrderModelChanged: {
        if (adminView.orderModel) adminView.orderModel.fetchOrders()
    }

    Component.onCompleted: {
        if (adminView.shopModel) adminView.shopModel.fetchShops()
        if (adminView.orderModel) adminView.orderModel.fetchOrders()
    }

    Flickable {
        id: adminFlickable
        anchors.fill: parent
        contentWidth: width
        contentHeight: adminLayout.implicitHeight + Responsive.gutter * 2
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        ColumnLayout {
            id: adminLayout
            width: adminFlickable.width - Responsive.gutter * 2
            x: Responsive.gutter
            y: Responsive.gutter
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
                    implicitHeight: 110
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
                    implicitHeight: 110
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
                    implicitHeight: 110
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
                    implicitHeight: 110
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
                id: userCard
                Layout.fillWidth: true
                visible: adminView.activeTab === "users" || adminView.activeTab === "overview"
                implicitHeight: visible ? (userFormLayout.implicitHeight + Theme.space20 * 2) : 0
                cardRadius: Theme.radiusLarge

                ColumnLayout {
                    id: userFormLayout
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
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
                            implicitHeight: 44
                            model: ["customer", "shopkeeper", "delivery", "admin"]
                            contentItem: Text {
                                leftPadding: Theme.space12
                                rightPadding: Theme.space24
                                text: adminNewRole.displayText
                                font.pixelSize: Theme.fontBody
                                color: Theme.textPrimary
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            background: Rectangle {
                                implicitHeight: 44
                                radius: Theme.radiusMedium
                                color: Theme.inputBackground
                                border.color: adminNewRole.activeFocus ? Theme.inputBorderFocus : Theme.inputBorder
                                border.width: 1
                            }
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

            // Platform Users Directory (Shown when on Users tab)
            GlassCard {
                id: usersListCard
                Layout.fillWidth: true
                visible: adminView.activeTab === "users"
                implicitHeight: visible ? (usersLayout.implicitHeight + Theme.space20 * 2) : 0
                cardRadius: Theme.radiusLarge

                ColumnLayout {
                    id: usersLayout
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: Theme.space20
                    spacing: Theme.space12

                    Text {
                        text: "👥 Active User Accounts & Credentials"
                        font.pixelSize: Theme.fontSubheading
                        font.weight: Font.Bold
                        color: Theme.textPrimary
                    }

                    Repeater {
                        model: [
                            { name: "System Administrator", email: "admin@quickcart.com", phone: "+919999900000", role: "ADMIN", status: "Active" },
                            { name: "Bob Merchant", email: "merchant@quickcart.com", phone: "+919876543211", role: "SHOPKEEPER", status: "Active" },
                            { name: "Charlie Courier", email: "delivery@quickcart.com", phone: "+919876543212", role: "DELIVERY", status: "Active" },
                            { name: "Alice Customer", email: "customer@quickcart.com", phone: "+919876543210", role: "CUSTOMER", status: "Active" }
                        ]

                        delegate: Rectangle {
                            id: uRow
                            required property var modelData
                            Layout.fillWidth: true
                            implicitHeight: 52
                            radius: Theme.radiusMedium
                            color: Theme.surfaceVariant
                            border.color: Theme.border
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.space16
                                anchors.rightMargin: Theme.space16
                                spacing: Theme.space12

                                Text {
                                    text: uRow.modelData.role === "ADMIN" ? "🛡️" : (uRow.modelData.role === "SHOPKEEPER" ? "🏪" : (uRow.modelData.role === "DELIVERY" ? "🛵" : "👤"))
                                    font.pixelSize: 18
                                }

                                ColumnLayout {
                                    spacing: 2
                                    Layout.fillWidth: true
                                    Text {
                                        text: uRow.modelData.name
                                        font.pixelSize: Theme.fontBody
                                        font.weight: Font.Bold
                                        color: Theme.textPrimary
                                    }
                                    Text {
                                        text: uRow.modelData.email + " • " + uRow.modelData.phone
                                        font.pixelSize: Theme.fontSmall
                                        color: Theme.textSecondary
                                    }
                                }

                                Rectangle {
                                    Layout.preferredWidth: 92
                                    Layout.preferredHeight: 24
                                    radius: Theme.radiusSmall
                                    color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.15)
                                    border.color: Theme.primary
                                    border.width: 1
                                    Text {
                                        anchors.centerIn: parent
                                        text: uRow.modelData.role
                                        font.pixelSize: 10
                                        font.weight: Font.Bold
                                        color: Theme.primary
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Shop Management & Geofencing Section
            GlassCard {
                id: shopCard
                Layout.fillWidth: true
                visible: adminView.activeTab === "shops" || adminView.activeTab === "overview"
                implicitHeight: visible ? (shopFormLayout.implicitHeight + Theme.space20 * 2) : 0
                cardRadius: Theme.radiusLarge

                ColumnLayout {
                    id: shopFormLayout
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
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
                            implicitHeight: 44
                            model: ["grocery", "restaurant", "pharmacy", "electronics"]
                            contentItem: Text {
                                leftPadding: Theme.space12
                                rightPadding: Theme.space24
                                text: adminShopCat.displayText
                                font.pixelSize: Theme.fontBody
                                color: Theme.textPrimary
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            background: Rectangle {
                                implicitHeight: 44
                                radius: Theme.radiusMedium
                                color: Theme.inputBackground
                                border.color: adminShopCat.activeFocus ? Theme.inputBorderFocus : Theme.inputBorder
                                border.width: 1
                            }
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

            // Partner Stores Directory (Shown when on Shops tab)
            GlassCard {
                id: storesListCard
                Layout.fillWidth: true
                visible: adminView.activeTab === "shops"
                implicitHeight: visible ? (storesLayout.implicitHeight + Theme.space20 * 2) : 0
                cardRadius: Theme.radiusLarge

                ColumnLayout {
                    id: storesLayout
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: Theme.space20
                    spacing: Theme.space12

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "🏪 Active Partner Stores Directory (" + (adminView.shopModel ? adminView.shopModel.count : 0) + ")"
                            font.pixelSize: Theme.fontSubheading
                            font.weight: Font.Bold
                            color: Theme.textPrimary
                        }
                        Item { Layout.fillWidth: true }
                        CustomButton {
                            text: "🔄 Refresh"
                            variant: "outline"
                            implicitHeight: 32
                            onClicked: {
                                if (adminView.shopModel) adminView.shopModel.fetchShops()
                            }
                        }
                    }

                    Repeater {
                        model: adminView.shopModel
                        delegate: Rectangle {
                            id: storeRow
                            required property string name
                            required property string category
                            required property double rating
                            required property string address
                            required property bool isOpen

                            Layout.fillWidth: true
                            implicitHeight: 52
                            radius: Theme.radiusMedium
                            color: Theme.surfaceVariant
                            border.color: Theme.border
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.space16
                                anchors.rightMargin: Theme.space16
                                spacing: Theme.space12

                                Text {
                                    text: "🏬"
                                    font.pixelSize: 18
                                }

                                ColumnLayout {
                                    spacing: 2
                                    Layout.fillWidth: true
                                    Text {
                                        text: storeRow.name
                                        font.pixelSize: Theme.fontBody
                                        font.weight: Font.Bold
                                        color: Theme.textPrimary
                                    }
                                    Text {
                                        text: storeRow.address + " • " + storeRow.category
                                        font.pixelSize: Theme.fontSmall
                                        color: Theme.textSecondary
                                        elide: Text.ElideRight
                                    }
                                }

                                Rectangle {
                                    Layout.preferredWidth: 64
                                    Layout.preferredHeight: 24
                                    radius: Theme.radiusSmall
                                    color: storeRow.isOpen ? Qt.rgba(Theme.success.r, Theme.success.g, Theme.success.b, 0.2) : Qt.rgba(Theme.danger.r, Theme.danger.g, Theme.danger.b, 0.2)
                                    border.color: storeRow.isOpen ? Theme.success : Theme.danger
                                    border.width: 1
                                    Text {
                                        anchors.centerIn: parent
                                        text: storeRow.isOpen ? "OPEN" : "CLOSED"
                                        font.pixelSize: 10
                                        font.weight: Font.Bold
                                        color: storeRow.isOpen ? Theme.success : Theme.danger
                                    }
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
