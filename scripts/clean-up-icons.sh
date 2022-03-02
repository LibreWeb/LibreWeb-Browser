#!/usr/bin/env bash
# Description: Remove all 'apps' folders from hicolor icon theme
# and remove legacy & cursors folders from Adwaita icon theme.

# Remove all the apps folders from hicolor icon theme
find ./misc/packaging_icons/hicolor/ -type d -name "*apps" -prune -exec rm -rf {} +

# Remove all the legacy folders
find ./misc/packaging_icons/Adwaita/ -type d -name "*legacy" -prune -exec rm -rf {} +

# Remove cursors folder
rm -rf ./misc/packaging_icons/Adwaita/cursors

