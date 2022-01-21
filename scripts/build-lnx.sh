#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Development build for Linux

if [ ! -d "build" ]; then
  echo "Creating build directory..."
  mkdir build
fi

if [ -z "$(ls build)" ]; then
  echo "INFO: Run cmake & ninja"
  cd build
  cmake -G Ninja ..
else
  echo "INFO: Only run ninja..."
  cd build
fi
ninja
