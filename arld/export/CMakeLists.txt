# Export module: SVG, PDF, PNG, JPEG pipeline
# Sprint 0-5: SVG export implementation

find_package(nlohmann_json 3.11 REQUIRED)

add_library(arld_export STATIC
    include/arld/export/IExporter.h
    include/arld/export/SvgExporter.h
    include/arld/export/PdfExporter.h
    include/arld/export/PngExporter.h
    include/arld/export/JpegExporter.h
    include/arld/export/BatchExporter.h
    src/SvgExporter.cpp
    src/PdfExporter.cpp
    src/PngExporter.cpp
    src/JpegExporter.cpp
    src/BatchExporter.cpp
)

target_include_directories(arld_export PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(arld_export PUBLIC arld_core nlohmann_json::nlohmann_json)

target_compile_features(arld_export PUBLIC cxx_std_20)
