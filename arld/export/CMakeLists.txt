# Export module: SVG, PDF, PNG, JPEG pipeline
# Scaffolded Sprint 0-1; SVG export implementation begins Sprint 0-5

find_package(Qt6 6.7 REQUIRED COMPONENTS Core Svg)

add_library(arld_export INTERFACE)

target_link_libraries(arld_export
    INTERFACE
        arld_core
        Qt6::Core
        Qt6::Svg
)
