#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Build & run unit-tests

if [ ! -d "build_test" ]; then
  echo "Creating test build directory..."
  mkdir build_test
fi

if [ -z "$(ls build_test)" ]; then
  echo "INFO: Run cmake & ninja"
  cd build_test
  cmake -G Ninja -DDOXYGEN:BOOL=FALSE -DUNITTEST:BOOL=TRUE ..
else
  echo "INFO: Only run ninja..."
  cd build_test
fi
# Build & run unit tests
ninja tests
