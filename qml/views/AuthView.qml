pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"
import "../responsive"

Item {
    id: authView

    property var authService: null
    property string mode: "login" // "login" or "signup"
    property string selectedRole: "customer" // "customer", "shopkeeper", "delivery"

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Item {
            width: Math.min(parent.width, 1100)
            anchors.horizontalCenter: parent.horizontalCenter
            implicitHeight: mainLayout.implicitHeight + (Responsive.isDesktop ? 80 : 40)

            RowLayout {
                id: mainLayout
                width: parent.width - (Responsive.isDesktop ? 64 : 32)
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: Responsive.isDesktop ? 40 : 20
                spacing: Responsive.isDesktop ? 48 : 0

                // Desktop Branding / Hero Column
                GlassCard {
                    id: desktopHeroCard
                    visible: Responsive.isDesktop
                    Layout.fillWidth: true
                    Layout.preferredHeight: authCard.implicitHeight
                    cardRadius: Theme.radiusLarge

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.space32
                        spacing: Theme.space24

                        RowLayout {
                            spacing: Theme.space12
                            Rectangle {
                                Layout.preferredWidth: 44
                                Layout.preferredHeight: 44
                                radius: Theme.radiusMedium
                                color: Theme.primary
                                Text {
                                    anchors.centerIn: parent
                                    text: "⚡"
                                    font.pixelSize: 22
                                }
                            }
                            ColumnLayout {
                                spacing: 2
                                Text {
                                    text: "QuickShopp"
                                    font.pixelSize: Theme.fontTitle
                                    font.weight: Font.Bold
                                    color: Theme.textPrimary
                                }
                                Text {
                                    text: "Hyperlocal Delivery Ecosystem"
                                    font.pixelSize: Theme.fontSmall
                                    color: Theme.textSecondary
                                }
                            }
                        }

                        Text {
                            text: "Ultra-fast hyperlocal commerce connecting customers, merchants, and couriers with guaranteed 10-minute dispatch."
                            font.pixelSize: Theme.fontBody
                            color: Theme.textSecondary
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: Theme.divider
                        }

                        // Feature Highlights
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space16

                            RowLayout {
                                spacing: Theme.space12
                                Text { text: "🚀"; font.pixelSize: 20 }
                                ColumnLayout {
                                    spacing: 0
                                    Text { text: "Sub-10 Min Order Processing"; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold; color: Theme.textPrimary }
                                    Text { text: "Real-time state machines synchronize dispatch in milliseconds"; font.pixelSize: Theme.fontSmall; color: Theme.textMuted }
                                }
                            }

                            RowLayout {
                                spacing: Theme.space12
                                Text { text: "🏪"; font.pixelSize: 20 }
                                ColumnLayout {
                                    spacing: 0
                                    Text { text: "Direct Merchant Integration"; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold; color: Theme.textPrimary }
                                    Text { text: "Live catalog & inventory management for neighborhood stores"; font.pixelSize: Theme.fontSmall; color: Theme.textMuted }
                                }
                            }

                            RowLayout {
                                spacing: Theme.space12
                                Text { text: "🛡️"; font.pixelSize: 20 }
                                ColumnLayout {
                                    spacing: 0
                                    Text { text: "Secure OTP Delivery Handoff"; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold; color: Theme.textPrimary }
                                    Text { text: "4-digit cryptographic verification protects customer orders"; font.pixelSize: Theme.fontSmall; color: Theme.textMuted }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        Text {
                            text: "🔒 Enterprise-grade security with encrypted local credentials"
                            font.pixelSize: Theme.fontSmall
                            color: Theme.textMuted
                        }
                    }
                }

                // Auth Form Card (Responsive 440px max width)
                GlassCard {
                    id: authCard
                    Layout.preferredWidth: Math.min(parent.width, 440)
                    Layout.alignment: Responsive.isDesktop ? Qt.AlignVCenter : Qt.AlignHCenter
                    implicitHeight: cardContent.implicitHeight + 48
                    cardRadius: Theme.radiusLarge

                    ColumnLayout {
                        id: cardContent
                        anchors.fill: parent
                        anchors.margins: Theme.space24
                        spacing: Theme.space20

                        // Title
                        ColumnLayout {
                            spacing: Theme.space4
                            Text {
                                text: authView.mode === "login" ? "Welcome to QuickShopp" : "Join QuickShopp Ecosystem"
                                font.pixelSize: Theme.fontTitle
                                font.weight: Font.Bold
                                color: Theme.textPrimary
                            }
                            Text {
                                text: authView.mode === "login" ? "Log in to order fresh meals, groceries, and more." : "Create an account to start buying, selling, or delivering."
                                font.pixelSize: Theme.fontSmall
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        // Tabs
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space8

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
                            color: Qt.rgba(Theme.danger.r, Theme.danger.g, Theme.danger.b, 0.15)
                            border.color: Theme.danger
                            radius: Theme.radiusSmall

                            Text {
                                id: errText
                                anchors.centerIn: parent
                                width: parent.width - 24
                                text: authView.authService ? authView.authService.errorMessage : ""
                                color: Theme.danger
                                font.pixelSize: Theme.fontSmall
                                wrapMode: Text.WordWrap
                            }
                        }

                        // Form Fields
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space12

                            // Full Name (Signup only)
                            ColumnLayout {
                                Layout.fillWidth: true
                                visible: authView.mode === "signup"
                                spacing: Theme.space4
                                Text { text: "Full Name"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall; font.weight: Font.DemiBold }
                                CustomTextField {
                                    id: nameField
                                    Layout.fillWidth: true
                                    placeholderText: "Enter your full name"
                                }
                            }

                            // Identifier / Email
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space4
                                Text {
                                    text: authView.mode === "login" ? "Email or Mobile Number" : "Email Address"
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSmall
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
                                spacing: Theme.space4
                                Text { text: "Password"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall; font.weight: Font.DemiBold }
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
                                spacing: Theme.space4
                                Text { text: "Mobile Phone Number"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall; font.weight: Font.DemiBold }
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
                                spacing: Theme.space8
                                Text { text: "Register As"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall; font.weight: Font.DemiBold }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: Theme.space8

                                    // Customer Card
                                    GlassCard {
                                        Layout.fillWidth: true
                                        implicitHeight: 70
                                        strokeColor: authView.selectedRole === "customer" ? Theme.primary : Theme.border
                                        color: authView.selectedRole === "customer" ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.15) : Theme.surfaceGlass

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: authView.selectedRole = "customer"
                                        }

                                        ColumnLayout {
                                            anchors.centerIn: parent
                                            spacing: 2
                                            Text { text: "🛒"; font.pixelSize: 20; Layout.alignment: Qt.AlignHCenter }
                                            Text { text: "Customer"; color: Theme.textPrimary; font.pixelSize: Theme.fontSmall; font.weight: Font.DemiBold; Layout.alignment: Qt.AlignHCenter }
                                        }
                                    }

                                    // Shopkeeper Card
                                    GlassCard {
                                        Layout.fillWidth: true
                                        implicitHeight: 70
                                        strokeColor: authView.selectedRole === "shopkeeper" ? Theme.warning : Theme.border
                                        color: authView.selectedRole === "shopkeeper" ? Qt.rgba(Theme.warning.r, Theme.warning.g, Theme.warning.b, 0.15) : Theme.surfaceGlass

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: authView.selectedRole = "shopkeeper"
                                        }

                                        ColumnLayout {
                                            anchors.centerIn: parent
                                            spacing: 2
                                            Text { text: "🏪"; font.pixelSize: 20; Layout.alignment: Qt.AlignHCenter }
                                            Text { text: "Merchant"; color: Theme.textPrimary; font.pixelSize: Theme.fontSmall; font.weight: Font.DemiBold; Layout.alignment: Qt.AlignHCenter }
                                        }
                                    }

                                    // Delivery Rider Card
                                    GlassCard {
                                        Layout.fillWidth: true
                                        implicitHeight: 70
                                        strokeColor: authView.selectedRole === "delivery" ? Theme.secondary : Theme.border
                                        color: authView.selectedRole === "delivery" ? Qt.rgba(Theme.secondary.r, Theme.secondary.g, Theme.secondary.b, 0.15) : Theme.surfaceGlass

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: authView.selectedRole = "delivery"
                                        }

                                        ColumnLayout {
                                            anchors.centerIn: parent
                                            spacing: 2
                                            Text { text: "🛵"; font.pixelSize: 20; Layout.alignment: Qt.AlignHCenter }
                                            Text { text: "Rider"; color: Theme.textPrimary; font.pixelSize: Theme.fontSmall; font.weight: Font.DemiBold; Layout.alignment: Qt.AlignHCenter }
                                        }
                                    }
                                }
                            }

                            // Rider Compliance Fields
                            ColumnLayout {
                                Layout.fillWidth: true
                                visible: authView.mode === "signup" && authView.selectedRole === "delivery"
                                spacing: Theme.space12

                                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.divider }
                                Text { text: "Rider Compliance Documents"; color: Theme.textPrimary; font.pixelSize: Theme.fontBody; font.weight: Font.Bold }

                                ColumnLayout {
                                    Layout.fillWidth: true; spacing: Theme.space4
                                    Text { text: "Driving License Number"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                                    CustomTextField { id: licenseNumField; Layout.fillWidth: true; placeholderText: "e.g. DL-142010001234" }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true; spacing: Theme.space4
                                    Text { text: "License Photo URL"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                                    CustomTextField { id: licensePhotoField; Layout.fillWidth: true; placeholderText: "https://image-host.com/license.jpg" }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true; spacing: Theme.space4
                                    Text { text: "Vehicle Type"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                                    ComboBox {
                                        id: vehicleTypeCombo
                                        Layout.fillWidth: true
                                        model: ["bicycle", "scooter", "motorcycle", "auto", "car"]
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true; spacing: Theme.space4
                                    visible: vehicleTypeCombo.currentText !== "bicycle"
                                    Text { text: "RC (Registration Certificate) Number"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                                    CustomTextField { id: rcNumField; Layout.fillWidth: true; placeholderText: "e.g. DL-3C-AB-1234" }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true; spacing: Theme.space4
                                    visible: vehicleTypeCombo.currentText !== "bicycle"
                                    Text { text: "RC Photo URL"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                                    CustomTextField { id: rcPhotoField; Layout.fillWidth: true; placeholderText: "https://image-host.com/rc.jpg" }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true; spacing: Theme.space4
                                    Text { text: "Vehicle Plate Number"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                                    CustomTextField { id: vehicleNumField; Layout.fillWidth: true; placeholderText: "e.g. DL 3C AB 1234" }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true; spacing: Theme.space4
                                    Text { text: "Vehicle Photo URL"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
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
}
