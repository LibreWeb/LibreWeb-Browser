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

# Add JSON Parser via CMake Fetch Content feature
include(FetchContent)

FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
  GIT_TAG v3.9.1)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  # Include CPack packaging settings
  #include(packaging)
endif()

set(HEADERS
    about.h
    file.h
    ipfs.h
    mainwindow.h
    menu.h
    md-parser.h
    draw.h
    source-code-dialog.h
)

set(SOURCES 
  main.cc
  about.cc
  file.cc
  ipfs.cc
  mainwindow.cc
  menu.cc
  md-parser.cc
  draw.cc
  source-code-dialog.cc
  ${HEADERS}
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

# Get include list the cmark binary directory for the generated config.h, .._version.h & .._export.h files
# Get include list the cmark extensions binary directory for the generated ..._export.h file
get_property(CMAKE_BINARY_DIR GLOBAL PROPERTY COMMONMARKER_BINARY_DIR)
get_property(CMAKE_EXTENSIONS_BINARY_DIR GLOBAL PROPERTY COMMONMARKER_EXTENSIONS_BINARY_DIR)

target_include_directories(${PROJECT_TARGET} PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_EXTENSIONS_BINARY_DIR}
    ${GTKMM_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/lib/ipfs-http-client/include
  )

target_link_directories(${PROJECT_TARGET} PRIVATE
  ${GTKMM_LIBRARY_DIRS}
  ${CAIRO_LIBRARY_DIRS}
)

target_link_libraries(${PROJECT_TARGET} PRIVATE LibCommonMarker LibCommonMarkerExtensions ipfs-http-client Threads::Threads ${CXX_FILESYSTEM_LIBRARIES} ${GTKMM_LIBRARIES} ${CAIRO_LIBRARIES} nlohmann_json::nlohmann_json)

install(TARGETS ${PROJECT_TARGET} RUNTIME DESTINATION "bin" COMPONENT applications)
