find_package(Qt6 6.7 REQUIRED COMPONENTS Core Concurrent Widgets Svg SvgWidgets)

if(APPLE)
    set(BADGE_SRC AppBadge_mac.mm)
else()
    set(BADGE_SRC AppBadge_stub.cpp)
endif()

qt_add_executable(arld
    main.cpp
    MainWindow.cpp
    MainWindow.h
    AppBadge.h
    TelemetryManager.h
    TelemetryManager.cpp
    ${BADGE_SRC}
    user_manual.qrc
)

set_target_properties(arld PROPERTIES AUTORCC ON)

target_link_libraries(arld
    PRIVATE
        arld_core
        arld_ui
        arld_export
        Qt6::Core
        Qt6::Concurrent
        Qt6::Widgets
        Qt6::Svg
        Qt6::SvgWidgets
)

set_target_properties(arld PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "ARLD"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.opsnormal.arld"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    WIN32_EXECUTABLE TRUE
)

if(WIN32)
    # Embed the app icon into the executable via a Windows resource file
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/arld_win.rc" "IDI_ICON1 ICON \"${CMAKE_CURRENT_SOURCE_DIR}/arld.ico\"\n")
    target_sources(arld PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/arld_win.rc")
endif()

# Install the .app bundle (macOS) or executable (other platforms)
install(TARGETS arld
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Bundle Qt frameworks and plugins into the installed .app for distribution
qt_generate_deploy_app_script(
    TARGET arld
    OUTPUT_SCRIPT _deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${_deploy_script})

if(WIN32)
    # windeployqt only handles Qt DLLs; vcpkg DLLs (gmp-10.dll, hpdf.dll, …)
    # must be copied separately.  Capture the vcpkg bin dir at configure time
    # so the install-time CODE block can find the DLLs.
    set(_vcpkg_dll_dir "")
    if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
        set(_vcpkg_dll_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
    endif()

    install(CODE "
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES \"\${CMAKE_INSTALL_PREFIX}/bin/arld.exe\"
            RESOLVED_DEPENDENCIES_VAR   _deps
            UNRESOLVED_DEPENDENCIES_VAR _missing
            DIRECTORIES \"${_vcpkg_dll_dir}\"
            PRE_EXCLUDE_REGEXES
                [[api-ms-win-.*]]
                [[ext-ms-.*]]
                [[hvsifiletrust\\.dll]]
            POST_EXCLUDE_REGEXES
                [[.*[/\\\\\\\\][Ss]ystem32[/\\\\\\\\].*\\.dll]]
                [[.*[/\\\\\\\\][Ss]yswow64[/\\\\\\\\].*\\.dll]]
        )
        foreach(_dep \${_deps})
            file(INSTALL
                DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\"
                TYPE SHARED_LIBRARY
                FOLLOW_SYMLINK_CHAIN
                FILES \"\${_dep}\"
            )
        endforeach()
        if(_missing)
            message(WARNING \"Unresolved DLL dependencies at install time: \${_missing}\")
        endif()
    ")
endif()
