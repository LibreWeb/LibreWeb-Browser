#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Linux release (production) build + create Debian package file (.deb), 
#  RPM [Red Hat] Package Manager (.rpm) and compressed file (.tgz/.tar.gz)
#
#  Installs into /usr prefix directory under Linux.

rm -rf build_prod
mkdir build_prod
cd build_prod
# First build the application for Linux
echo "INFO: Start building...";
cmake -G Ninja -DCMAKE_INSTALL_PREFIX:PATH=/usr -DDOXYGEN:BOOL=FALSE -DCMAKE_BUILD_TYPE=Release ..
ninja && 
echo "INFO: Start packaging to tgz, deb and rpm...";
cpack -C Release -G "TGZ;DEB;RPM"
