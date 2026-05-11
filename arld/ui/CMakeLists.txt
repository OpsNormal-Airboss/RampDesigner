# UI module: Qt canvas, toolbar, panels, undo/redo stack
# Scaffolded Sprint 0-1; canvas implementation begins Sprint 0-2

find_package(Qt6 6.7 REQUIRED COMPONENTS Core Widgets Svg)

add_library(arld_ui INTERFACE)

target_link_libraries(arld_ui
    INTERFACE
        arld_core
        Qt6::Core
        Qt6::Widgets
        Qt6::Svg
)
