cmake_minimum_required(VERSION 3.28)

# Must be set before project() when using vcpkg toolchain via environment
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(arld VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(arld/core)
add_subdirectory(arld/export)
add_subdirectory(arld/ui)
add_subdirectory(arld/app)

enable_testing()
add_subdirectory(arld/tests)
