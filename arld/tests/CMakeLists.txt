find_package(Catch2 3 REQUIRED)
include(CTest)
include(Catch)

add_executable(arld_tests
    test_smoke.cpp
    test_undo.cpp
)

target_link_libraries(arld_tests
    PRIVATE
        arld_core
        Catch2::Catch2WithMain
)

catch_discover_tests(arld_tests)
