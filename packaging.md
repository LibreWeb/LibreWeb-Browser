# Packaging

The additional files in `packaging_win` are required for packaging 
to the Windows platform (after the cross-compile went successfully).

## Freedom Names node binary

Every package bundles the [Freedom Names](https://gitlab.melroy.org/freedom-names/freedom-names) node binary,
which the browser starts automatically at runtime. The `scripts/get-freedom-names.sh` script downloads the
pinned release into the `freedom-names/` directory (the `build-*-prod.sh` scripts call it automatically), and
CMake installs it next to the browser binary: `bin/freedom-names` (Linux), `bin\freedom-names.exe` (Windows NSIS)
and `Contents/MacOS/freedom-names-darwin` (macOS app bundle).

## Native GTK Icon Themes

GTK icons themes we include in the Windows & macOS packages:

* Adwaita (**Source:** `/usr/share/icons/Adwaita` Linux Mint 20.x distro)
* hicolor - Used as fallback theme (**Source:** `/usr/share/icons/hicolor` Linux Mint 20.x distro)

Those all the icon themes are stored in the `share/icons` directory during cmake install/cpack.

*Important:* We **clean-up unnecessary icons** to reduce the package size. 
Do not forget to run the script: `./scripts/clean-up-icons.sh` to remove all unwanted files.

## macOS platform

Run `./scripts/build-macos-prod.sh` on a macOS machine (or let the GitHub Actions "MacOS build" workflow do it,
which builds both an Apple Silicon and an Intel DMG). The script builds the `libreweb-browser.app` bundle and
packages it into a drag & drop DMG image via CPack (`cpack -G DragNDrop`), named
`libreweb-browser-v<version>-macos-<arch>.dmg`.

During `cmake --install`/`cpack`, the [BundleUtilities](https://cmake.org/cmake/help/latest/module/BundleUtilities.html)
`fixup_bundle()` call copies all non-system dylibs the browser links against (the gtkmm stack and
gtk-mac-integration, resolved from the Homebrew prefix via pkg-config) into the app bundle and rewrites the
install names, so the shipped app is standalone.

No D-Bus helper binaries are bundled: the browser runs as `APPLICATION_NON_UNIQUE` (no session bus needed for
GApplication uniqueness), GSettings uses the keyfile backend on macOS and processes are spawned directly via
`Glib::spawn`. Should a session bus ever become necessary, install fresh `dbus`/`glib` binaries from Homebrew
at package time on the runner (do not check pre-built Mach-O binaries into git — they go stale and their
hardcoded Homebrew keg paths break on other machines).

Code signing / notarization is not done yet (see the TODO in `scripts/build-macos-prod.sh`).

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

