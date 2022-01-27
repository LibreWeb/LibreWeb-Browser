#!/usr/bin/env bash
# Description: Check the coding style guidelines (only dry-run)
find src/ tst/ -iname *.h -o -iname *.cc -o -iname *.h.in | xargs clang-format --dry-run -Werror -style=file -fallback-style=LLVM
