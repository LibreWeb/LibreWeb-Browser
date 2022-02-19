include(${CMAKE_SOURCE_DIR}/cmake/GSettings.cmake)

set(PROJECT_TARGET libreweb-browser)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# Find required dependencies
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
# Only for macOS
if(APPLE)
    pkg_check_modules(MAC_INTEGRATION REQUIRED gtk-mac-integration-gtk3)
endif()

# Generate Project version header file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/project_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/project_config.h)

# Include the binary dir, in order to find project_config.h
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Source code
set(HEADERS
    about.h
    draw.h
    file.h
    ipfs.h
    middleware-i.h
    middleware.h
    toolbar-button.h
    toc-model-cols.h
    mainwindow.h
    md-parser.h
    menu.h
    ipfs-daemon.h
    option-group.h
    source-code-dialog.h
)
set(SOURCES 
  main.cc
  about.cc
  draw.cc
  file.cc
  ipfs.cc
  middleware.cc
  toolbar-button.cc
  mainwindow.cc
  md-parser.cc
  menu.cc
  ipfs-daemon.cc
  option-group.cc
  source-code-dialog.cc
  ${HEADERS}
)

# Install and recompile glib gsettings schema
add_schema("org.libreweb.browser.gschema.xml" GSCHEMA_RING)

# For Windows & macOS
if(WIN32 OR APPLE)
    # Additional schema files
    set(ADDITIONAL_SCHEMA_FILES
        org.gtk.Settings.ColorChooser.gschema.xml
        org.gtk.Settings.Debug.gschema.xml
        org.gtk.Settings.EmojiChooser.gschema.xml
        org.gtk.Settings.FileChooser.gschema.xml
    )
    message(STATUS "Add Windows Schema files")
    foreach(SCHEMA_FILE ${ADDITIONAL_SCHEMA_FILES})
        add_schema(${SCHEMA_FILE} GSCHEMA_RING)
    endforeach(SCHEMA_FILE)
endif()
# Only for Windows
if(WIN32)
    # Text format Windows resource file for Resource Compiler (rc)
    set(WINDOWS_RES_IN ${CMAKE_SOURCE_DIR}/misc/libreweb.rc.in)
    set(WINDOWS_RES ${CMAKE_CURRENT_BINARY_DIR}/libreweb.rc)
    configure_file(${WINDOWS_RES_IN} ${WINDOWS_RES} @ONLY)
	# Set a default rc compiler if it was not defined yet
	if(NOT CMAKE_RC_COMPILER)
		set(CMAKE_RC_COMPILER windres)
	endif()
	enable_language(RC)
	set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -Ocoff -o <OBJECT> <SOURCE>")
	set_source_files_properties(${WINDOWS_RES} PROPERTIES LANGUAGE RC)
endif()

# Get include list the cmark binary directory for the generated config.h, .._version.h & .._export.h files
# Get include list the cmark extensions binary directory for the generated ..._export.h file
get_property(CMARK_BINARY_DIR GLOBAL PROPERTY COMMONMARKER_BINARY_DIR)
get_property(CMARK_EXTENSIONS_BINARY_DIR GLOBAL PROPERTY COMMONMARKER_EXTENSIONS_BINARY_DIR)

if(NOT UNITTEST)
    # LibreWeb binary
    if(APPLE)
        # Add macOS icns icon to bundle
        set(MACOSX_BUNDLE_ICON_FILE ${CMAKE_SOURCE_DIR}/images/icons/libreweb-browser.icns)
        set_source_files_properties(${MACOSX_BUNDLE_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

        add_executable(${PROJECT_TARGET} MACOSX_BUNDLE ${GSCHEMA_RING} ${MACOSX_BUNDLE_ICON_FILE} ${SOURCES})
        # Set MacOSX bundle & XCode props
        set_target_properties(${PROJECT_TARGET}
            PROPERTIES
            MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}-v${PROJECT_VERSION}"
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/misc/Info.plist"
            MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME}"
            MACOSX_BUNDLE_ICON_FILE libreweb-browser.icns
            MACOSX_BUNDLE_GUI_IDENTIFIER "${PROJECT_NAME}"
            MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${CODE_SIGN_IDENTITY}"
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${DEVELOPMENT_TEAM_ID}"
        )
    else()
        add_executable(${PROJECT_TARGET} ${GSCHEMA_RING} ${WINDOWS_RES} ${SOURCES})
    endif()

    ## Definitions just in case
    if(WIN32)
        # Define _WIN32 for Windows platforms
        target_compile_definitions(${PROJECT_TARGET} PRIVATE _WIN32)
    elseif(APPLE)
        # Define __APPLE__ for macOS platforms
        target_compile_definitions(${PROJECT_TARGET} PRIVATE __APPLE__)
    elseif(UNIX)
        # Define __linux__ for Unix platforms
        target_compile_definitions(${PROJECT_TARGET} PRIVATE __linux__)
    endif()

    # Add fallback for std filesystem in older GCC versions
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.4)
            message(FATAL_ERROR "You are on an extremely old version of GCC. Please update your compiler to at least GCC 8.0, preferably latest")
        elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
            message(WARNING "Old Version of GCC detected. Using Legacy C++ support")
            # Add stdc++fs library in older GCC compiler versions
            set(CXX_FILESYSTEM_LIBRARIES "stdc++fs")
            target_compile_definitions(${PROJECT_TARGET} PUBLIC LEGACY_CXX)
        endif()
    endif()

    target_include_directories(${PROJECT_TARGET} PRIVATE
        ${CMARK_BINARY_DIR}
        ${CMARK_EXTENSIONS_BINARY_DIR}
        ${GTKMM_INCLUDE_DIRS}
    )
    target_link_directories(${PROJECT_TARGET} PRIVATE ${GTKMM_LIBRARY_DIRS})
    target_link_libraries(${PROJECT_TARGET} PRIVATE
        LibCommonMarker
        LibCommonMarkerExtensions
        ipfs-http-client
        whereami
        Threads::Threads
        ${CXX_FILESYSTEM_LIBRARIES}
        ${GTKMM_LIBRARIES}
        nlohmann_json::nlohmann_json
    )
    target_compile_options(${PROJECT_TARGET} PRIVATE ${GTKMM_CFLAGS_OTHER})

    if(APPLE)
        # Add GTK Mac Integration for macOS
        target_include_directories(${PROJECT_TARGET} PRIVATE ${MAC_INTEGRATION_INCLUDE_DIRS})
        target_link_directories(${PROJECT_TARGET} PRIVATE ${MAC_INTEGRATION_LIBRARY_DIRS})
        target_link_libraries(${PROJECT_TARGET} PRIVATE ${MAC_INTEGRATION_LIBRARIES})
        target_compile_options(${PROJECT_TARGET} PRIVATE ${MAC_INTEGRATION_CFLAGS_OTHER})

        # Install browser binary macOS
        install(TARGETS ${PROJECT_TARGET}
            RUNTIME DESTINATION .
            BUNDLE DESTINATION . COMPONENT Runtime)

        SET(APP_BUNDLE_NAME LibreWeb.app)
        SET(BUNDLE_INSTALL_TARGET ${CMAKE_INSTALL_PREFIX}/${APP_BUNDLE_NAME})
        SET(APPS ${BUNDLE_INSTALL_TARGET})

        # TODO: Install  integration dylib
        #install( FILES ${libqrencode_LIBRARIES} DESTINATION ${APP_BUNDLE_NAME}/Contents/MacOS)
        #foreach( lib ${libqrencode_LIBRARIES} )
        #    get_filename_component(libfile ${lib} NAME)
        #    MESSAGE("Adding ${libfile} to bundle")
        #    list(APPEND BUNDLELIBS
        #        ${BUNDLE_INSTALL_TARGET}/Contents/MacOS/${libfile})
        #endforeach(lib ${libqrencode_LIBRARIES})
        # Add "BUNDLELIBS" to fixup_bundle

        # Fix-up bundle & verify if app is standalone
        # https://cmake.org/cmake/help/latest/module/BundleUtilities.html
        install(CODE "
            include(BundleUtilities)
            set(BU_CHMOD_BUNDLE_ITEMS ON)
            fixup_bundle(\"${APPS}\" \"\" \"\")
            verify_app(${APPS})")
    else()
        # Install browser binary
        install(TARGETS ${PROJECT_TARGET} RUNTIME DESTINATION bin)
    endif()

else()
    # Build seperate libraries for unit testing
    set(PROJECT_TARGET_LIB ${PROJECT_TARGET}-lib)
    add_library(${PROJECT_TARGET_LIB}-file STATIC file.h file.cc)
    add_library(${PROJECT_TARGET_LIB}-draw STATIC draw.h draw.cc md-parser.h md-parser.cc)
    add_library(${PROJECT_TARGET_LIB}-parser STATIC md-parser.h md-parser.cc)
    # Only link/include external libs we really need for the unittest libaries
    target_include_directories(${PROJECT_TARGET_LIB}-draw PRIVATE
        ${CMARK_BINARY_DIR}
        ${CMARK_EXTENSIONS_BINARY_DIR}
        ${GTKMM_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_TARGET_LIB}-draw PRIVATE
        LibCommonMarker
        LibCommonMarkerExtensions
        ${GTKMM_LIBRARIES}
    )
    target_compile_options(${PROJECT_TARGET_LIB}-draw PRIVATE ${GTKMM_CFLAGS_OTHER})
    target_include_directories(${PROJECT_TARGET_LIB}-parser PRIVATE
        ${CMARK_BINARY_DIR}
        ${CMARK_EXTENSIONS_BINARY_DIR}
        ${GTKMM_INCLUDE_DIRS}
    )
    target_link_directories(${PROJECT_TARGET_LIB}-parser PRIVATE ${GTKMM_LIBRARY_DIRS})
    target_link_libraries(${PROJECT_TARGET_LIB}-parser PRIVATE
        LibCommonMarker
        LibCommonMarkerExtensions
    )
endif()