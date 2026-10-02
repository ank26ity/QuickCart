pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: authView

    property var authService: null
    property string mode: "login" // "login" or "signup"
    property string selectedRole: "customer" // "customer", "shopkeeper", "delivery"

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: Math.min(parent.width - 32, 480)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 40
            spacing: 24

            GlassCard {
                Layout.fillWidth: true
                implicitHeight: cardContent.implicitHeight + 48
                cardRadius: 16

                ColumnLayout {
                    id: cardContent
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 20

                    // Title
                    ColumnLayout {
                        spacing: 6
                        Text {
                            text: authView.mode === "login" ? "Welcome to QuickShopp" : "Join QuickShopp Ecosystem"
                            font.pixelSize: 22
                            font.weight: Font.Bold
                            color: "#f8fafc"
                        }
                        Text {
                            text: authView.mode === "login" ? "Log in to order fresh meals, groceries, and more." : "Create an account to start buying, selling, or delivering."
                            font.pixelSize: 13
                            color: "#94a3b8"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }

                    // Tabs
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        CustomButton {
                            Layout.fillWidth: true
                            text: "Log In"
                            variant: authView.mode === "login" ? "primary" : "outline"
                            onClicked: authView.mode = "login"
                        }
                        CustomButton {
                            Layout.fillWidth: true
                            text: "Sign Up"
                            variant: authView.mode === "signup" ? "primary" : "outline"
                            onClicked: authView.mode = "signup"
                        }
                    }

                    // Error Box
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: errText.implicitHeight + 16
                        visible: authView.authService ? authView.authService.errorMessage !== "" : false
                        color: Qt.rgba(0.93, 0.26, 0.26, 0.15)
                        border.color: "#ef4444"
                        radius: 8

                        Text {
                            id: errText
                            anchors.centerIn: parent
                            width: parent.width - 24
                            text: authView.authService ? authView.authService.errorMessage : ""
                            color: "#ef4444"
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                        }
                    }

                    // Form Fields
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 14

                        // Full Name (Signup only)
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: authView.mode === "signup"
                            spacing: 4
                            Text { text: "Full Name"; color: "#cbd5e1"; font.pixelSize: 13; font.weight: Font.DemiBold }
                            CustomTextField {
                                id: nameField
                                Layout.fillWidth: true
                                placeholderText: "Enter your full name"
                            }
                        }

                        // Identifier / Email
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text {
                                text: authView.mode === "login" ? "Email or Mobile Number" : "Email Address"
                                color: "#cbd5e1"
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                            }
                            CustomTextField {
                                id: identifierField
                                Layout.fillWidth: true
                                placeholderText: authView.mode === "login" ? "name@domain.com or +919876543210" : "name@domain.com"
                            }
                        }

                        // Password
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text { text: "Password"; color: "#cbd5e1"; font.pixelSize: 13; font.weight: Font.DemiBold }
                            CustomTextField {
                                id: passwordField
                                Layout.fillWidth: true
                                echoMode: TextInput.Password
                                placeholderText: "••••••••"
                            }
                        }

                        // Mobile Phone (Signup only)
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: authView.mode === "signup"
                            spacing: 4
                            Text { text: "Mobile Phone Number"; color: "#cbd5e1"; font.pixelSize: 13; font.weight: Font.DemiBold }
                            CustomTextField {
                                id: phoneField
                                Layout.fillWidth: true
                                placeholderText: "+919876543210"
                            }
                        }

                        // Role Selector Cards (Signup only)
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: authView.mode === "signup"
                            spacing: 8
                            Text { text: "Register As"; color: "#cbd5e1"; font.pixelSize: 13; font.weight: Font.DemiBold }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                // Customer Card
                                GlassCard {
                                    Layout.fillWidth: true
                                    implicitHeight: 70
                                    strokeColor: authView.selectedRole === "customer" ? "#10b981" : "#1e293b"
                                    color: authView.selectedRole === "customer" ? Qt.rgba(0.06, 0.72, 0.50, 0.15) : Qt.rgba(0.06, 0.09, 0.16, 0.6)

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: authView.selectedRole = "customer"
                                    }

                                    ColumnLayout {
                                        anchors.centerIn: parent
                                        spacing: 2
                                        Text { text: "🛒"; font.pixelSize: 20; Layout.alignment: Qt.AlignHCenter }
                                        Text { text: "Customer"; color: "#f8fafc"; font.pixelSize: 12; font.weight: Font.DemiBold; Layout.alignment: Qt.AlignHCenter }
                                    }
                                }

                                // Shopkeeper Card
                                GlassCard {
                                    Layout.fillWidth: true
                                    implicitHeight: 70
                                    strokeColor: authView.selectedRole === "shopkeeper" ? "#f59e0b" : "#1e293b"
                                    color: authView.selectedRole === "shopkeeper" ? Qt.rgba(0.96, 0.62, 0.04, 0.15) : Qt.rgba(0.06, 0.09, 0.16, 0.6)

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: authView.selectedRole = "shopkeeper"
                                    }

                                    ColumnLayout {
                                        anchors.centerIn: parent
                                        spacing: 2
                                        Text { text: "🏪"; font.pixelSize: 20; Layout.alignment: Qt.AlignHCenter }
                                        Text { text: "Merchant"; color: "#f8fafc"; font.pixelSize: 12; font.weight: Font.DemiBold; Layout.alignment: Qt.AlignHCenter }
                                    }
                                }

                                // Delivery Rider Card
                                GlassCard {
                                    Layout.fillWidth: true
                                    implicitHeight: 70
                                    strokeColor: authView.selectedRole === "delivery" ? "#6366f1" : "#1e293b"
                                    color: authView.selectedRole === "delivery" ? Qt.rgba(0.38, 0.40, 0.94, 0.15) : Qt.rgba(0.06, 0.09, 0.16, 0.6)

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: authView.selectedRole = "delivery"
                                    }

                                    ColumnLayout {
                                        anchors.centerIn: parent
                                        spacing: 2
                                        Text { text: "🛵"; font.pixelSize: 20; Layout.alignment: Qt.AlignHCenter }
                                        Text { text: "Rider"; color: "#f8fafc"; font.pixelSize: 12; font.weight: Font.DemiBold; Layout.alignment: Qt.AlignHCenter }
                                    }
                                }
                            }
                        }

                        // Rider Compliance Fields
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: authView.mode === "signup" && authView.selectedRole === "delivery"
                            spacing: 12

                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#334155" }
                            Text { text: "Rider Compliance Documents"; color: "#f8fafc"; font.pixelSize: 14; font.weight: Font.Bold }

                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 4
                                Text { text: "Driving License Number"; color: "#cbd5e1"; font.pixelSize: 12 }
                                CustomTextField { id: licenseNumField; Layout.fillWidth: true; placeholderText: "e.g. DL-142010001234" }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 4
                                Text { text: "License Photo URL"; color: "#cbd5e1"; font.pixelSize: 12 }
                                CustomTextField { id: licensePhotoField; Layout.fillWidth: true; placeholderText: "https://image-host.com/license.jpg" }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 4
                                Text { text: "Vehicle Type"; color: "#cbd5e1"; font.pixelSize: 12 }
                                ComboBox {
                                    id: vehicleTypeCombo
                                    Layout.fillWidth: true
                                    model: ["bicycle", "scooter", "motorcycle", "auto", "car"]
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 4
                                visible: vehicleTypeCombo.currentText !== "bicycle"
                                Text { text: "RC (Registration Certificate) Number"; color: "#cbd5e1"; font.pixelSize: 12 }
                                CustomTextField { id: rcNumField; Layout.fillWidth: true; placeholderText: "e.g. DL-3C-AB-1234" }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 4
                                visible: vehicleTypeCombo.currentText !== "bicycle"
                                Text { text: "RC Photo URL"; color: "#cbd5e1"; font.pixelSize: 12 }
                                CustomTextField { id: rcPhotoField; Layout.fillWidth: true; placeholderText: "https://image-host.com/rc.jpg" }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 4
                                Text { text: "Vehicle Plate Number"; color: "#cbd5e1"; font.pixelSize: 12 }
                                CustomTextField { id: vehicleNumField; Layout.fillWidth: true; placeholderText: "e.g. DL 3C AB 1234" }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 4
                                Text { text: "Vehicle Photo URL"; color: "#cbd5e1"; font.pixelSize: 12 }
                                CustomTextField { id: vehiclePhotoField; Layout.fillWidth: true; placeholderText: "https://image-host.com/vehicle.jpg" }
                            }
                        }

                        // Submit Button
                        CustomButton {
                            Layout.fillWidth: true
                            implicitHeight: 48
                            text: authView.mode === "login" ? "Sign In" : "Complete Registration"
                            enabled: authView.authService ? !authView.authService.isLoading : true
                            onClicked: {
                                if (authView.mode === "login") {
                                    if (authView.authService) authView.authService.login(identifierField.text, passwordField.text)
                                } else {
                                    const data = {
                                        "name": nameField.text,
                                        "email": identifierField.text,
                                        "password": passwordField.text,
                                        "phone": phoneField.text,
                                        "role": authView.selectedRole,
                                        "licenseNumber": licenseNumField.text,
                                        "licensePhoto": licensePhotoField.text,
                                        "vehicleType": vehicleTypeCombo.currentText,
                                        "rcNumber": rcNumField.text,
                                        "rcPhoto": rcPhotoField.text,
                                        "vehicleNumber": vehicleNumField.text,
                                        "vehiclePhoto": vehiclePhotoField.text
                                    }
                                    if (authView.authService) authView.authService.signup(data)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
