# Example 1: https://github.com/MariaDB/server/tree/10.8/cmake
# Example 2: https://gitlab.com/inkscape/inkscape/blob/master/CMakeScripts/ConfigCPack.cmake
#set(CPACK_PACKAGE_NAME "LibreWeb Browser")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "LibreWeb Browser - Decentralized Web-Browser")
set(CPACK_PACKAGE_VENDOR "Melroy van den Berg")
set(CPACK_PACKAGE_CONTACT "Melroy van den Berg <info@libreweb.org>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://libreweb.org")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/misc/package_desc.txt")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_TARGET}-${CPACK_PACKAGE_VERSION}")
set(CPACK_DEBIAN_PACKAGE_SECTION "web")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Internet")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-v${CPACK_PACKAGE_VERSION}") # Without '-Linux' or '-Win' suffix
# Windows specific options - GUI Installer (NSIS generator)
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/images/browser_logo_small.bmp")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "LibreWeb")
set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
set(CPACK_NSIS_HELP_LINK "https://docs.libreweb.org/how-tos/")
set(CPACK_NSIS_URL_INFO_ABOUT "https://libreweb.org")
set(CPACK_NSIS_DISPLAY_NAME "LibreWeb Browser")
set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} v${CPACK_PACKAGE_VERSION}")
set(CPACK_PACKAGE_EXECUTABLES "libreweb-browser;LibreWeb Browser")
set(CPACK_NSIS_MENU_LINKS 
  "${CPACK_PACKAGE_HOMEPAGE_URL}" "LibreWeb Homepage"
  "https://gitlab.melroy.org/libreweb/browser" "LibreWeb Source code"
)
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\libreweb-browser.exe") # Import: double backlash
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/images/icons/libreweb-browser.ico")
set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/images/icons/libreweb-browser.ico")
set(CPACK_NSIS_MODIFY_PATH ON)

# Detect Linux Distro for RPM files
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux" AND EXISTS "/etc/os-release")
    execute_process (
        COMMAND grep "^NAME=" /etc/os-release
        COMMAND sed -e "s/NAME=//g"
        COMMAND sed -e "s/\"//g"
        RESULT_VARIABLE DIFINE_LINUX_DISTRO_RESULT
        OUTPUT_VARIABLE LINUX_DISTRO
    )
    if (NOT ${DIFINE_LINUX_DISTRO_RESULT} EQUAL 0)
        message (FATAL_ERROR "Linux distro identification error")
    endif ()
endif ()

# RPM section
if(${LINUX_DISTRO} MATCHES "openSUSE")
  # OpenSuse/Leap
  set(CPACK_RPM_PACKAGE_REQUIRES "gtkmm3")
else()
  # Redhat/CentOS/Fedora/etc.
  set(CPACK_RPM_PACKAGE_REQUIRES "gtkmm30")
endif()
# Optional RPM packages (non for now)
set(CPACK_RPM_PACKAGE_SUGGESTS "")

# Debian Jessie/Ubuntu Trusty/Mint Qiana (libgtkmm-3.0-1) or 
# Debian Stretch, Buster or newer, Ubuntu Xenial, Artful, Bionic or newer, Linux Mint Sarah, Tessa, Tina or newer (libgtkmm-3.0-1v5)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgtkmm-3.0-1 | libgtkmm-3.0-1v5")
# Optional deb packages (non for now)
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "")

# include CPack model once all variables are set
include(CPack)

