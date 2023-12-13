#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Development build for Linux, same as build-lnx.sh but without Doxygen
if [ -z "$(ls build)" ]; then
  echo "INFO: Run cmake & ninja"
  cmake -GNinja -DDOXYGEN:BOOL=FALSE -B build
else
  echo "INFO: Only run ninja..."
fi
cmake --build ./build
