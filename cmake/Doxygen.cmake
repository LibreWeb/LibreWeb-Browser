find_package(Doxygen
             REQUIRED dot
             OPTIONAL_COMPONENTS mscgen dia)
set(DOXYFILE_IN ${CMAKE_SOURCE_DIR}/misc/Doxyfile.in)
set(DOXYFILE ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

# Build (configure) doxyfile
configure_file(${DOXYFILE_IN} ${DOXYFILE} @ONLY)

# The depends ALL option build the docs together with the app
add_custom_target(doc ALL
  COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Generating documentation with Doxygen"
  VERBATIM
)
