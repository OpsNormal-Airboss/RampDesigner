# Export module: SVG, PDF, PNG, JPEG pipeline
# Sprint 0-5: SVG export implementation
# Sprint 1-5: PDF export (libharu), ViolationReportExporter (CSV)

find_package(nlohmann_json 3.11 REQUIRED)
find_package(ZLIB QUIET)

add_library(arld_export STATIC
    include/arld/export/IExporter.h
    include/arld/export/SvgExporter.h
    include/arld/export/PdfExporter.h
    include/arld/export/PngExporter.h
    include/arld/export/JpegExporter.h
    include/arld/export/BatchExporter.h
    include/arld/export/ViolationReportExporter.h
    src/SvgExporter.cpp
    src/PdfExporter.cpp
    src/PngExporter.cpp
    src/JpegExporter.cpp
    src/BatchExporter.cpp
    src/ViolationReportExporter.cpp
)

target_include_directories(arld_export PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# stb_image_write (vendored header-only)
target_include_directories(arld_export PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(arld_export PUBLIC arld_core nlohmann_json::nlohmann_json)

target_compile_features(arld_export PUBLIC cxx_std_20)

# libharu (PDF export)
find_library(HPDF_LIB hpdf HINTS /opt/homebrew/lib ${CMAKE_PREFIX_PATH}/lib)
find_path(HPDF_INCLUDE_DIR hpdf.h HINTS /opt/homebrew/include ${CMAKE_PREFIX_PATH}/include)
if (HPDF_LIB AND HPDF_INCLUDE_DIR)
    target_include_directories(arld_export PRIVATE ${HPDF_INCLUDE_DIR})
    target_link_libraries(arld_export PRIVATE ${HPDF_LIB})
    # PUBLIC so consumers (e.g. test binary) know libharu is available
    target_compile_definitions(arld_export PUBLIC HAVE_LIBHARU=1)
    # libharu requires zlib for deflate (PDF stream compression)
    if (ZLIB_FOUND)
        target_link_libraries(arld_export PUBLIC ZLIB::ZLIB)
    else()
        find_library(ZLIB_LIB z)
        if (ZLIB_LIB)
            target_link_libraries(arld_export PUBLIC ${ZLIB_LIB})
        endif()
    endif()
    message(STATUS "libharu found: ${HPDF_LIB}")
else()
    message(STATUS "libharu NOT found — PdfExporter will be a stub")
endif()

# libqrencode (QR codes in PDF)
find_library(QRENCODE_LIB qrencode HINTS /opt/homebrew/lib ${CMAKE_PREFIX_PATH}/lib)
find_path(QRENCODE_INCLUDE_DIR qrencode.h HINTS /opt/homebrew/include ${CMAKE_PREFIX_PATH}/include)
if (QRENCODE_LIB AND QRENCODE_INCLUDE_DIR)
    target_include_directories(arld_export PRIVATE ${QRENCODE_INCLUDE_DIR})
    target_link_libraries(arld_export PRIVATE ${QRENCODE_LIB})
    target_compile_definitions(arld_export PRIVATE HAVE_QRENCODE=1)
    message(STATUS "libqrencode found: ${QRENCODE_LIB}")
else()
    message(STATUS "libqrencode NOT found — QR code will be omitted from PDF title block")
endif()
