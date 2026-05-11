find_package(Qt6 6.7 REQUIRED COMPONENTS Core Widgets Svg SvgWidgets)

add_library(arld_ui STATIC
    include/arld/ui/AircraftItem.h
    include/arld/ui/ClearanceZoneItem.h
    include/arld/ui/LibraryPanel.h
    include/arld/ui/RampBoundaryItem.h
    include/arld/ui/RampScene.h
    include/arld/ui/RampView.h
    include/arld/ui/ScaleBarItem.h
    src/AircraftItem.cpp
    src/ClearanceZoneItem.cpp
    src/LibraryPanel.cpp
    src/RampBoundaryItem.cpp
    src/RampScene.cpp
    src/RampView.cpp
    src/ScaleBarItem.cpp
    ../data/library/aircraft_library.qrc
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
        Qt6::SvgWidgets
)

target_compile_features(arld_ui PUBLIC cxx_std_20)
