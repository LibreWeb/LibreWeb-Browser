#include "ipfs.h"
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <iostream>
#include <string.h>
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>

#ifdef LEGACY_CXX
#include <experimental/filesystem>
namespace n_fs = ::std::experimental::filesystem;
#else
#include <filesystem>
namespace n_fs = ::std::filesystem;
#endif

int IPFS::startIPFSDaemon()
{
    // Be sure to kill any running IPFS daemons
    int res = std::system("killall -q ipfs");
    if (res != 0)
    {
        // ignore
    }

    // Find the IPFS binary
    std::string executable = IPFS::findIPFSBinary();
    std::cout << "INFO: Starting IPFS Daemon, using: " << executable << std::endl;
    if (n_fs::exists(executable))
    {
        /// open /dev/null for writing
        int fd = open("/dev/null", O_WRONLY);
        dup2(fd, 1); // make stdout a copy of fd (> /dev/null)
        dup2(fd, 2); // ..and same with stderr
        close(fd);   // close fd
        // stdout and stderr now write to /dev/null

        // Ready to call exec to start IPFS Daemon
        const char *exe = executable.c_str();
        char *proc[] = {strdup(exe), strdup("daemon"), strdup("--init"), strdup("--migrate"), NULL};
        return execv(exe, proc);
    }
    else
    {
        std::cerr << "Error: IPFS Daemon is not found. IPFS will not work!" << std::endl;
        return -1;
    }
}

std::string IPFS::findIPFSBinary()
{
    // Try absolute path first
    for (std::string data_dir : Glib::get_system_data_dirs())
    {
        std::vector<std::string> path_builder{data_dir, "libreweb-browser", "go-ipfs", "ipfs"};
        std::string ipfs_binary_path = Glib::build_path(G_DIR_SEPARATOR_S, path_builder);
        if (Glib::file_test(ipfs_binary_path, Glib::FileTest::FILE_TEST_IS_EXECUTABLE))
        {
            return ipfs_binary_path;
        }
    }

    // Try local path if the images are not installed (yet)
    // When working directory is in the build/bin folder (relative path)
    std::string currentPath = n_fs::current_path().string();
    std::string ipfs_binary_path = Glib::build_filename(currentPath, "../..", "go-ipfs", "ipfs");
    if (Glib::file_test(ipfs_binary_path, Glib::FileTest::FILE_TEST_IS_EXECUTABLE))
    {
        return ipfs_binary_path;
    }
    else
    {
        return "";
    }
}