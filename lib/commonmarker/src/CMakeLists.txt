if(${CMAKE_VERSION} VERSION_GREATER "3.3")
    cmake_policy(SET CMP0063 NEW)
endif()
# Set version within this CMake file
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 29)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION_GFM 2)

# Rename the libary to our needs
set(LIBRARY "LibCommonMarker")
set(HEADERS
  cmark-gfm.h
  cmark-gfm-extension_api.h
  parser.h
  buffer.h
  node.h
  iterator.h
  chunk.h
  references.h
  footnotes.h
  map.h
  utf8.h
  scanners.h
  inlines.h
  houdini.h
  cmark_ctype.h
  render.h
  registry.h
  syntax_extension.h
  plugin.h
  )
set(LIBRARY_SOURCES
  cmark.c
  node.c
  iterator.c
  blocks.c
  inlines.c
  scanners.c
  scanners.re
  utf8.c
  buffer.c
  references.c
  footnotes.c
  map.c
  render.c
  man.c
  xml.c
  html.c
  commonmark.c
  plaintext.c
  latex.c
  houdini_href_e.c
  houdini_html_e.c
  houdini_html_u.c
  cmark_ctype.c
  arena.c
  linked_list.c
  syntax_extension.c
  registry.c
  plugin.c
  ${HEADERS}
  )

set_property(GLOBAL PROPERTY COMMONMARKER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

include_directories(. ${CMAKE_CURRENT_BINARY_DIR})
include_directories(
  ${PROJECT_BINARY_DIR}/extensions
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmark-gfm_version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmark-gfm_version.h)

include (GenerateExportHeader)

# -fvisibility=hidden
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# Feature tests
include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckSymbolExists)
CHECK_INCLUDE_FILE(stdbool.h HAVE_STDBOOL_H)
CHECK_C_SOURCE_COMPILES(
  "int main() { __builtin_expect(0,0); return 0; }"
  HAVE___BUILTIN_EXPECT)
CHECK_C_SOURCE_COMPILES("
  int f(void) __attribute__ (());
  int main() { return 0; }
" HAVE___ATTRIBUTE__)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_library(${LIBRARY} ${LIBRARY_SOURCES})
target_include_directories(${LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Generate export.h
generate_export_header(${LIBRARY}
  BASE_NAME cmark-gfm)
