cmake_minimum_required(VERSION 3.16)
project(QuickCart VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)
option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

if(ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
        message(STATUS "Enabling AddressSanitizer and UndefinedBehaviorSanitizer")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
        message(STATUS "Enabling code coverage instrumentation")
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)
    endif()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    QuickControls2
    Network
    Concurrent
    QuickTest
)

if(ANDROID)
    include(FetchContent)
    FetchContent_Declare(
        android_openssl
        URL "https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip"
    )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
    set(SECURITY_LIBS "")
else()
    find_package(OpenSSL REQUIRED)
    set(SECURITY_LIBS OpenSSL::Crypto)
endif()

if(APPLE)
    list(APPEND SECURITY_LIBS "-framework Security" "-framework CoreFoundation")
endif()

if(COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)
endif()

set(PROJECT_SOURCES
    main.cpp
    core/result.h
    core/logging.h
    core/logging.cpp
    core/appconfig.h
    core/appconfig.cpp
    core/thememanager.h
    core/thememanager.cpp
    core/validators.h
    core/validators.cpp
    core/orderstatemachine.h
    core/orderstatemachine.cpp
    security/securestorage.h
    security/securestorage.cpp
    security/permissionmanager.h
    security/permissionmanager.cpp
    api/networkinterceptor.h
    api/circuitbreaker.h
    api/circuitbreaker.cpp
    api/networkmanager.h
    api/networkmanager.cpp
    api/apiclient.h
    api/apiclient.cpp
    models/authservice.h
    models/authservice.cpp
    models/shopmodel.h
    models/shopmodel.cpp
    models/productmodel.h
    models/productmodel.cpp
    models/cartmanager.h
    models/cartmanager.cpp
    models/ordermodel.h
    models/ordermodel.cpp
)

qt_add_executable(QuickCart ${PROJECT_SOURCES})
set_target_properties(QuickCart PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "QuickCart"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.quickshopp.quickcart"
    OUTPUT_NAME "QuickCartApp"
)

# Declare singletons with QT_QML_SINGLETON_TYPE
set_source_files_properties(
    qml/theme/Theme.qml
    qml/theme/Metrics.qml
    qml/responsive/Responsive.qml
    qml/navigation/Router.qml
    qml/i18n/Strings.qml
    qml/icons/Icons.qml
    qml/config/Config.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(QuickCart
    URI QuickCart
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/theme/Theme.qml
        qml/theme/Metrics.qml
        qml/responsive/Responsive.qml
        qml/navigation/Router.qml
        qml/i18n/Strings.qml
        qml/icons/Icons.qml
        qml/config/Config.qml
        qml/components/HeaderBar.qml
        qml/components/GlassCard.qml
        qml/components/CustomButton.qml
        qml/components/CustomTextField.qml
        qml/components/ThemeSelector.qml
        qml/components/AppScaffold.qml
        qml/views/AuthView.qml
        qml/views/CustomerView.qml
        qml/views/ShopkeeperView.qml
        qml/views/DeliveryView.qml
        qml/views/AdminView.qml
    RESOURCES
        qml/qtquickcontrols2.conf
)

target_link_libraries(QuickCart PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Concurrent
    ${SECURITY_LIBS}
)
if(ANDROID)
    add_android_openssl_libraries(QuickCart)
endif()

target_include_directories(QuickCart PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/security
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/models
)
if(APPLE AND EXISTS "/opt/homebrew/include")
    target_include_directories(QuickCart PRIVATE /opt/homebrew/include)
endif()


include(GNUInstallDirs)
install(TARGETS QuickCart
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ── CTest Test Harness ──────────────────────────────────────────────────────
if(NOT ANDROID)
find_package(Qt6 REQUIRED COMPONENTS Test)
enable_testing()


# 1. Automated WCAG Contrast Test
add_executable(test_contrast tests/cpp/test_contrast.cpp)
target_compile_definitions(test_contrast PRIVATE
    QUICKCART_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(test_contrast PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)
add_test(NAME ContrastTest COMMAND test_contrast)

# 2. Automated ThemeManager Test
add_executable(test_thememanager
    tests/cpp/test_thememanager.cpp
    core/thememanager.cpp
)
target_link_libraries(test_thememanager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)
target_include_directories(test_thememanager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)
add_test(NAME ThemeManagerTest COMMAND test_thememanager)

# 4. Result Monad Test
add_executable(test_result tests/cpp/test_result.cpp)
target_link_libraries(test_result PRIVATE Qt6::Core Qt6::Test)
target_include_directories(test_result PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core)
add_test(NAME ResultTest COMMAND test_result)

# 5. Validators Test
add_executable(test_validators tests/cpp/test_validators.cpp core/validators.cpp)
target_link_libraries(test_validators PRIVATE Qt6::Core Qt6::Test)
target_include_directories(test_validators PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core)
add_test(NAME ValidatorsTest COMMAND test_validators)

# 6. Order State Machine Test
add_executable(test_orderstatemachine tests/cpp/test_orderstatemachine.cpp core/orderstatemachine.cpp)
target_link_libraries(test_orderstatemachine PRIVATE Qt6::Core Qt6::Test)
target_include_directories(test_orderstatemachine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core)
add_test(NAME OrderStateMachineTest COMMAND test_orderstatemachine)

# 7. Permission Manager Test
add_executable(test_permissionmanager
    tests/cpp/test_permissionmanager.cpp
    security/permissionmanager.cpp
    core/logging.cpp
)
target_link_libraries(test_permissionmanager PRIVATE Qt6::Core Qt6::Test)
target_include_directories(test_permissionmanager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core ${CMAKE_CURRENT_SOURCE_DIR}/security)
add_test(NAME PermissionManagerTest COMMAND test_permissionmanager)

# 8. Secure Storage Test
add_executable(test_securestorage
    tests/cpp/test_securestorage.cpp
    security/securestorage.cpp
    core/logging.cpp
)
target_link_libraries(test_securestorage PRIVATE Qt6::Core Qt6::Test ${SECURITY_LIBS})
target_include_directories(test_securestorage PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core ${CMAKE_CURRENT_SOURCE_DIR}/security)
add_test(NAME SecureStorageTest COMMAND test_securestorage)

# 9. ApiClient Test
add_executable(test_apiclient
    tests/cpp/test_apiclient.cpp
    api/apiclient.cpp
    api/networkmanager.cpp
    api/circuitbreaker.cpp
    security/securestorage.cpp
    core/appconfig.cpp
    core/logging.cpp
    tests/tools/mockapiserver.cpp
)
target_link_libraries(test_apiclient PRIVATE Qt6::Core Qt6::Network Qt6::Concurrent Qt6::Test ${SECURITY_LIBS})
target_include_directories(test_apiclient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core ${CMAKE_CURRENT_SOURCE_DIR}/security ${CMAKE_CURRENT_SOURCE_DIR}/api ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools)
add_test(NAME ApiClientTest COMMAND test_apiclient)

# ── Integration Flow Tests ──────────────────────────────────────────────────

# 10. Auth Flow Test (4a)
add_executable(test_auth_flow
    tests/cpp/test_auth_flow.cpp
    models/authservice.cpp
    core/validators.cpp
    api/networkmanager.cpp
    api/circuitbreaker.cpp
    security/securestorage.cpp
    security/permissionmanager.cpp
    core/appconfig.cpp
    core/logging.cpp
    tests/tools/mockapiserver.cpp
)
target_link_libraries(test_auth_flow PRIVATE Qt6::Core Qt6::Network Qt6::Concurrent Qt6::Test ${SECURITY_LIBS})
target_include_directories(test_auth_flow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core ${CMAKE_CURRENT_SOURCE_DIR}/security ${CMAKE_CURRENT_SOURCE_DIR}/api ${CMAKE_CURRENT_SOURCE_DIR}/models ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools)
add_test(NAME AuthFlowTest COMMAND test_auth_flow)

# 11. Customer Cart Flow Test (4b)
add_executable(test_customer_cart_flow
    tests/cpp/test_customer_cart_flow.cpp
    models/cartmanager.cpp
    models/shopmodel.cpp
    api/apiclient.cpp
    api/networkmanager.cpp
    api/circuitbreaker.cpp
    core/validators.cpp
    security/securestorage.cpp
    core/appconfig.cpp
    core/logging.cpp
    tests/tools/mockapiserver.cpp
)
target_link_libraries(test_customer_cart_flow PRIVATE Qt6::Core Qt6::Network Qt6::Concurrent Qt6::Test ${SECURITY_LIBS})
target_include_directories(test_customer_cart_flow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core ${CMAKE_CURRENT_SOURCE_DIR}/security ${CMAKE_CURRENT_SOURCE_DIR}/api ${CMAKE_CURRENT_SOURCE_DIR}/models ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools)
add_test(NAME CustomerCartFlowTest COMMAND test_customer_cart_flow)

# 12. Merchant Flow Test (4c & 4f)
add_executable(test_merchant_flow
    tests/cpp/test_merchant_flow.cpp
    models/ordermodel.cpp
    core/orderstatemachine.cpp
    api/networkmanager.cpp
    api/circuitbreaker.cpp
    security/securestorage.cpp
    core/appconfig.cpp
    core/logging.cpp
    tests/tools/mockapiserver.cpp
)
target_link_libraries(test_merchant_flow PRIVATE Qt6::Core Qt6::Network Qt6::Concurrent Qt6::Test ${SECURITY_LIBS})
target_include_directories(test_merchant_flow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core ${CMAKE_CURRENT_SOURCE_DIR}/security ${CMAKE_CURRENT_SOURCE_DIR}/api ${CMAKE_CURRENT_SOURCE_DIR}/models ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools)
add_test(NAME MerchantFlowTest COMMAND test_merchant_flow)

# 13. Courier Flow Test (4d)
add_executable(test_courier_flow
    tests/cpp/test_courier_flow.cpp
    models/ordermodel.cpp
    core/orderstatemachine.cpp
    api/networkmanager.cpp
    api/circuitbreaker.cpp
    security/securestorage.cpp
    core/appconfig.cpp
    core/logging.cpp
    tests/tools/mockapiserver.cpp
)
target_link_libraries(test_courier_flow PRIVATE Qt6::Core Qt6::Network Qt6::Concurrent Qt6::Test ${SECURITY_LIBS})
target_include_directories(test_courier_flow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core ${CMAKE_CURRENT_SOURCE_DIR}/security ${CMAKE_CURRENT_SOURCE_DIR}/api ${CMAKE_CURRENT_SOURCE_DIR}/models ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools)
add_test(NAME CourierFlowTest COMMAND test_courier_flow)

# 14. Network Resilience Test (4h)
add_executable(test_network_resilience
    tests/cpp/test_network_resilience.cpp
    api/networkmanager.cpp
    api/circuitbreaker.cpp
    security/securestorage.cpp
    core/appconfig.cpp
    core/logging.cpp
    tests/tools/mockapiserver.cpp
)
target_link_libraries(test_network_resilience PRIVATE Qt6::Core Qt6::Network Qt6::Concurrent Qt6::Test ${SECURITY_LIBS})
target_include_directories(test_network_resilience PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core ${CMAKE_CURRENT_SOURCE_DIR}/security ${CMAKE_CURRENT_SOURCE_DIR}/api ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools)
add_test(NAME NetworkResilienceTest COMMAND test_network_resilience)

# 15. Automated Responsive System & UI Offscreen Test
add_executable(test_responsive_ui
    tests/cpp/test_responsive_ui.cpp
    core/thememanager.cpp
)
target_compile_definitions(test_responsive_ui PRIVATE
    QUICKCART_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(test_responsive_ui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::Test
)
target_include_directories(test_responsive_ui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)
add_test(NAME ResponsiveUITest COMMAND test_responsive_ui)

# 16. Product CRUD Test
add_executable(test_product_crud
    tests/cpp/test_product_crud.cpp
    models/productmodel.cpp
    api/networkmanager.cpp
    api/circuitbreaker.cpp
    security/securestorage.cpp
    core/appconfig.cpp
    core/logging.cpp
)
target_link_libraries(test_product_crud PRIVATE Qt6::Core Qt6::Network Qt6::Concurrent Qt6::Test ${SECURITY_LIBS})
target_include_directories(test_product_crud PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core ${CMAKE_CURRENT_SOURCE_DIR}/security ${CMAKE_CURRENT_SOURCE_DIR}/api ${CMAKE_CURRENT_SOURCE_DIR}/models)
add_test(NAME ProductCrudTest COMMAND test_product_crud)

# 17. Delivery OTP Verification Test
add_executable(test_delivery_otp
    tests/cpp/test_delivery_otp.cpp
    core/validators.cpp
    core/orderstatemachine.cpp
    core/logging.cpp
    core/appconfig.cpp
    api/networkmanager.cpp
    api/circuitbreaker.cpp
    security/securestorage.cpp
    tests/tools/mockapiserver.cpp
)
target_link_libraries(test_delivery_otp PRIVATE Qt6::Core Qt6::Network Qt6::Test ${SECURITY_LIBS})
target_include_directories(test_delivery_otp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core ${CMAKE_CURRENT_SOURCE_DIR}/security ${CMAKE_CURRENT_SOURCE_DIR}/api ${CMAKE_CURRENT_SOURCE_DIR}/models)
add_test(NAME DeliveryOtpTest COMMAND test_delivery_otp)

# 18. Location Fallback Test
add_executable(test_location_fallback
    tests/cpp/test_location_fallback.cpp
    core/validators.cpp
    models/shopmodel.cpp
    api/networkmanager.cpp
    api/circuitbreaker.cpp
    security/securestorage.cpp
    core/appconfig.cpp
    core/logging.cpp
)
target_link_libraries(test_location_fallback PRIVATE Qt6::Core Qt6::Network Qt6::Concurrent Qt6::Test ${SECURITY_LIBS})
target_include_directories(test_location_fallback PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core ${CMAKE_CURRENT_SOURCE_DIR}/security ${CMAKE_CURRENT_SOURCE_DIR}/api ${CMAKE_CURRENT_SOURCE_DIR}/models)
add_test(NAME LocationFallbackTest COMMAND test_location_fallback)

# 19. Router Guards Test
add_executable(test_router_guards
    tests/cpp/test_router_guards.cpp
)
target_compile_definitions(test_router_guards PRIVATE
    QUICKCART_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(test_router_guards PRIVATE Qt6::Core Qt6::Qml Qt6::Test)
target_include_directories(test_router_guards PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core)
add_test(NAME RouterGuardsTest COMMAND test_router_guards)

# 20. Polling Synchronization Test (4g)
add_executable(test_polling_sync
    tests/cpp/test_polling_sync.cpp
    models/ordermodel.cpp
    core/orderstatemachine.cpp
    api/networkmanager.cpp
    api/circuitbreaker.cpp
    security/securestorage.cpp
    core/appconfig.cpp
    core/logging.cpp
)
target_link_libraries(test_polling_sync PRIVATE Qt6::Core Qt6::Network Qt6::Concurrent Qt6::Test ${SECURITY_LIBS})
target_include_directories(test_polling_sync PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core ${CMAKE_CURRENT_SOURCE_DIR}/security ${CMAKE_CURRENT_SOURCE_DIR}/api ${CMAKE_CURRENT_SOURCE_DIR}/models)
add_test(NAME PollingSyncTest COMMAND test_polling_sync)

# 21. Admin Flow Test (4e)
add_executable(test_admin_flow
    tests/cpp/test_admin_flow.cpp
    security/permissionmanager.cpp
    models/shopmodel.cpp
    models/ordermodel.cpp
    core/orderstatemachine.cpp
    core/validators.cpp
    core/logging.cpp
    api/networkmanager.cpp
    api/circuitbreaker.cpp
    security/securestorage.cpp
    core/appconfig.cpp
    tests/tools/mockapiserver.cpp
)
target_link_libraries(test_admin_flow PRIVATE Qt6::Core Qt6::Network Qt6::Concurrent Qt6::Test ${SECURITY_LIBS})
target_include_directories(test_admin_flow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core ${CMAKE_CURRENT_SOURCE_DIR}/security ${CMAKE_CURRENT_SOURCE_DIR}/api ${CMAKE_CURRENT_SOURCE_DIR}/models)
add_test(NAME AdminFlowTest COMMAND test_admin_flow)

# 22. Theme Token Completeness Test
add_executable(test_theme_tokens
    tests/cpp/test_theme_tokens.cpp
    core/thememanager.cpp
)
target_compile_definitions(test_theme_tokens PRIVATE
    QUICKCART_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(test_theme_tokens PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Test)
target_include_directories(test_theme_tokens PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core)
add_test(NAME ThemeTokensTest COMMAND test_theme_tokens)

# 23. Qt Quick Test Suite (TestCase for Theme tokens, Router guards, and Role login flow)
add_executable(test_qml_suite
    tests/qml/tst_qml_suite.cpp
    ${CMAKE_BINARY_DIR}/.qt/rcc/qrc_QuickCart_raw_qml_0.cpp
    ${CMAKE_BINARY_DIR}/.qt/rcc/qrc_qmake_QuickCart.cpp
    core/thememanager.cpp
)
target_compile_definitions(test_qml_suite PRIVATE
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/qml"
    QUICKCART_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    QUICKCART_IMPORT_DIR="${CMAKE_BINARY_DIR}"
)
target_link_libraries(test_qml_suite PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickTest
)
add_test(NAME QmlSuiteTest COMMAND test_qml_suite)

# 24. Domain Models & Framework Deep Coverage Test
add_executable(test_unit_models_coverage
    tests/cpp/test_unit_models_coverage.cpp
    models/cartmanager.cpp
    models/authservice.cpp
    models/ordermodel.cpp
    models/shopmodel.cpp
    models/productmodel.cpp
    api/apiclient.cpp
    api/networkmanager.cpp
    api/circuitbreaker.cpp
    security/securestorage.cpp
    security/permissionmanager.cpp
    core/orderstatemachine.cpp
    core/validators.cpp
    core/appconfig.cpp
    core/logging.cpp
    tests/tools/mockapiserver.cpp
)
target_link_libraries(test_unit_models_coverage PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Concurrent
    Qt6::Test
    ${SECURITY_LIBS}
)
target_include_directories(test_unit_models_coverage PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/security
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools
)
add_test(NAME UnitModelsCoverageTest COMMAND test_unit_models_coverage)

# 16. Static QML Lint Check via CTest
find_program(QMLLINT_EXECUTABLE qmllint HINTS /opt/homebrew/bin)
if(QMLLINT_EXECUTABLE)
    add_test(NAME QmlLint
        COMMAND ${QMLLINT_EXECUTABLE}
            -I ${CMAKE_CURRENT_SOURCE_DIR}/qml
            -I ${CMAKE_CURRENT_SOURCE_DIR}/qml/theme
            -I ${CMAKE_CURRENT_SOURCE_DIR}/qml/responsive
            -I ${CMAKE_CURRENT_SOURCE_DIR}/qml/navigation
            -I ${CMAKE_CURRENT_SOURCE_DIR}/qml/i18n
            -I ${CMAKE_CURRENT_SOURCE_DIR}/qml/icons
            -I ${CMAKE_CURRENT_SOURCE_DIR}/qml/config
            -I ${CMAKE_CURRENT_SOURCE_DIR}/qml/components
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/main.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/theme/Theme.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/theme/Metrics.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/responsive/Responsive.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/navigation/Router.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/i18n/Strings.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/icons/Icons.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/config/Config.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/components/AppScaffold.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/components/CustomButton.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/components/CustomTextField.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/components/GlassCard.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/components/HeaderBar.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/components/ThemeSelector.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/views/AdminView.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/views/AuthView.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/views/CustomerView.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/views/DeliveryView.qml
            ${CMAKE_CURRENT_SOURCE_DIR}/qml/views/ShopkeeperView.qml
    )
endif()
endif() # NOT ANDROID

