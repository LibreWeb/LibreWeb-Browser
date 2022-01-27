#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Development build for Linux, same as build-lnx.sh but without Doxygen

if [ ! -d "build" ]; then
  echo "Creating build directory..."
  mkdir build
fi

if [ -z "$(ls build)" ]; then
  echo "INFO: Run cmake & ninja"
  cd build
  cmake -G Ninja -DDOXYGEN:BOOL=FALSE ..
else
  echo "INFO: Only run ninja..."
  cd build
fi
ninja
