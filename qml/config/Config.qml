/**
 * @file Config.qml
 * @brief Configuration proxy singleton exposing runtime parameters and feature flags.
 * @layer Presentation (QML Singleton)
 */

pragma Singleton
pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: root

    // Reference to C++ AppConfig instance
    property var appConfig: null

    readonly property string apiBaseUrl: root.appConfig ? root.appConfig.apiBaseUrl : "http://localhost:5001"
    readonly property string environment: root.appConfig ? root.appConfig.environment : "development"
    readonly property bool isProduction: root.appConfig ? root.appConfig.isProduction : false
    readonly property int requestTimeoutMs: root.appConfig ? root.appConfig.requestTimeoutMs : 10000

    function isFeatureEnabled(featureName: string): bool {
        if (root.appConfig) {
            return root.appConfig.isFeatureEnabled(featureName);
        }
        return false;
    }
}
