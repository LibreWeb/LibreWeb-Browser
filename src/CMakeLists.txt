# TODO: Maybe move this CMake file to cmake folder and include it in the top-level Cmake file?
set (CMAKE_INCLUDE_CURRENT_DIR ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CYGWIN)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(PROJECT_TARGET browser)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM gtkmm-3.0)
pkg_check_modules(CAIRO cairomm-1.0)

add_definitions(${GTKMM_CFLAGS_OTHER})

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  # Include CPack packaging settings
  #include(packaging)
endif()

set(SOURCES 
  main.cc
  mainwindow.cc
  mainwindow.h
  menu.cc
  menu.h
  md-parser.cc
  md-parser.h
  render-area.cc
  render-area.h
)

add_executable(${PROJECT_TARGET} ${SOURCES})

# Add workaround for std filesystem in older GCC versions
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

# Get include list the cmark binary directory for config.h, .._version.h & .._export.h
# Get include list the cmark extensions binary directory for ..._export.h
get_property(CMAKE_BINARY_DIR GLOBAL PROPERTY COMMONMARKER_BINARY_DIR)
get_property(CMAKE_EXTENSIONS_BINARY_DIR GLOBAL PROPERTY COMMONMARKER_EXTENSIONS_BINARY_DIR)

target_include_directories(${PROJECT_TARGET} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${CMAKE_EXTENSIONS_BINARY_DIR}
    ${GTKMM_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    lib/commonmarker/src
  )

target_link_directories(${PROJECT_TARGET} PRIVATE
  ${GTKMM_LIBRARY_DIRS}
  ${CAIRO_LIBRARY_DIRS}
)

target_link_libraries(${PROJECT_TARGET} PRIVATE LibCommonMarker LibCommonMarkerExtensions Threads::Threads ${CXX_FILESYSTEM_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${GTKMM_LIBRARIES} ${CAIRO_LIBRARIES})

install(TARGETS ${PROJECT_TARGET} RUNTIME DESTINATION "bin" COMPONENT applications)
