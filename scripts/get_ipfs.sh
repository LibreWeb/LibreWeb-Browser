#!/bin/bash
# By: Melroy van den Berg
#
# Provide the IPFS version below

IPFS_VERSION=0.9.1

##############################
# Leave the code alone below #
##############################
CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

echo "INFO: Start downloading IPFS (version ${IPFS_VERSION})..."
# Download & untar Go-IPFS
wget --quiet "https://dist.ipfs.io/go-ipfs/v${IPFS_VERSION}/go-ipfs_v${IPFS_VERSION}_linux-amd64.tar.gz" -O "$CURRENT_DIR/go-ipfs.tar.gz"

echo "INFO: Extracting Go IPFS..."
# Extract on root level of the git repo
tar -xvzf "$CURRENT_DIR/go-ipfs.tar.gz" -C "$CURRENT_DIR/../"
echo "INFO: Clean up"
# Clean-up
rm -rf "$CURRENT_DIR/go-ipfs.tar.gz"
