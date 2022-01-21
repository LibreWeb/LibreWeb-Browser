# Rename the libary to our needs
set(LIBRARY "LibCommonMarkerExtensions")
# Extend the sources with our LibreWeb extensions
set (LIBRARY_SOURCES
   core-extensions.c
   table.c
   strikethrough.c
   autolink.c
   tagfilter.c
   ext_scanners.c
   ext_scanners.re
   ext_scanners.h
   tasklist.c
   highlight.c
   superscript.c
   subscript.c
)

set_property(GLOBAL PROPERTY COMMONMARKER_EXTENSIONS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

include_directories(
  ${PROJECT_SOURCE_DIR}/lib/commonmarker/src
  ${PROJECT_BINARY_DIR}/lib/commonmarker/src
)

include (GenerateExportHeader)

include_directories(. ${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_RELEASE} -pg")
set(CMAKE_LINKER_PROFILE "${CMAKE_LINKER_FLAGS_RELEASE} -pg")

add_library(${LIBRARY} ${LIBRARY_SOURCES})
target_include_directories(${LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(${LIBRARY} PROPERTIES
 COMPILE_FLAGS "-DCMARK_GFM_STATIC_DEFINE -DCMARK_GFM_EXTENSIONS_STATIC_DEFINE"
 POSITION_INDEPENDENT_CODE ON)

# Generate export.h
generate_export_header(${LIBRARY}
  BASE_NAME cmark-gfm-extensions)

target_link_libraries(${LIBRARY} LibCommonMarker)
