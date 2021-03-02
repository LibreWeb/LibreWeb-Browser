cmake_minimum_required (VERSION 3.11.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(git_version)

project (browser
  VERSION ${GIT_TAG_VERSION}
  DESCRIPTION "Browser - Decentralized WWW"
  LANGUAGES C CXX)

message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message("PROJECT_VERSION = ${PROJECT_VERSION}")

# Production build
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  # Include CPack packaging settings
  include(packaging)
endif()

# Disable CTest testcases & install in cpp-ipfs-http-client
set (BUILD_TESTING OFF CACHE BOOL "Disable CTest" FORCE)
set (DISABLE_INSTALL ON CACHE BOOL "Disable Install" FORCE)

add_subdirectory (lib/commonmarker/src)
add_subdirectory (lib/commonmarker/extensions)
add_subdirectory (lib/ipfs-http-client)
add_subdirectory (src)

# Additional install files
install(FILES misc/libreweb-browser.desktop DESTINATION share/applications)
install(PROGRAMS go-ipfs/ipfs DESTINATION share/libreweb-browser/go-ipfs)
install(DIRECTORY images DESTINATION share/libreweb-browser)
install(FILES images/icons/libreweb-browser.png DESTINATION share/icons/hicolor/48x48/apps)
install(FILES images/icons/libreweb-browser.svg DESTINATION share/icons/hicolor/scalable/apps)
