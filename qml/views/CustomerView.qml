pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import "../components"
import "../theme"
import "../responsive"

Item {
    id: customerView

    property var shopModel: null
    property var productModel: null
    property var cartManager: null
    property var orderModel: null

    property string currentCategory: "all"
    property var selectedShopData: null
    property double userLat: 28.6139
    property double userLng: 77.2090
    property bool mobileCartOpen: false

    Component.onCompleted: {
        if (customerView.shopModel) customerView.shopModel.fetchShops(currentCategory, userLat, userLng)
        if (customerView.orderModel) customerView.orderModel.fetchOrders()
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Responsive.gutter
        spacing: Responsive.gutter

        // Left Side: Main Content (Store Grid or Product Menu)
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.space16

            // GPS Location Selector Bar
            GlassCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 56

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.space12
                    spacing: Theme.space12

                    Text { text: "📍 GPS:"; color: Theme.primary; font.pixelSize: Theme.fontBody; font.weight: Font.Bold }

                    CustomButton {
                        text: "Central Delhi"
                        variant: (customerView.userLat === 28.6139 && customerView.userLng === 77.2090) ? "primary" : "outline"
                        implicitHeight: 34
                        onClicked: {
                            customerView.userLat = 28.6139
                            customerView.userLng = 77.2090
                            if (customerView.shopModel) customerView.shopModel.fetchShops(customerView.currentCategory, 28.6139, 77.2090)
                        }
                    }

                    CustomButton {
                        text: "Bangalore (BLR)"
                        variant: (customerView.userLat === 12.9716 && customerView.userLng === 77.5946) ? "primary" : "outline"
                        implicitHeight: 34
                        onClicked: {
                            customerView.userLat = 12.9716
                            customerView.userLng = 77.5946
                            if (customerView.shopModel) customerView.shopModel.fetchShops(customerView.currentCategory, 12.9716, 77.5946)
                        }
                    }

                    CustomButton {
                        text: "South Delhi"
                        variant: (customerView.userLat === 28.5750 && customerView.userLng === 77.2150) ? "primary" : "outline"
                        implicitHeight: 34
                        onClicked: {
                            customerView.userLat = 28.5750
                            customerView.userLng = 77.2150
                            if (customerView.shopModel) customerView.shopModel.fetchShops(customerView.currentCategory, 28.5750, 77.2150)
                        }
                    }

                    CustomButton {
                        text: "Noida (>3km)"
                        variant: (customerView.userLat === 28.5708 && customerView.userLng === 77.3261) ? "primary" : "outline"
                        implicitHeight: 34
                        visible: !Responsive.isCompact
                        onClicked: {
                            customerView.userLat = 28.5708
                            customerView.userLng = 77.3261
                            if (customerView.shopModel) customerView.shopModel.fetchShops(customerView.currentCategory, 28.5708, 77.3261)
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            // Header Controls & Category Tabs
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space8
                visible: customerView.selectedShopData === null

                CustomButton {
                    text: "All Shops"
                    variant: customerView.currentCategory === "all" ? "primary" : "outline"
                    onClicked: {
                        customerView.currentCategory = "all"
                        if (customerView.shopModel) customerView.shopModel.fetchShops("all", customerView.userLat, customerView.userLng)
                    }
                }
                CustomButton {
                    text: "Grocery"
                    variant: customerView.currentCategory === "Grocery" ? "primary" : "outline"
                    onClicked: {
                        customerView.currentCategory = "Grocery"
                        if (customerView.shopModel) customerView.shopModel.fetchShops("Grocery", customerView.userLat, customerView.userLng)
                    }
                }
                CustomButton {
                    text: "Restaurants"
                    variant: customerView.currentCategory === "Restaurants" ? "primary" : "outline"
                    onClicked: {
                        customerView.currentCategory = "Restaurants"
                        if (customerView.shopModel) customerView.shopModel.fetchShops("Restaurants", customerView.userLat, customerView.userLng)
                    }
                }
                CustomButton {
                    text: "Pharmacy"
                    variant: customerView.currentCategory === "Pharmacy" ? "primary" : "outline"
                    visible: !Responsive.isCompact
                    onClicked: {
                        customerView.currentCategory = "Pharmacy"
                        if (customerView.shopModel) customerView.shopModel.fetchShops("Pharmacy", customerView.userLat, customerView.userLng)
                    }
                }

                Item { Layout.fillWidth: true }
            }

            // Breadcrumb / Back button when browsing a specific shop's menu
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space12
                visible: customerView.selectedShopData !== null

                CustomButton {
                    text: "← Back to Stores"
                    variant: "outline"
                    onClicked: {
                        customerView.selectedShopData = null
                    }
                }

                Text {
                    text: customerView.selectedShopData ? customerView.selectedShopData.name : ""
                    font.pixelSize: Theme.fontHeading
                    font.weight: Font.Bold
                    color: Theme.textPrimary
                }

                Item { Layout.fillWidth: true }
            }

            // View Switcher (Stores vs Products)
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // 1. Loading State Card
                GlassCard {
                    anchors.fill: parent
                    visible: customerView.selectedShopData === null && customerView.shopModel && customerView.shopModel.isLoading

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: Theme.space16

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            implicitWidth: 48
                            implicitHeight: 48
                            radius: 24
                            color: Qt.rgba(0.06, 0.72, 0.50, 0.15)
                            border.color: Theme.primary
                            border.width: 2

                            Text {
                                anchors.centerIn: parent
                                text: "⚡"
                                font.pixelSize: 22
                            }

                            RotationAnimator on rotation {
                                from: 0
                                to: 360
                                duration: Theme.durationSlow * 3
                                loops: Animation.Infinite
                                running: customerView.shopModel && customerView.shopModel.isLoading
                            }
                        }

                        Text {
                            text: "Discovering Nearby Partner Stores..."
                            font.pixelSize: Theme.fontSubheading
                            font.weight: Font.DemiBold
                            color: Theme.textPrimary
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // 2. Empty State Card
                GlassCard {
                    anchors.fill: parent
                    visible: customerView.selectedShopData === null && customerView.shopModel && !customerView.shopModel.isLoading && customerView.shopModel.count === 0

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: Theme.space12
                        width: Math.min(parent.width - 40, 480)

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            implicitWidth: 64
                            implicitHeight: 64
                            radius: 32
                            color: Qt.rgba(0.96, 0.62, 0.04, 0.15)
                            border.color: Theme.warning
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: "📍"
                                font.pixelSize: 32
                            }
                        }

                        Text {
                            text: "No Partner Stores Found"
                            font.pixelSize: Theme.fontHeading
                            font.weight: Font.Bold
                            color: Theme.textPrimary
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Text {
                            text: "There are no active partner stores for this category within your 3.0 km radius."
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontBody
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // 3. Adaptive Shops Grid View
                GridView {
                    id: shopGridView
                    anchors.fill: parent
                    cellWidth: Math.max(260, Math.floor(shopGridView.width / Math.max(1, Responsive.columns)))
                    cellHeight: 250
                    clip: true
                    visible: customerView.selectedShopData === null && customerView.shopModel && !customerView.shopModel.isLoading && customerView.shopModel.count > 0
                    model: customerView.shopModel

                    delegate: Item {
                        id: shopDelegate
                        width: shopGridView.cellWidth - Theme.space8
                        height: shopGridView.cellHeight - Theme.space8

                        required property int index
                        required property string name
                        required property string image
                        required property string description
                        required property double rating
                        required property double distance

                        GlassCard {
                            anchors.fill: parent
                            strokeColor: shopMouseArea.containsMouse ? Theme.primary : Theme.border

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: Theme.space12
                                spacing: Theme.space8

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 110
                                    radius: Theme.radiusMedium
                                    clip: true
                                    color: Theme.surfaceVariant

                                    Image {
                                        anchors.fill: parent
                                        source: shopDelegate.image
                                        fillMode: Image.PreserveAspectCrop
                                    }

                                    // Rating & Distance Badges
                                    Rectangle {
                                        anchors.top: parent.top
                                        anchors.right: parent.right
                                        anchors.margins: 8
                                        width: 52
                                        height: 24
                                        radius: 6
                                        color: Qt.rgba(0, 0, 0, 0.75)

                                        RowLayout {
                                            anchors.centerIn: parent
                                            spacing: 4
                                            Text { text: "★"; color: "#fbbf24"; font.pixelSize: 11 }
                                            Text { text: shopDelegate.rating.toFixed(1); color: "#ffffff"; font.pixelSize: 11; font.weight: Font.Bold }
                                        }
                                    }
                                }

                                Text {
                                    text: shopDelegate.name
                                    font.pixelSize: Theme.fontBody
                                    font.weight: Font.Bold
                                    color: Theme.textPrimary
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: shopDelegate.description
                                    font.pixelSize: Theme.fontSmall
                                    color: Theme.textSecondary
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: "⚡ " + (shopDelegate.distance > 0 ? (shopDelegate.distance.toFixed(1) + " km away") : "Nearby")
                                        font.pixelSize: Theme.fontSmall
                                        color: Theme.primary
                                    }
                                    Item { Layout.fillWidth: true }
                                    CustomButton {
                                        text: "View Menu"
                                        variant: "primary"
                                        implicitHeight: 30
                                        onClicked: {
                                            if (customerView.shopModel) {
                                                customerView.selectedShopData = customerView.shopModel.getShopAt(shopDelegate.index)
                                                if (customerView.productModel) {
                                                    customerView.productModel.fetchProductsForShop(customerView.selectedShopData.id)
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            MouseArea {
                                id: shopMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (customerView.shopModel) {
                                        customerView.selectedShopData = customerView.shopModel.getShopAt(shopDelegate.index)
                                        if (customerView.productModel) {
                                            customerView.productModel.fetchProductsForShop(customerView.selectedShopData.id)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // 4. Products Catalog Grid View
                GridView {
                    id: productGridView
                    anchors.fill: parent
                    cellWidth: Math.max(260, Math.floor(productGridView.width / Math.max(1, Responsive.columns)))
                    cellHeight: 280
                    clip: true
                    visible: customerView.selectedShopData !== null && customerView.productModel && !customerView.productModel.isLoading && customerView.productModel.count > 0
                    model: customerView.productModel

                    delegate: Item {
                        id: prodDelegate
                        width: productGridView.cellWidth - Theme.space8
                        height: productGridView.cellHeight - Theme.space8

                        required property int index
                        required property string name
                        required property string image
                        required property string description
                        required property double price
                        required property int quantity
                        required property bool isLowStock
                        required property bool isOutOfStock

                        GlassCard {
                            anchors.fill: parent
                            strokeColor: Theme.border

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: Theme.space12
                                spacing: Theme.space8

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 120
                                    radius: Theme.radiusMedium
                                    clip: true
                                    color: Theme.surfaceVariant

                                    Image {
                                        anchors.fill: parent
                                        source: prodDelegate.image
                                        fillMode: Image.PreserveAspectCrop
                                    }

                                    // Stock badge
                                    Rectangle {
                                        visible: prodDelegate.isLowStock || prodDelegate.isOutOfStock
                                        anchors.top: parent.top
                                        anchors.left: parent.left
                                        anchors.margins: 8
                                        width: stockText.implicitWidth + 12
                                        height: 22
                                        radius: 4
                                        color: prodDelegate.isOutOfStock ? Theme.danger : Theme.warning

                                        Text {
                                            id: stockText
                                            anchors.centerIn: parent
                                            text: prodDelegate.isOutOfStock ? "Out of Stock" : "Only " + prodDelegate.quantity + " left"
                                            color: prodDelegate.isOutOfStock ? Theme.onDanger : Theme.onWarning
                                            font.pixelSize: 10
                                            font.weight: Font.Bold
                                        }
                                    }
                                }

                                Text {
                                    text: prodDelegate.name
                                    font.pixelSize: Theme.fontBody
                                    font.weight: Font.Bold
                                    color: Theme.textPrimary
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: "₹" + prodDelegate.price.toFixed(2)
                                        font.pixelSize: Theme.fontSubheading
                                        font.weight: Font.Bold
                                        color: Theme.primary
                                    }
                                    Item { Layout.fillWidth: true }
                                    CustomButton {
                                        text: "Add"
                                        variant: "primary"
                                        implicitHeight: 32
                                        enabled: !prodDelegate.isOutOfStock
                                        onClicked: {
                                            if (customerView.cartManager && customerView.productModel) {
                                                var p = customerView.productModel.getProductAt(prodDelegate.index)
                                                customerView.cartManager.addItem(p, prodDelegate.quantity)
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

        // Right Side: Docked Cart Drawer (Hidden on compact phones, shown on tablet/desktop)
        GlassCard {
            id: cartPanel
            visible: !Responsive.isCompact
            Layout.preferredWidth: Responsive.isDesktop ? 360 : 300
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.space16
                spacing: Theme.space16

                Text {
                    text: "🛒 Shopping Cart"
                    font.pixelSize: Theme.fontHeading
                    font.weight: Font.Bold
                    color: Theme.textPrimary
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: customerView.cartManager ? customerView.cartManager.items : null

                    delegate: GlassCard {
                        id: cartDelegate
                        required property var modelData
                        width: ListView.view ? ListView.view.width : 280
                        implicitHeight: 64
                        strokeColor: Theme.border

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: cartDelegate.modelData.name; font.pixelSize: 13; color: Theme.textPrimary; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                Text { text: "₹" + (cartDelegate.modelData.price * cartDelegate.modelData.quantity).toFixed(2); font.pixelSize: 12; color: Theme.primary; font.weight: Font.Bold }
                            }

                            RowLayout {
                                spacing: 4
                                CustomButton {
                                    text: "-"
                                    variant: "outline"
                                    implicitWidth: 28
                                    implicitHeight: 28
                                    onClicked: {
                                        if (customerView.cartManager) customerView.cartManager.updateQuantity(cartDelegate.modelData.productId, -1)
                                    }
                                }
                                Text { text: cartDelegate.modelData.quantity; color: Theme.textPrimary; font.pixelSize: 13; font.weight: Font.Bold }
                                CustomButton {
                                    text: "+"
                                    variant: "outline"
                                    implicitWidth: 28
                                    implicitHeight: 28
                                    onClicked: {
                                        if (customerView.cartManager) customerView.cartManager.updateQuantity(cartDelegate.modelData.productId, 1)
                                    }
                                }
                            }
                        }
                    }
                }

                // Summary
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.space8
                    visible: customerView.cartManager && customerView.cartManager.itemCount > 0

                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Subtotal"; color: Theme.textSecondary; font.pixelSize: Theme.fontBody }
                        Item { Layout.fillWidth: true }
                        Text { text: "₹" + (customerView.cartManager ? customerView.cartManager.subtotal.toFixed(2) : "0.00"); color: Theme.textPrimary; font.pixelSize: Theme.fontBody }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Delivery Fee"; color: Theme.textSecondary; font.pixelSize: Theme.fontBody }
                        Item { Layout.fillWidth: true }
                        Text { text: "₹" + (customerView.cartManager ? customerView.cartManager.deliveryFee.toFixed(2) : "0.00"); color: Theme.textPrimary; font.pixelSize: Theme.fontBody }
                    }
                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.divider }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Total"; color: Theme.textPrimary; font.pixelSize: Theme.fontSubheading; font.weight: Font.Bold }
                        Item { Layout.fillWidth: true }
                        Text { text: "₹" + (customerView.cartManager ? customerView.cartManager.total.toFixed(2) : "0.00"); color: Theme.primary; font.pixelSize: Theme.fontHeading; font.weight: Font.Bold }
                    }

                    CustomTextField {
                        id: checkoutAddressField
                        Layout.fillWidth: true
                        placeholderText: "Delivery street address"
                    }

                    CustomButton {
                        Layout.fillWidth: true
                        text: "Place Order Now"
                        variant: "primary"
                        enabled: customerView.cartManager && !customerView.cartManager.isSubmitting
                        onClicked: {
                            if (customerView.cartManager) customerView.cartManager.placeOrder(checkoutAddressField.text)
                        }
                    }
                }
            }
        }
    }

    // ── Mobile Sticky Cart Bar & Bottom Sheet Drawer (< 600dp) ─────────────
    Rectangle {
        id: mobileStickyCartBar
        visible: Responsive.isCompact && customerView.cartManager && customerView.cartManager.itemCount > 0 && !customerView.mobileCartOpen
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Theme.space12
        height: 52
        radius: Theme.radiusLarge
        color: Theme.primary

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.space16
            anchors.rightMargin: Theme.space16

            Text {
                text: "🛒 " + (customerView.cartManager ? customerView.cartManager.itemCount : 0) + " items"
                color: Theme.onPrimary
                font.pixelSize: Theme.fontBody
                font.weight: Font.Bold
            }
            Item { Layout.fillWidth: true }
            Text {
                text: "₹" + (customerView.cartManager ? customerView.cartManager.total.toFixed(2) : "0.00") + "  ➔"
                color: Theme.onPrimary
                font.pixelSize: Theme.fontBody
                font.weight: Font.Bold
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: { customerView.mobileCartOpen = true }
        }
    }

    // Mobile Cart Full Drawer
    Rectangle {
        id: mobileCartDrawer
        visible: Responsive.isCompact && customerView.mobileCartOpen
        anchors.fill: parent
        color: Theme.background
        z: 99

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.space16
            spacing: Theme.space16

            RowLayout {
                Layout.fillWidth: true
                Text { text: "🛒 Your Order"; font.pixelSize: Theme.fontHeading; font.weight: Font.Bold; color: Theme.textPrimary }
                Item { Layout.fillWidth: true }
                CustomButton {
                    text: "✕ Close"
                    variant: "outline"
                    implicitHeight: 32
                    onClicked: { customerView.mobileCartOpen = false }
                }
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: customerView.cartManager ? customerView.cartManager.items : null

                delegate: GlassCard {
                    id: mCartDelegate
                    required property var modelData
                    width: ListView.view ? ListView.view.width : 280
                    implicitHeight: 64
                    strokeColor: Theme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: mCartDelegate.modelData.name; font.pixelSize: 13; color: Theme.textPrimary; font.weight: Font.DemiBold; elide: Text.ElideRight }
                            Text { text: "₹" + (mCartDelegate.modelData.price * mCartDelegate.modelData.quantity).toFixed(2); font.pixelSize: 12; color: Theme.primary; font.weight: Font.Bold }
                        }

                        RowLayout {
                            spacing: 4
                            CustomButton {
                                text: "-"
                                variant: "outline"
                                implicitWidth: 28
                                implicitHeight: 28
                                onClicked: {
                                    if (customerView.cartManager) customerView.cartManager.updateQuantity(mCartDelegate.modelData.productId, -1)
                                }
                            }
                            Text { text: mCartDelegate.modelData.quantity; color: Theme.textPrimary; font.pixelSize: 13; font.weight: Font.Bold }
                            CustomButton {
                                text: "+"
                                variant: "outline"
                                implicitWidth: 28
                                implicitHeight: 28
                                onClicked: {
                                    if (customerView.cartManager) customerView.cartManager.updateQuantity(mCartDelegate.modelData.productId, 1)
                                }
                            }
                        }
                    }
                }
            }

            // Keyboard-aware bottom form
            Flickable {
                Layout.fillWidth: true
                Layout.preferredHeight: 180
                contentHeight: mCheckoutCol.implicitHeight
                clip: true

                ColumnLayout {
                    id: mCheckoutCol
                    width: parent.width
                    spacing: Theme.space8

                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Total Payable"; font.pixelSize: Theme.fontSubheading; font.weight: Font.Bold; color: Theme.textPrimary }
                        Item { Layout.fillWidth: true }
                        Text { text: "₹" + (customerView.cartManager ? customerView.cartManager.total.toFixed(2) : "0.00"); color: Theme.primary; font.pixelSize: Theme.fontHeading; font.weight: Font.Bold }
                    }

                    CustomTextField {
                        id: mAddressField
                        Layout.fillWidth: true
                        placeholderText: "Delivery street address"
                    }

                    CustomButton {
                        Layout.fillWidth: true
                        text: "Confirm & Place Order"
                        variant: "primary"
                        enabled: customerView.cartManager && !customerView.cartManager.isSubmitting
                        onClicked: {
                            if (customerView.cartManager) {
                                customerView.cartManager.placeOrder(mAddressField.text)
                                customerView.mobileCartOpen = false
                            }
                        }
                    }
                }
            }
        }
    }
}
