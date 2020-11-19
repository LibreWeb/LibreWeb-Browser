cmake_minimum_required (VERSION 3.5)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Get VERSION from most recent git tag
include(git_version)

project (browser
  VERSION ${GIT_TAG_VERSION}
  DESCRIPTION "Browser - Decentralized WWW"
  LANGUAGES C CXX)

message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message("PROJECT_VERSION = ${PROJECT_VERSION}")

# Production build
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  add_definitions(-DPRODUCTION=1)

  # Include CPack packaging settings
  include(packaging)
endif()

add_subdirectory (lib/commonmarker/src)
add_subdirectory (lib/commonmarker/extensions)
add_subdirectory (src)

# Add desktop/images files
install(FILES misc/browser.desktop DESTINATION share/applications)
#install(FILES misc/browser.png DESTINATION share/icons/hicolor/48x48/apps)
#install(FILES misc/browser.svg DESTINATION share/icons/hicolor/scalable/apps)
#install(DIRECTORY images DESTINATION share/browser)

