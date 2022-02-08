# Windows Packaging 

The additional files in `packaging_win` are required for packaging to the Windows platform (after the cross-compile went successfully).

## GTK Windows 10 Theme

We ship the Windows binary with the Windows 10 theme. Giving LibreWeb a very native look under Windows.
For that we depend on 3rd partt GTK theme.

* [Windows 10 theme](https://github.com/B00merang-Project/Windows-10)
* [Windows 10 icon set](https://github.com/B00merang-Artwork/Windows-10/)

Those Windows 10 theme/icon files are stored inside the respectively `share/themes` and `share/icons` directories.

## Native GTK Theme Icons

Icons themes:

* Adwaita (**Source:** `/usr/share/icons/Adwaita` Linux Mint 20.x distro)
* hicolor - Used as fallback theme (**Source:** `/usr/share/icons/hicolor` Linux Mint 20.x distro)
  * Important: Do not forget to remove all the apps folder contents/icons. This lead to extra size and maybe installation issues due to symbolic links.
    Therefor run the `./scripts/clean-up-hicolor-icons.sh` script. Which removes all `apps` directories.

Those all the icon themes are stored in the `share/icons` directory.

## Windows GTK Binary files

The following GTK binary files should be shipped together with the `libreweb-browser.exe` binary.

The files should be placed in the `bin` directory.

* gdbus.exe / dbus-daemon.exe - Creating a gdbus daemon under Windows (**Source:** MXE cross-build)
* gspawn-win64-helper.exe - Starting processes under Windows (**Source:** MXE cross-build)
* gspawn-win64-console.exe - Starting processes under Windows (**Source:** MXE cross-build)
* And more...

And for debugging purpose:

* gdb.exe - For debugging the binary (**source:** MinGW)

