cmake_minimum_required(VERSION 3.21)
project(UsageFloat VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Network Widgets Svg)
qt_standard_project_setup(REQUIRES 6.5)
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.8)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(usage-float src/main.cpp src/controller.cpp src/controller.h src/usage.cpp src/usage.h)
qt_add_qml_module(usage-float
    URI UsageFloat VERSION 1.0
    QML_FILES qml/Main.qml qml/ProviderCard.qml qml/Meter.qml qml/IconButton.qml qml/UsageToolTip.qml
)
target_link_libraries(usage-float PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Network Qt6::Widgets Qt6::Svg)
if(UNIX AND NOT APPLE)
    # Explicitly include Wayland in Qt's deployment dependency graph. Otherwise
    # a working SDK build can become an X11-only installed app.
    find_package(Qt6 REQUIRED COMPONENTS WaylandClient)
    target_link_libraries(usage-float PRIVATE Qt6::WaylandClient)
    qt_import_plugins(usage-float INCLUDE Qt6::QWaylandIntegrationPlugin Qt6::QWaylandEglPlatformIntegrationPlugin)
endif()
set_target_properties(usage-float PROPERTIES
    WIN32_EXECUTABLE TRUE MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER io.usagefloat.desktop
    MACOSX_BUNDLE_BUNDLE_NAME "Usage Float"
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)

include(GNUInstallDirs)
install(TARGETS usage-float BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(UNIX AND NOT APPLE)
    install(FILES packaging/io.usagefloat.desktop.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/io.usagefloat.desktop.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif()
qt_generate_deploy_qml_app_script(TARGET usage-float OUTPUT_SCRIPT deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
install(SCRIPT ${deploy_script})

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(usage-tests tests/usage_test.cpp src/usage.cpp)
    target_include_directories(usage-tests PRIVATE src)
    target_link_libraries(usage-tests PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME usage-parsing COMMAND usage-tests)
    qt_add_executable(controller-tests tests/controller_test.cpp src/controller.cpp src/controller.h src/usage.cpp)
    target_include_directories(controller-tests PRIVATE src)
    target_link_libraries(controller-tests PRIVATE Qt6::Core Qt6::Gui Qt6::Network Qt6::Test)
    add_test(NAME provider-lifecycle COMMAND controller-tests)
endif()
