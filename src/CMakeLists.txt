set (CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

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
  markdown-render.cc
  markdown-render.h
  mainwindow.cc
  mainwindow.h
  scene.h
  scene.cc
)

if(ANDROID)
  add_library (browser SHARED
    ${SOURCES}
  )
else()
  add_executable (browser
    ${SOURCES}
  )
endif()

# Get the cmark binary dir for config.h
get_property(cmark_binary_dir GLOBAL PROPERTY COMMONMARKER_BINARY_DIR)
include_directories(${cmark_binary_dir})

include_directories(lib/commonmarker/src)

target_link_libraries (browser LINK_PUBLIC LibCommonMarker LibCommonMarkerExtensions)
target_link_libraries (browser PRIVATE Qt5::Widgets)
