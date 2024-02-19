#!/bin/bash
# By: Melroy van den Berg
# Description: Automatically download and unzip the IPFS CLI (Kubo) binaries for Linux & Windows

## Provide the Kubo version below ##
KUBO_VERSION=0.26.0

##############################
# Leave the code alone below #
##############################
CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Download & untar/zip Kubo
echo "INFO: Start downloading Kubo (v${KUBO_VERSION})..."
wget --quiet "https://dist.ipfs.tech/kubo/v${KUBO_VERSION}/kubo_v${KUBO_VERSION}_darwin-amd64.tar.gz" -O "$CURRENT_DIR/kubo_darwin.tar.gz"
wget --quiet "https://dist.ipfs.tech/kubo/v${KUBO_VERSION}/kubo_v${KUBO_VERSION}_linux-amd64.tar.gz" -O "$CURRENT_DIR/kubo_linux.tar.gz"
wget --quiet "https://dist.ipfs.tech/kubo/v${KUBO_VERSION}/kubo_v${KUBO_VERSION}_windows-amd64.zip" -O "$CURRENT_DIR/kubo_windows.zip"

# Extract on root level of the git repo
echo "INFO: Extracting Kubo..."
tar -xzf "$CURRENT_DIR/kubo_darwin.tar.gz" kubo/ipfs -C "$CURRENT_DIR/../"
# rename darwin binary
mv "$CURRENT_DIR/../kubo/ipfs" "$CURRENT_DIR/../kubo/ipfs-darwin"
tar -xzf "$CURRENT_DIR/kubo_linux.tar.gz" kubo/ipfs -C "$CURRENT_DIR/../"
unzip -q -o "$CURRENT_DIR/kubo_windows.zip" kubo/ipfs.exe -d "$CURRENT_DIR/../"

# Clean-up
echo "INFO: Clean up"
rm -rf "$CURRENT_DIR/kubo_darwin.tar.gz"
rm -rf "$CURRENT_DIR/kubo_linux.tar.gz"
rm -rf "$CURRENT_DIR/kubo_windows.zip"
