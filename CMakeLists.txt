cmake_minimum_required (VERSION 3.5)

project (browser LANGUAGES C CXX)

add_subdirectory (lib/commonmarker/src)
add_subdirectory (lib/commonmarker/extensions)
add_subdirectory (src)