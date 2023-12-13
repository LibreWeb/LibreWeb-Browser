#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Debug build for Linux
rm -rf build
make -GNinja -DDOXYGEN:BOOL=FALSE -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build ./build --config Debug
