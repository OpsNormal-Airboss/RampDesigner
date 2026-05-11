find_package(Qt6 6.7 REQUIRED COMPONENTS Core Widgets Svg)

add_library(arld_ui STATIC
    src/RampBoundaryItem.cpp
    src/RampScene.cpp
    src/RampView.cpp
    src/ScaleBarItem.cpp
)

target_include_directories(arld_ui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(arld_ui
    PUBLIC
        arld_core
        Qt6::Core
        Qt6::Widgets
        Qt6::Svg
)

target_compile_features(arld_ui PUBLIC cxx_std_20)
