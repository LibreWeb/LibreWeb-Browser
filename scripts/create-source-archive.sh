#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Create custom source code archive with version.txt
# Depends on one environment variable: $CI_COMMIT_TAG

if [ -z ${CI_COMMIT_TAG} ]; then
    echo "WARNING: CI_COMMIT_TAG env. variable is not set! WARNING if this should become a new release."
    echo
    echo "Skipping git archive command..."
    exit 0
else
    # Create version.txt
    echo -n "${CI_COMMIT_TAG}" > version.txt
    # Create source archive (tar.gz)
    git archive --format=tar.gz --output=build_prod/libreweb-browser-source-v${CI_COMMIT_TAG}.tar.gz --add-file=version.txt HEAD
fi
