cmake_minimum_required(VERSION 3.21)
project(UsageFloat VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Network Widgets Svg)
qt_standard_project_setup(REQUIRES 6.8)
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.8)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(usage-float src/main.cpp src/controller.cpp src/controller.h src/usage.cpp src/usage.h)
set(app_icon_sizes 16 24 32 48 64 128 256 512)
set(app_icon_files)
foreach(size IN LISTS app_icon_sizes)
    list(APPEND app_icon_files "packaging/icons/${size}.png")
endforeach()
qt_add_resources(usage-float app_icons PREFIX "/icons" BASE "packaging/icons" FILES ${app_icon_files})
if(WIN32)
    target_sources(usage-float PRIVATE packaging/UsageFloat.rc)
    target_include_directories(usage-float PRIVATE packaging)
elseif(APPLE)
    target_sources(usage-float PRIVATE packaging/icons/UsageFloat.icns)
    set_source_files_properties(packaging/icons/UsageFloat.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(usage-float PROPERTIES MACOSX_BUNDLE_ICON_FILE UsageFloat.icns)
endif()
qt_add_qml_module(usage-float
    URI UsageFloat VERSION 1.0
    QML_FILES qml/Main.qml qml/ProviderCard.qml qml/Meter.qml qml/IconButton.qml qml/UsageToolTip.qml
)
target_link_libraries(usage-float PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Network Qt6::Widgets Qt6::Svg)
# Ship the headless platform too, so CI tests the deployed app rather than
# accidentally borrowing a platform plugin from the development SDK.
qt_import_plugins(usage-float INCLUDE Qt6::QOffscreenIntegrationPlugin)
if(UNIX AND NOT APPLE)
    # Explicitly include Wayland in Qt's deployment dependency graph. Otherwise
    # a working SDK build can become an X11-only installed app.
    find_package(Qt6 REQUIRED COMPONENTS WaylandClient)
    target_link_libraries(usage-float PRIVATE Qt6::WaylandClient)
    qt_import_plugins(usage-float INCLUDE Qt6::QWaylandIntegrationPlugin Qt6::QWaylandEglPlatformIntegrationPlugin)
endif()
set_target_properties(usage-float PROPERTIES
    WIN32_EXECUTABLE TRUE MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER io.usagefloat.UsageFloat
    MACOSX_BUNDLE_BUNDLE_NAME "Usage Float"
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)

include(GNUInstallDirs)
install(TARGETS usage-float BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(UNIX AND NOT APPLE)
    install(FILES packaging/io.usagefloat.UsageFloat.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/io.usagefloat.UsageFloat.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
    foreach(size IN LISTS app_icon_sizes)
        install(FILES "packaging/icons/${size}.png"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps"
            RENAME io.usagefloat.UsageFloat.png)
    endforeach()
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
    qt_add_executable(window-tests tests/window_test.cpp src/controller.cpp src/controller.h src/usage.cpp)
    target_include_directories(window-tests PRIVATE src)
    target_compile_definitions(window-tests PRIVATE USAGE_FLOAT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(window-tests PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Network Qt6::Test)
    add_test(NAME window-transitions COMMAND window-tests)
    set_tests_properties(window-transitions PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
endif()
