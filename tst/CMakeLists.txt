
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

get_property(CMARK_BINARY_DIR GLOBAL PROPERTY COMMONMARKER_BINARY_DIR)

enable_testing()

add_executable(draw draw_test.cc mock-middleware.h)
target_compile_features(draw PUBLIC cxx_std_20)
set_target_properties(draw PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(draw PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMARK_BINARY_DIR} ${GTKMM_INCLUDE_DIRS})
target_link_libraries(draw PRIVATE libreweb-browser-lib-draw ${GTKMM_LIBRARIES} LibCommonMarker gtest_main gmock_main)
add_test(NAME draw_test COMMAND draw)

add_executable(file file_test.cc)
target_compile_features(file PUBLIC cxx_std_20)
set_target_properties(file PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(file PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(file PRIVATE libreweb-browser-lib-file gtest_main)
add_test(NAME file_test COMMAND file)

add_executable(parser parser_test.cc)
target_compile_features(parser PUBLIC cxx_std_20)
set_target_properties(parser PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(parser PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMARK_BINARY_DIR}  ${GTKMM_INCLUDE_DIRS})
target_link_libraries(parser PRIVATE libreweb-browser-lib-parser ${GTKMM_LIBRARIES} LibCommonMarker gtest_main)
add_test(NAME parser_test COMMAND parser)

# Add target that runs all unit-tests
# The unit tests are running in xvfb (virtual frame buffer), allowing us
# to use GTK widgets.
add_custom_target(tests ALL
  COMMAND xvfb-run env GTEST_COLOR=1 ${CMAKE_CTEST_COMMAND} --verbose --output-on-failure
  DEPENDS draw file parser
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tst
  COMMENT "Execute all unit-tests"
  VERBATIM
)
