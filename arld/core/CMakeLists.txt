find_package(nlohmann_json 3.11 REQUIRED)

add_library(arld_core STATIC
    src/Layout.cpp
    src/UndoStack.cpp
    src/AircraftLibraryParser.cpp
)

target_include_directories(arld_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(arld_core
    PUBLIC
        nlohmann_json::nlohmann_json
)

target_compile_features(arld_core PUBLIC cxx_std_20)
