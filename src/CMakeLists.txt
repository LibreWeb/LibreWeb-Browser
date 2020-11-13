set (CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)

set (CMAKE_CXX_STANDARD 11)
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

if(ANDROID)
  add_library (browser SHARED
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
  )
else()
  add_executable (browser
    main.cpp
    markdown-render.cpp
    markdown-render.h
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
  )
endif()

target_link_libraries (browser LINK_PUBLIC CommonMarker CommonMarkerExtensions)
target_link_libraries (browser PRIVATE Qt5::Widgets)
