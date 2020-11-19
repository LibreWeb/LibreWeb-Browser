# TODO: Maybe move this CMake file to cmake folder and include it in the top-level Cmake file?
set (CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(PROJECT_TARGET browser)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package (Qt5 COMPONENTS Widgets REQUIRED)
find_library (libcmark-gfm libcmark-gfm.so REQUIRED)
find_library (libcmark-gfm-extensions libcmark-gfm-extensions.so REQUIRED)

set(SOURCES 
  main.cc
  md-parser.cc
  md-parser.h
  md-render.cc
  md-render.h
  mainwindow.cc
  mainwindow.h
  scene.h
  scene.cc
)

# TODO: Why is the executable 'browser' detetected as shared lib? I Just want to have an executable
#       Maybe because I build shared libs in other CMakeLists, .. uhm
if(ANDROID)
  add_library (${PROJECT_TARGET} SHARED
    ${SOURCES}
  )
else()
  add_executable (${PROJECT_TARGET}
    ${SOURCES}
  )
endif()


# Get the cmark binary dir for config.h
get_property(cmark_binary_dir GLOBAL PROPERTY COMMONMARKER_BINARY_DIR)
include_directories(${cmark_binary_dir})

include_directories(lib/commonmarker/src)

# TODO: 1x target_link_libraries?
target_link_libraries (${PROJECT_TARGET} LINK_PUBLIC LibCommonMarker LibCommonMarkerExtensions)
target_link_libraries (${PROJECT_TARGET} PRIVATE Qt5::Widgets)

install(TARGETS ${PROJECT_TARGET} RUNTIME DESTINATION "bin" COMPONENT applications)
