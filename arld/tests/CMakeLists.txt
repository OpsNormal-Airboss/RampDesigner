find_package(Catch2 3 REQUIRED)
include(CTest)
include(Catch)

add_executable(arld_tests
    test_smoke.cpp
    test_undo.cpp
    test_aircraft_library.cpp
    test_clearance.cpp
    test_project_file.cpp
    test_pdf_exporter.cpp
    test_png_exporter.cpp
    test_schema_migration.cpp
    test_unit_converter.cpp
    test_svg_sanitizer.cpp
    test_tail_swing.cpp
    test_batch_exporter.cpp
    test_versioning.cpp
)

target_link_libraries(arld_tests
    PRIVATE
        arld_core
        arld_export
        Catch2::Catch2WithMain
)

catch_discover_tests(arld_tests)
