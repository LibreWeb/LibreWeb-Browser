# Packaging

The additional files in `packaging_win` and `packaging_macos` are required for packaging 
to respectively the Windows and macOS platforms (after the cross-compile went successfully).

## Native GTK Icon Themes

GTK icons themes we include in the Windows & macOS packages:

* Adwaita (**Source:** `/usr/share/icons/Adwaita` Linux Mint 20.x distro)
* hicolor - Used as fallback theme (**Source:** `/usr/share/icons/hicolor` Linux Mint 20.x distro)

Those all the icon themes are stored in the `share/icons` directory during cmake install/cpack.

*Important:* We **clean-up unnecessary icons** to reduce the package size. 
Do not forget to run the script: `./scripts/clean-up-icons.sh` to remove all unwanted files.

## Windows platform

### GTK Windows 10 Theme

We include the GTK "Windows 10" theme in the Windows package. Giving LibreWeb a very native look & feel under Microsoft Windows.
For that we depend on 3rd part GTK Windows 10 theme:

* [Windows 10 theme](https://github.com/B00merang-Project/Windows-10)
* [Windows 10 icon set](https://github.com/B00merang-Artwork/Windows-10/)

Those Windows 10 theme/icon files are stored inside the respectively `share/themes` and `share/icons` directories.


### Windows GTK Binary files

The following GTK binary files should be shipped together with the `libreweb-browser.exe` binary.

The files should be placed in the `bin` directory.

* gdbus.exe / dbus-daemon.exe - Creating a gdbus daemon under Windows (**Source:** MXE cross-build)
* gspawn-win64-helper.exe - Starting processes under Windows (**Source:** MXE cross-build)
* gspawn-win64-console.exe - Starting processes under Windows (**Source:** MXE cross-build)
* And more...

And for debugging purpose:

* gdb.exe - For debugging the binary (**source:** MinGW)

