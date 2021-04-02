#include "ipfs-process.h"
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

/**
 * \brief Start IPFS Daemon in the background via fork() see: main.cc
 * \return exit code
 */
int IPFSProcess::startIPFSDaemon()
{
    bool startIPFSDaemon = true;
    pid_t daemonPID = IPFSProcess::getRunningDaemonPID();
    // Valid PID?
    if (daemonPID > 0)
    {
        // Kill any running IPFS daemon processes if needed
        if (IPFSProcess::shouldProcessTerminated(daemonPID))
        {
            std::cout << "INFO: Already running ipfs process will be terminated." << std::endl;
            int res = std::system("killall -w -q ipfs");
            if (res != 0)
            {
                // ignore
            }
        }
        else
        {
            // Keep running, don't start another daemon process
             std::cout << "INFO: Keep using the current running IPFS process, with PID: " << std::to_string(daemonPID) << std::endl;
            startIPFSDaemon = false;
        }
    }

    if (startIPFSDaemon)
    {
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
    return 0;
}

/**
 * \brief Retrieve IPFS Daemon PID (zero if non-exists)
 * \return pid_t, 0 if non-exists, -1 on error
 */
pid_t IPFSProcess::getRunningDaemonPID()
{
    pid_t pid = 0; // PID of 0 is by definition not possible
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
            pid = strtoul(pidbuf, NULL, 10);
        }
    }
    else
    {
        pid = -1; // on error
    }
    return pid;
}

/**
 * \brief Determine if the app needs to kill any running IPFS process
 * \param pid Give an actual valid PID to determine if the process needs to be killed (and started again)
 * \return true if it needs to be terminated, otherwise false
 */
bool IPFSProcess::shouldProcessTerminated(pid_t pid)
{
    char pathbuf[1024];
    memset(pathbuf, 0, sizeof(pathbuf));
    std::string path = "/proc/" + std::to_string(pid) + "/exe";
    if (readlink(path.c_str(), pathbuf, sizeof(pathbuf) - 1) > 0)
    {
        char beginPath[] = "/usr/share/libreweb-browser";
        // If the begin path does not path (!= 0), return true,
        // meaning the process will be killed.
        return (strncmp(pathbuf, beginPath, strlen(beginPath)) != 0);
        // TODO: Compare IPFS version as well (via: "ipfs version" command), maybe?
    }
    else
    {
        return true; // error fall-back also kill
    }
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