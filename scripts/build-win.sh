#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Development build Windows 64-bit

if [ ! -d "build_win" ]; then
  echo "Creating build directory..."
  mkdir build_win
fi

if [ -z "$(ls build_win)" ]; then
  echo "INFO: Run cmake & ninja"
  cd build_win
  x86_64-w64-mingw32.static-cmake -G Ninja ..
else
  echo "INFO: Only run ninja..."
  cd build_win
fi
ninja
