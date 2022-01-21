#!/usr/bin/env bash
# Description: Check the coding style guidelines & fix them automatically
find src/ -iname *.h -o -iname *.cc -o -iname *.h.in | xargs clang-format -i -style=file -fallback-style=LLVM -assume-filename=../.clang-format
