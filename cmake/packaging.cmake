
# Example: https://github.com/MariaDB/server/tree/10.5/cmake
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Browser - Decentralized WWW")
set(CPACK_PACKAGE_VENDOR "Melroy van den Berg")
set(CPACK_PACKAGE_CONTACT "Melroy van den Berg <melroy@melroy.org>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://melroy.org")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_TARGET}-${CPACK_PACKAGE_VERSION}")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils") # Change
set(CPACK_RPM_PACKAGE_GROUP      "Applications/Productivity") # Change
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-v${CPACK_PACKAGE_VERSION}") # Without '-Linux' suffix

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

if(${LINUX_DISTRO} MATCHES "openSUSE")
  # OpenSuse/Leap
  set(CPACK_RPM_PACKAGE_REQUIRES "") # Change
else()
  # Redhat/CentOS/Fedora/etc.
  set(CPACK_RPM_PACKAGE_REQUIRES "") # Change
endif()
# Optional RPM packages
set(CPACK_RPM_PACKAGE_SUGGESTS "") # Change

# Debian/Ubuntu/Mint Mint
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5widgets5")
# Optional deb packages
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "") # Change

# include CPack model once all variables are set
include(CPack)