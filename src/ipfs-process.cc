#include "ipfs-process.h"
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>

#ifdef LEGACY_CXX
#include <experimental/filesystem>
namespace n_fs = ::std::experimental::filesystem;
#else
#include <filesystem>
namespace n_fs = ::std::filesystem;
#endif

/**
 * \brief Start IPFS Daemon in the background via fork() see: main.cc
 * \return exit code
 */
int IPFSProcess::startIPFSDaemon()
{
    // Kill any running IPFS daemons if needed
    if (IPFSProcess::shouldKillRunningProcess())
    {
        std::cout << "INFO: Already running ipfs process will be terminated." << std::endl;
        int res = std::system("killall -w -q ipfs");
        if (res != 0)
        {
            // ignore
        }
    }

    // Find the IPFS binary
    std::string executable = IPFSProcess::findIPFSBinary();
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

/**
 * \brief Determine if the app needs to kill any running IPFS process
 * \return true if it needs to be terminated, otherwise false
 */
bool IPFSProcess::shouldKillRunningProcess()
{
    FILE *cmd_pipe = popen("pidof -s ipfs", "r");
    if (cmd_pipe != NULL)
    {
        char pidbuf[512];
        memset(pidbuf, 0, sizeof(pidbuf));
        if (fgets(pidbuf, 512, cmd_pipe) == NULL)
        {
            //ignore
        }
        pclose(cmd_pipe);

        if (strlen(pidbuf) > 0)
        {
            pid_t pid = strtoul(pidbuf, NULL, 10);
            char pathbuf[1024];
            memset(pathbuf, 0, sizeof(pathbuf));
            std::string path = "/proc/" + std::to_string(pid) + "/exe";
            if (readlink(path.c_str(), pathbuf, sizeof(pathbuf) - 1) > 0)
            {
                char beginPath[] = "/usr/share/libreweb-browser";
                // If the begin path does not path (!= 0), return true,
                // meaning the process will be killed.
                return (strncmp(pathbuf, beginPath, strlen(beginPath)) != 0);
            }
            // TODO: Compare IPFS version as well, maybe?
        }
        else
        {
            // No running IPFS process
            return false;
        }
    }
    // Something went wrong, fallback is to kill (better safe then sorry)
    return true;
}

/**
 * \brief Try to find the binary location of ipfs (IPFS go server)
 * \return full path to the ipfs binary, empty string when not found
 */
std::string IPFSProcess::findIPFSBinary()
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