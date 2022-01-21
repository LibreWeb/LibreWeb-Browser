#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Build Windows 64-bit production release

rm -rf build_prod_win
mkdir build_prod_win
cd build_prod_win

x86_64-w64-mingw32.static-cmake -G Ninja -DDOXYGEN:BOOL=FALSE -DCMAKE_BUILD_TYPE=Release .. &&
ninja && 
echo "INFO: Start NSIS packaging for Windows...";
cpack -C Release -G NSIS
