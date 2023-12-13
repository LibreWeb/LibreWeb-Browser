#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Build macOS 64-bit production release using Drag & Drop installer
cmake -GNinja -DDOXYGEN:BOOL=FALSE -DCMAKE_BUILD_TYPE=Release -B build_prod_macos
# TODO:
# cmake -G Xcode -DCODE_SIGN_IDENTITY="codesign ID..." -DDEVELOPMENT_TEAM_ID="team ID..."
# xcodebuild \
#  -project "libreweb-browser.xcodeproj" \
#  -scheme libreweb-browser \
#  -configuration Release
cmake --build ./build_prod_macos --config Release 
# Build MacOS package
cd build_prod_macos
cpack -C Release -G DragNDrop
