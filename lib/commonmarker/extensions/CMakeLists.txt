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
   )

include_directories (
  ${PROJECT_SOURCE_DIR}/lib/commonmarker/src
  ${PROJECT_BINARY_DIR}/lib/commonmarker/src
)

include (GenerateExportHeader)
include_directories (. ${CMAKE_CURRENT_BINARY_DIR})

add_library (CommonMarkerExtensions ${LIBRARY_SOURCES})
target_include_directories (CommonMarkerExtensions PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
