#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Download & untar Go-IPFS
wget --quiet -O "$DIR/go-ipfs.tar.gz" "https://dist.ipfs.io/go-ipfs/v0.7.0/go-ipfs_v0.7.0_linux-amd64.tar.gz"
tar -xvzf "$DIR/go-ipfs.tar.gz" -C "$DIR/../"
# Clean-up
rm -rf "$DIR/go-ipfs.tar.gz"
