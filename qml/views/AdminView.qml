pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: adminView

    property var shopModel: null
    property var orderModel: null
    property var networkManager: null
    property string activeTab: "overview" // "overview", "users", "shops", "categories"

    Component.onCompleted: {
        if (adminView.shopModel) adminView.shopModel.fetchShops()
        if (adminView.orderModel) adminView.orderModel.fetchOrders()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // Admin Header & Sub-Navigation Tabs
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text { text: "🛡️ Administrator Console"; font.pixelSize: 22; font.weight: Font.Bold; color: "#f8fafc" }
            Item { Layout.fillWidth: true }

            CustomButton {
                text: "Overview"
                variant: adminView.activeTab === "overview" ? "primary" : "outline"
                onClicked: adminView.activeTab = "overview"
            }
            CustomButton {
                text: "User Management"
                variant: adminView.activeTab === "users" ? "primary" : "outline"
                onClicked: adminView.activeTab = "users"
            }
            CustomButton {
                text: "Shop Management"
                variant: adminView.activeTab === "shops" ? "primary" : "outline"
                onClicked: adminView.activeTab = "shops"
            }
        }

        // Overview Tab: System Metrics
        RowLayout {
            Layout.fillWidth: true
            spacing: 16
            visible: adminView.activeTab === "overview"

            GlassCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 110
                ColumnLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text { text: "Total Partner Shops"; font.pixelSize: 13; color: "#94a3b8"; Layout.alignment: Qt.AlignHCenter }
                    Text { text: adminView.shopModel ? adminView.shopModel.count.toString() : "0"; font.pixelSize: 24; font.weight: Font.Bold; color: "#10b981"; Layout.alignment: Qt.AlignHCenter }
                }
            }
            GlassCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 110
                ColumnLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text { text: "System Active Orders"; font.pixelSize: 13; color: "#94a3b8"; Layout.alignment: Qt.AlignHCenter }
                    Text { text: adminView.orderModel ? adminView.orderModel.count.toString() : "0"; font.pixelSize: 24; font.weight: Font.Bold; color: "#f59e0b"; Layout.alignment: Qt.AlignHCenter }
                }
            }
            GlassCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 110
                ColumnLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text { text: "Gross Platform Revenue"; font.pixelSize: 13; color: "#94a3b8"; Layout.alignment: Qt.AlignHCenter }
                    Text { text: "₹" + (adminView.orderModel ? (adminView.orderModel.count * 250).toFixed(2) : "0.00"); font.pixelSize: 24; font.weight: Font.Bold; color: "#6366f1"; Layout.alignment: Qt.AlignHCenter }
                }
            }
        }

        // User Management Section
        GlassCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: adminView.activeTab === "users" || adminView.activeTab === "overview"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Text { text: "Registered Platform Users & Rider Credentials"; font.pixelSize: 16; font.weight: Font.Bold; color: "#f8fafc" }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomTextField { id: adminNewName; Layout.fillWidth: true; placeholderText: "Name" }
                    CustomTextField { id: adminNewEmail; Layout.fillWidth: true; placeholderText: "Email" }
                    CustomTextField { id: adminNewPhone; Layout.fillWidth: true; placeholderText: "Phone" }
                    ComboBox { id: adminNewRole; Layout.preferredWidth: 130; model: ["customer", "shopkeeper", "delivery", "admin"] }

                    CustomButton {
                        text: "+ Create User"
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
                                    adminNewName.text = ""; adminNewEmail.text = ""; adminNewPhone.text = ""
                                })
                            }
                        }
                    }
                }
            }
        }

        // Shop & Dispatch Section
        GlassCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: adminView.activeTab === "shops"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Text { text: "Shop Management & Custom GPS Location Settings"; font.pixelSize: 16; font.weight: Font.Bold; color: "#f8fafc" }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    CustomTextField { id: adminShopName; Layout.fillWidth: true; placeholderText: "Store Name" }
                    ComboBox { id: adminShopCat; Layout.preferredWidth: 140; model: ["grocery", "restaurant", "pharmacy"] }
                    CustomTextField { id: adminShopLat; Layout.preferredWidth: 100; text: "28.6139"; placeholderText: "Latitude" }
                    CustomTextField { id: adminShopLng; Layout.preferredWidth: 100; text: "77.2090"; placeholderText: "Longitude" }

                    CustomButton {
                        text: "+ Register Store"
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
                                })
                            }
                        }
                    }
                }
            }
        }
    }
}
