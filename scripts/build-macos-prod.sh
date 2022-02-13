#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Build macOS 64-bit production release using Drag & Drop installer

rm -rf build_prod_macos
mkdir build_prod_macos
cd build_prod_macos

cmake -G Ninja -DDOXYGEN:BOOL=FALSE -DCMAKE_BUILD_TYPE=Release ..
# TODO:
# cmake -G Xcode -DCODE_SIGN_IDENTITY="codesign ID..." -DDEVELOPMENT_TEAM_ID="team ID..."


#TODO: xcodebuild \
# -project "libreweb-browser.xcodeproj" \
#    -scheme libreweb-browser \
#    -configuration Release

ninja && 
echo "INFO: Start Drag & Drop packaging for MacOS...";
cpack -C Release -G DragNDrop
