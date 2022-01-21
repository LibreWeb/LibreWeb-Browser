#!/usr/bin/env bash
# Description: Remove all 'apps' folders from hicolor
find ./packaging_win/share/icons/hicolor/ -type d -name "*apps" -prune -exec rm -rf {} +
