find_package(Qt6 6.7 REQUIRED COMPONENTS Core Concurrent Widgets Svg SvgWidgets)

if(APPLE)
    set(BADGE_SRC AppBadge_mac.mm)
else()
    set(BADGE_SRC AppBadge_stub.cpp)
endif()

qt_add_executable(arld
    main.cpp
    MainWindow.cpp
    MainWindow.h
    AppBadge.h
    ${BADGE_SRC}
)

target_link_libraries(arld
    PRIVATE
        arld_core
        arld_ui
        arld_export
        Qt6::Core
        Qt6::Concurrent
        Qt6::Widgets
        Qt6::Svg
        Qt6::SvgWidgets
)

set_target_properties(arld PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "ARLD"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.opsnormal.arld"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
    WIN32_EXECUTABLE TRUE
)
