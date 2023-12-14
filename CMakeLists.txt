cmake_minimum_required (VERSION 3.20)

option(DOXYGEN "Build Doxygen documentation" ON)
option(UNITTEST "Build unit tests")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(GitVersion)

project(libreweb-browser
  VERSION ${GIT_TAG_VERSION}
  DESCRIPTION "LibreWeb Browser - Decentralized Web-Browser"
  LANGUAGES C CXX)

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "PROJECT_VERSION: ${PROJECT_VERSION}")
if(UNITTEST)
  message(STATUS "Building the unit tests")
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
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  include(Packaging)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_C_EXTENSIONS NO)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CYGWIN)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

if(WIN32)
  set(WINDOWS_FLAGS -mwindows)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -pedantic -Werror=incompatible-pointer-types")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  -Wall")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${WINDOWS_FLAGS} -O3 -DNDEBUG")

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
