import QtQuick
import QtTest
import QuickCart

Item {
    id: root
    width: 400
    height: 300

    TestCase {
        name: "ThemeTokensTestCase"
        when: windowShown

        function initTestCase() {
            Theme.themeManager = themeManager
        }

        function test_theme_color_tokens() {
            verify(Theme.primary.toString() !== "")
            verify(Theme.background.toString() !== "")
            verify(Theme.surface.toString() !== "")
            verify(Theme.textPrimary.toString() !== "")
            verify(Theme.danger.toString() !== "")
            verify(Theme.warning.toString() !== "")
            verify(Theme.success.toString() !== "")
        }

        function test_metrics_tokens() {
            compare(Theme.space8, 8)
            compare(Theme.space16, 16)
            compare(Theme.space24, 24)
            compare(Theme.space32, 32)
            compare(Theme.radiusSmall, 6)
            compare(Theme.radiusMedium, 10)
            compare(Theme.radiusLarge, 14)
        }

        function test_contrast_tokens() {
            verify(Theme.textPrimaryBrand.toString() !== "")
            verify(Theme.textDanger.toString() !== "")
            verify(Theme.textWarning.toString() !== "")
            verify(Theme.textSuccess.toString() !== "")
            verify(Theme.textDisabled.toString() !== "")
            verify(Theme.surfaceDisabled.toString() !== "")
        }

        function test_live_dark_light_switch() {
            Theme.setMode("dark")
            var darkBg = Theme.background.toString()
            Theme.setMode("light")
            var lightBg = Theme.background.toString()
            verify(darkBg !== lightBg)
            // Reset to dark
            Theme.setMode("dark")
        }
    }
}
