#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Used for memory leak analysis,
# be-aware that you will get a lot of false positives messages due to GTK

valgrind --leak-check=full --track-origins=yes ./build/src/libreweb-browser
