cmake_minimum_required(VERSION 3.28)

# Must be set before project() when using vcpkg toolchain via environment
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(arld VERSION 1.0.1 LANGUAGES CXX)

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

# ---------------------------------------------------------------------------
# CPack packaging
# ---------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "ARLD")
set(CPACK_PACKAGE_VENDOR "OpsNormal Airboss")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Airshow Ramp Layout Designer")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(UNIX)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "OpsNormal Airboss <airboss@pmlairboss.com>")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6widgets6, libqt6svg6")
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Airshow Ramp Layout Designer")
    set(CPACK_NSIS_PACKAGE_NAME "ARLD")
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/arld/app/arld.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/arld/app/arld.ico")
    # Start Menu shortcut: path relative to install prefix -> display name
    set(CPACK_NSIS_MENU_LINKS
        "bin/arld.exe" "Airshow Ramp Layout Designer"
    )
    # Offer to launch the app from the finish page
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "arld.exe")
endif()

include(CPack)

enable_testing()
add_subdirectory(arld/tests)

# ---------------------------------------------------------------------------
# NOTICES.txt generation (Sprint 2-3-9)
# ---------------------------------------------------------------------------
add_custom_target(generate_notices
    COMMAND ${CMAKE_COMMAND} -E echo "Generating NOTICES.txt..."
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/check-licenses.py
            --notices ${CMAKE_SOURCE_DIR}/NOTICES.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating NOTICES.txt from scripts/check-licenses.py"
)
