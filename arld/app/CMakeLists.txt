find_package(Qt6 6.7 REQUIRED COMPONENTS Core Widgets)

qt_add_executable(arld
    main.cpp
    MainWindow.cpp
    MainWindow.h
)

target_link_libraries(arld
    PRIVATE
        arld_core
        arld_ui
        arld_export
        Qt6::Core
        Qt6::Widgets
)

set_target_properties(arld PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
