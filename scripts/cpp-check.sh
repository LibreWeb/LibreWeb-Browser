#!/usr/bin/env bash
# TODO: add to following flag to the cppcheck: --addon=cert
cppcheck --enable=all --library=googletest --suppressions-list=suppressions.txt --inline-suppr --check-level=exhaustive --error-exitcode=1 "$@" -I ./src -I lib/commonmarker/src/ -I lib/commonmarker/extensions/ ./src ./tst
