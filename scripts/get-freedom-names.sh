#!/usr/bin/env bash
# Description: Download the Freedom Names node binary into the freedom-names/ directory,
# where the CMake install rules (and the packaging) expect it.
#
# Source: the GitLab CI artifact of the latest successful pipeline on a ref
# (the freedom-names project's `go-build` job publishes the `freedom-names` binary).
# Once the project starts tagging releases, set FN_VERSION to fetch a release
# asset instead of a moving branch artifact.
#
# Usage:
#   ./scripts/get-freedom-names.sh                # latest main branch CI artifact
#   FN_REF=some-branch ./scripts/get-freedom-names.sh
#   FN_VERSION=v0.3.0 ./scripts/get-freedom-names.sh   # release asset (once tags exist)
set -euo pipefail

PROJECT_URL="https://gitlab.melroy.org/freedom-names/freedom-names"
FN_REF="${FN_REF:-main}"
FN_VERSION="${FN_VERSION:-}"
CI_JOB="go-build"

CURRENT_DIR=$(dirname "$(readlink -f "$0")")
DEST_DIR="$CURRENT_DIR/../freedom-names"
mkdir -p "$DEST_DIR"

download() {
  local url="$1" dest="$2"
  echo "INFO: Downloading $url"
  wget --quiet "$url" -O "$dest" || {
    echo "ERROR: Download failed: $url" >&2
    rm -f "$dest"
    return 1
  }
  chmod +x "$dest"
  echo "INFO: Saved $dest"
}

if [ -n "$FN_VERSION" ]; then
  # Release asset (permanent link). Requires the freedom-names project to attach
  # the binary as a release asset named "freedom-names" on tag $FN_VERSION.
  download "$PROJECT_URL/-/releases/$FN_VERSION/downloads/freedom-names" "$DEST_DIR/freedom-names"
else
  # Raw artifact from the latest successful pipeline on $FN_REF.
  download "$PROJECT_URL/-/jobs/artifacts/$FN_REF/raw/freedom-names?job=$CI_JOB" "$DEST_DIR/freedom-names"
fi

# TODO: Windows (freedom-names.exe) and macOS (freedom-names-darwin) binaries.
# The freedom-names CI only builds linux/amd64 today; once it adds
# GOOS=windows / GOOS=darwin cross-build jobs (Go cross-compiles trivially),
# fetch them here the same way:
#   download "$PROJECT_URL/-/jobs/artifacts/$FN_REF/raw/freedom-names.exe?job=go-build-windows" "$DEST_DIR/freedom-names.exe"
#   download "$PROJECT_URL/-/jobs/artifacts/$FN_REF/raw/freedom-names-darwin?job=go-build-darwin" "$DEST_DIR/freedom-names-darwin"

echo "INFO: Done. CMake will pick the binary up from freedom-names/ during install."
