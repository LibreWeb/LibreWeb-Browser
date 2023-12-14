#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Only build the documentation (used in CI/CD)
cmake -GNinja -B build_docs
cmake --build ./build_docs --target doc
