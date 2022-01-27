#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Only build the documentation (used in CI/CD)

mkdir build_docs
cd build_docs
echo "INFO: Build Doxygen...";
cmake -G Ninja ..
ninja doc
