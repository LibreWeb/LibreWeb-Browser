#!/usr/bin/env bash
cppcheck --enable=all  --suppressions-list=suppressions.txt --error-exitcode=1 "$@" -I lib/commonmarker/src/ -I lib/commonmarker/extensions/ ./src