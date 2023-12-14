#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Development build Windows 64-bit
if [ -z "$(ls build_win)" ]; then
  echo "INFO: Run cmake & ninja"
  x86_64-w64-mingw32.static-cmake -GNinja -B build_win
else
  echo "INFO: Only run ninja..."
fi
x86_64-w64-mingw32.static-cmake --build ./build_win
