cmake_minimum_required (VERSION 3.20)

option(DOXYGEN "Build Doxygen documentation" ON)
option(UNITTEST "Build unit tests")
option(PACKAGE "Build packages in release mode" OFF)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(GitVersion)

project(libreweb-browser
  VERSION ${GIT_TAG_VERSION}
  DESCRIPTION "LibreWeb Browser - Decentralized Web-Browser"
  LANGUAGES C CXX)

message(STATUS "Project version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Create packages: ${PACKAGE}")
message(STATUS "Generate documentation: ${DOXYGEN}")
if(UNITTEST)
  message(STATUS "Building the unit tests")
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Build docs using Doxygen
if(DOXYGEN)
  include(Doxygen)
endif()

# Install location
if(WIN32)
  set(DATADIR share                               CACHE PATH "datadir")
elseif(APPLE)
  set(BUNDLEDIR ${PROJECT_NAME}.app/Contents/)
  set(RESOURCESDIR ${BUNDLEDIR}/Resources)
  set(MACOSDIR ${BUNDLEDIR}/MacOS                 CACHE PATH "macosdir")
  set(DATADIR ${RESOURCESDIR}/share               CACHE PATH "datadir")
  set(LIBDIR ${RESOURCESDIR}/lib                  CACHE PATH "libadir")
elseif(UNIX)
  set (DATADIR share                              CACHE PATH "datadir")
endif()

# Production build, let's include the CPack settings
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release" AND PACKAGE)
  include(Packaging)
endif()

if(WIN32)
  set(WINDOWS_FLAGS -mwindows)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -pedantic -Werror=incompatible-pointer-types")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  -Wall")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${WINDOWS_FLAGS} -O2 -DNDEBUG")

# Disable CTest testcases & install in cpp-ipfs-http-client
set (BUILD_TESTING OFF CACHE BOOL "Disable CTest" FORCE)
set (DISABLE_INSTALL ON CACHE BOOL "Disable Install" FORCE)

add_subdirectory (lib/commonmarker/src)
add_subdirectory (lib/commonmarker/extensions)
add_subdirectory (lib/ipfs-http-client)
add_subdirectory (lib/whereami)
add_subdirectory (src)
if(UNITTEST)
  add_subdirectory(tst)
endif()

# Additional install files
if(WIN32)
  # Microsoft Windows specific
  install(DIRECTORY misc/packaging_icons/ DESTINATION ${DATADIR}/icons)
  install(DIRECTORY misc/packaging_win/ DESTINATION .)
  install(PROGRAMS go-ipfs/ipfs.exe DESTINATION bin)
elseif(APPLE)
  # Apple macOS specific
  install(DIRECTORY misc/packaging_icons/ DESTINATION ${DATADIR}/icons)
  install(DIRECTORY misc/packaging_macos/ DESTINATION ${MACOSDIR})
  install(PROGRAMS go-ipfs/ipfs-darwin DESTINATION ${MACOSDIR})
elseif(UNIX)
  # UNIX specific
  install(FILES misc/libreweb-browser.desktop DESTINATION ${DATADIR}/applications)
  install(PROGRAMS go-ipfs/ipfs DESTINATION bin)
endif()
# General
install(DIRECTORY images DESTINATION ${DATADIR}/libreweb)
install(FILES images/icons/libreweb-browser.png DESTINATION ${DATADIR}/icons/hicolor/48x48/apps)
install(FILES images/icons/libreweb-browser.svg DESTINATION ${DATADIR}/icons/hicolor/scalable/apps)
