#!/bin/bash
# By: Melroy van den Berg
# Description: Automatically download and unzip the IPFS CLI binaries for Linux & Windows

## Provide the IPFS version below ##
IPFS_VERSION=0.11.0

##############################
# Leave the code alone below #
##############################
CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Download & untar/zip Go-IPFS
echo "INFO: Start downloading IPFS (version ${IPFS_VERSION})..."
wget --quiet "https://dist.ipfs.io/go-ipfs/v${IPFS_VERSION}/go-ipfs_v${IPFS_VERSION}_linux-amd64.tar.gz" -O "$CURRENT_DIR/go-ipfs_linux.tar.gz"
wget --quiet "https://dist.ipfs.io/go-ipfs/v${IPFS_VERSION}/go-ipfs_v${IPFS_VERSION}_windows-amd64.zip" -O "$CURRENT_DIR/go-ipfs_windows.zip"

# Extract on root level of the git repo
echo "INFO: Extracting Go IPFS..."
tar -xzf "$CURRENT_DIR/go-ipfs_linux.tar.gz" -C "$CURRENT_DIR/../"
unzip -q -o "$CURRENT_DIR/go-ipfs_windows.zip" -d "$CURRENT_DIR/../"

# Clean-up
echo "INFO: Clean up"
rm -rf "$CURRENT_DIR/go-ipfs_linux.tar.gz"
rm -rf "$CURRENT_DIR/go-ipfs_windows.zip"
