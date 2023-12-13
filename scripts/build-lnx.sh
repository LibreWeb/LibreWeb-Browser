#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Development build for Linux
if [ -z "$(ls build)" ]; then
  echo "INFO: Run cmake & ninja"
  cmake -GNinja -B build
else
  echo "INFO: Only run ninja..."
fi
cmake --build ./build
