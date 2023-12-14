#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Build Windows 64-bit production release
rm -rf build_prod_win
x86_64-w64-mingw32.static-cmake -GNinja -DDOXYGEN:BOOL=FALSE -DCMAKE_BUILD_TYPE=Release -B build_prod_win
x86_64-w64-mingw32.static-cmake --build ./build_prod_win --config Release 
# Build packages
cd build_prod_win
# For some reason it couldn't find the makensis binary, hence the -D option
cpack -D CPACK_NSIS_EXECUTABLE=/usr/bin/makensis -C Release -G NSIS
