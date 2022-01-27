#!/usr/bin/env bash
# TODO: add to following flag to the cppcheck: --addon=cert
cppcheck --enable=all --library=googletest --suppressions-list=suppressions.txt --error-exitcode=1 "$@" -I lib/commonmarker/src/ -I lib/commonmarker/extensions/ ./src ./tst
