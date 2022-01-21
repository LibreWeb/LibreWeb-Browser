include(${CMAKE_SOURCE_DIR}/cmake/GSettings.cmake)

set(PROJECT_TARGET libreweb-browser)
set(THREADS_PREFER_PTHREAD_FLAG ON)

if(WIN32)
  set(WINDOWS_FLAGS -mwindows)
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${WINDOWS_FLAGS} -O3 -DNDEBUG")

# Find required dependencies
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

# Generate Project version header file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/project_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/project_config.h)

# In order to find project_config.h
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Source code
set(HEADERS
    about.h
    draw.h
    file.h
    ipfs.h
    middleware.h
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

# Only for Windows
if(WIN32)
    # Windows Schema files
    set(WINDOWS_SCHEMA_FILES
        org.gtk.Settings.ColorChooser.gschema.xml
        org.gtk.Settings.Debug.gschema.xml
        org.gtk.Settings.EmojiChooser.gschema.xml
        org.gtk.Settings.FileChooser.gschema.xml
    )
    message(STATUS "Add Windows Schema files")
    foreach(SCHEMA_FILE ${WINDOWS_SCHEMA_FILES})
        add_schema(${SCHEMA_FILE} GSCHEMA_RING)
    endforeach(SCHEMA_FILE)

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

add_executable(${PROJECT_TARGET} ${GSCHEMA_RING} ${WINDOWS_RES} ${SOURCES})

## Definitions just in case
# Define _WIN32 for Windows platforms
if(WIN32)
    target_compile_definitions(${PROJECT_TARGET} PRIVATE _WIN32)
endif()
# Define __linux__ for Unix platforms
if (UNIX)
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

# Get include list the cmark binary directory for the generated config.h, .._version.h & .._export.h files
# Get include list the cmark extensions binary directory for the generated ..._export.h file
get_property(CMAKE_BINARY_DIR GLOBAL PROPERTY COMMONMARKER_BINARY_DIR)
get_property(CMAKE_EXTENSIONS_BINARY_DIR GLOBAL PROPERTY COMMONMARKER_EXTENSIONS_BINARY_DIR)

target_include_directories(${PROJECT_TARGET} PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_EXTENSIONS_BINARY_DIR}
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

# Install browser binary
install(TARGETS ${PROJECT_TARGET} RUNTIME DESTINATION bin)
