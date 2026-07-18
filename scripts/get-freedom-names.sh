#!/usr/bin/env bash
# Description: Download the Freedom Names node release binary into the freedom-names/
# directory, where the CMake install rules (and the packaging) expect it:
#   freedom-names          (linux)
#   freedom-names.exe      (windows)
#   freedom-names-darwin   (macOS)
#
# Source: the GitLab release artifacts of the freedom-names project
# (https://gitlab.melroy.org/freedom-names/freedom-names), built by the
# `go-release` job on tag pipelines. A mirror exists on GitHub:
# https://github.com/FreedomNames/FreedomNames/releases
#
# Usage:
#   ./scripts/get-freedom-names.sh                 # auto-detect host OS
#   ./scripts/get-freedom-names.sh linux windows   # explicit target(s)
#   ./scripts/get-freedom-names.sh all             # linux + windows + darwin
# Environment:
#   FN_VERSION   Release tag (bare, no "v" prefix), default: 0.8.3
#   FN_ARCH      amd64 or arm64, default: auto-detect from uname -m
#   FN_BASE_URL  Override the artifact download base URL
set -euo pipefail

FN_VERSION="${FN_VERSION:-0.8.3}"
PROJECT_URL="https://gitlab.melroy.org/freedom-names/freedom-names"
FN_BASE_URL="${FN_BASE_URL:-$PROJECT_URL/-/jobs/artifacts/$FN_VERSION/raw/build_release}"
CI_JOB="go-release"

CURRENT_DIR=$(dirname "$(readlink -f "$0")")
DEST_DIR="$CURRENT_DIR/../freedom-names"

# SHA256 checksums of the pinned release archives. Update when bumping FN_VERSION.
checksum_for() {
  case "$1" in
  0.8.3-linux-amd64) echo "82d9a37cbba009bc66c4dac8a8e3d64faa7abb3833360bea049cc0533b3953f0" ;;
  0.8.3-linux-arm64) echo "1ae441a99a7b1c0f68946c88e21e9aee8536ed8290f0d35ed145a590dfe9dd8d" ;;
  0.8.3-windows-amd64) echo "7370b7aa2fd542b20e3195aaea054c6437c87cf55fc90de767c1225d0a7a4510" ;;
  0.8.3-windows-arm64) echo "84e3044a7f4349274479793fc473c8767580bd7cdb40698f92626467cccaa01e" ;;
  0.8.3-darwin-amd64) echo "1874779605c25dde4fad19753254fb7f4fd83f6699a57375c5846b9a0caa3fce" ;;
  0.8.3-darwin-arm64) echo "ba56504fdd65791be29c2743e25b5a42b7342fadcc465265b7da386a3a9641c9" ;;
  *) echo "" ;;
  esac
}

detect_arch() {
  case "${FN_ARCH:-$(uname -m)}" in
  amd64 | x86_64) echo "amd64" ;;
  arm64 | aarch64) echo "arm64" ;;
  *)
    echo "ERROR: Unsupported architecture '$(uname -m)'. Set FN_ARCH=amd64 or FN_ARCH=arm64." >&2
    exit 1
    ;;
  esac
}

detect_os() {
  case "$(uname -s)" in
  Linux) echo "linux" ;;
  Darwin) echo "darwin" ;;
  MINGW* | MSYS* | CYGWIN*) echo "windows" ;;
  *)
    echo "ERROR: Could not detect host OS. Pass an explicit target: linux, windows or darwin." >&2
    exit 1
    ;;
  esac
}

fetch() {
  local url="$1" dest="$2"
  echo "INFO: Downloading $url"
  if command -v curl >/dev/null 2>&1; then
    curl -fsSL -o "$dest" "$url"
  elif command -v wget >/dev/null 2>&1; then
    wget --quiet -O "$dest" "$url"
  else
    echo "ERROR: Neither curl nor wget is available." >&2
    exit 1
  fi
}

get_target() {
  local os="$1" ext extracted out
  case "$os" in
  linux)
    ext="tar.gz"
    extracted="freedom-names"
    out="freedom-names"
    ;;
  windows)
    ext="zip"
    extracted="freedom-names.exe"
    out="freedom-names.exe"
    ;;
  darwin)
    ext="tar.gz"
    extracted="freedom-names"
    out="freedom-names-darwin" # rename, matches the CMake install rule
    ;;
  *)
    echo "ERROR: Unknown target '$os'. Valid targets: linux, windows, darwin, all." >&2
    exit 1
    ;;
  esac

  local key="$FN_VERSION-$os-$ARCH"
  local archive="freedom-names-$key.$ext"
  local stamp="$DEST_DIR/.$out.version"

  # Idempotency: skip the download when the stamp matches and the binary is present.
  if [ -x "$DEST_DIR/$out" ] && [ "$(cat "$stamp" 2>/dev/null)" = "$key" ]; then
    echo "INFO: $out $key already present, skipping."
    return 0
  fi

  local tmp_dir="$TMP_DIR/$os"
  mkdir -p "$tmp_dir"

  fetch "$FN_BASE_URL/$archive?job=$CI_JOB" "$tmp_dir/$archive"

  # Verify the archive: pinned checksum when known, size sanity check otherwise.
  local expected_sum
  expected_sum=$(checksum_for "$key")
  if [ -n "$expected_sum" ]; then
    echo "$expected_sum  $tmp_dir/$archive" | sha256sum --check --quiet - || {
      echo "ERROR: Checksum mismatch for $archive." >&2
      exit 1
    }
  else
    echo "WARN: No pinned checksum for $key, only checking the archive size."
    local size
    size=$(wc -c <"$tmp_dir/$archive")
    if [ "$size" -lt 5000000 ]; then
      echo "ERROR: $archive is suspiciously small ($size bytes), aborting." >&2
      exit 1
    fi
  fi

  # Each archive contains a single binary at its root.
  case "$ext" in
  tar.gz) tar -xzf "$tmp_dir/$archive" -C "$tmp_dir" "$extracted" ;;
  zip)
    command -v unzip >/dev/null 2>&1 || {
      echo "ERROR: unzip is required for the windows target (e.g. apt-get install unzip)." >&2
      exit 1
    }
    unzip -qo "$tmp_dir/$archive" "$extracted" -d "$tmp_dir"
    ;;
  esac

  mkdir -p "$DEST_DIR"
  mv "$tmp_dir/$extracted" "$DEST_DIR/$out"
  chmod +x "$DEST_DIR/$out"
  echo "$key" >"$stamp"
  echo "INFO: Saved $DEST_DIR/$out ($key)"
}

ARCH=$(detect_arch)
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT
if [ "$#" -eq 0 ]; then
  TARGETS=("$(detect_os)")
elif [ "$1" = "all" ]; then
  TARGETS=(linux windows darwin)
else
  TARGETS=("$@")
fi

for target in "${TARGETS[@]}"; do
  get_target "$target"
done

echo "INFO: Done. CMake will pick the binary up from freedom-names/ during install."
