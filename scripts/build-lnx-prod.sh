#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Linux release (production) build + create Debian package file (.deb), 
#  RPM [Red Hat] Package Manager (.rpm) and compressed file (.tgz/.tar.gz)
#
#  Installs into /usr prefix directory under Linux.

# First build the application for Linux
cmake -GNinja -DCMAKE_INSTALL_PREFIX:PATH=/usr -DDOXYGEN:BOOL=FALSE -DCMAKE_BUILD_TYPE=Release -B build_prod
cmake --build ./build_prod --config Release 
# Build packages
cd build_prod
cpack -C Release -G "TGZ;DEB;RPM"
