#include "ipfs-daemon.h"

#include <glibmm/fileutils.h>
#include <glibmm/main.h>
#include <glibmm/miscutils.h>
#include <glibmm/shell.h>
#include <iostream>
#include <unistd.h>
#include <whereami.h>

#ifdef LEGACY_CXX
#include <experimental/filesystem>
namespace n_fs = ::std::experimental::filesystem;
#else
#include <filesystem>
namespace n_fs = ::std::filesystem;
#endif
#ifdef _WIN32
#include <windows.h>
#endif

/**
 * \brief Spawn the IPFS daemon in an async manner using Glib. If needed under Linux (under Windows, it tries to start
 * IPFS anyway).
 */
void IPFSDaemon::spawn()
{
  // Check for PID under UNIX
  int daemon_pid = IPFSDaemon::get_existing_pid();
  // Is IPFS Daemon already running?
  // cppcheck-suppress knownConditionTrueFalse
  if (daemon_pid > 0)
  {
    std::cout << "INFO: IPFS Daemon is already running. Do not start another IPFS process." << std::endl;
  }
  else
  {
    std::string command = IPFSDaemon::locate_ipfs_binary();
    if (n_fs::exists(command))
    {
      std::cout << "INFO: Starting IPFS Daemon: " << command << "..." << std::endl;
      try
      {
        // IPFS command
        std::vector<std::string> argv;
        argv.push_back(command);
        argv.push_back("daemon");
        argv.push_back("--init");
        argv.push_back("--migrate");

        // Spawn flags
        // Send stdout & stderr to /dev/null. Don't reaped the child automatically
        Glib::SpawnFlags flags =
            Glib::SPAWN_STDOUT_TO_DEV_NULL | Glib::SPAWN_STDERR_TO_DEV_NULL | Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_SEARCH_PATH;

        // Start IPFS using spawn_async_with_pipes,
        // so we also retrieve stdout & stderr.
        // spawn_async() is also fine
        Glib::spawn_async(working_dir_, argv, flags, Glib::SlotSpawnChildSetup(), &pid_);

        if (child_watch_connection_handler.connected())
          child_watch_connection_handler.disconnect();

        child_watch_connection_handler = Glib::signal_child_watch().connect(sigc::mem_fun(*this, &IPFSDaemon::child_watch_exit), pid_);
      }
      catch (Glib::SpawnError& error)
      {
        std::cerr << "ERROR: IPFS process could not be started. Reason: " << error.what() << std::endl;
      }
      catch (Glib::ShellError& error)
      {
        std::cerr << "ERROR: IPFS process could not be started. Reason: " << error.what() << std::endl;
      }
    }
    else
    {
      std::cerr << "ERROR: IPFS Daemon is not found. IPFS will most likely not work!" << std::endl;
    }
  }
}

/**
 * \brief Stop process manually
 */
void IPFSDaemon::stop()
{
  if (pid_ != 0)
    Glib::spawn_close_pid(pid_);
  child_watch_connection_handler.disconnect();
}

/**
 * \brief Exit signal handler for the process.
 * Emits the exited signal with the status code.
 *
 * Avoid using this-> calls, this will lead to segmention faults
 */
void IPFSDaemon::child_watch_exit(Glib::Pid pid, int child_status)
{
  std::cout << "WARN: IPFS Daemon exited, PID: " << pid << ", with status code: " << child_status << std::endl;
  Glib::spawn_close_pid(pid);
  // Emit exit signal with status code
  exited.emit(child_status);
}

/**
 * \brief Get Process ID (PID)
 * \return PID
 */
int IPFSDaemon::get_pid() const
{
  if (pid_ == 0)
    return 0;
#ifdef _WIN32
  return GetProcessId(pid_);
#else
  return pid_;
#endif
}

/**
 * \brief Try to locate the ipfs binary path (IPFS go server)
 * \return full path to the ipfs binary, empty string when not found
 */
std::string IPFSDaemon::locate_ipfs_binary()
{
  std::string ipfs_binary_name = "ipfs";
  std::string current_executable_path;
#if defined(_WIN32)
  ipfs_binary_name += ".exe";
#elif defined(__APPLE__)
  ipfs_binary_name += "-darwin";
#endif
  // Use the current executable directory (bin folder), to locate the go-ipfs binary
  // (for Linux, Mac OS and Windows)
  char* path = NULL;
  int length, dirname_length;
  length = wai_getExecutablePath(NULL, 0, &dirname_length);
  if (length > 0)
  {
    path = static_cast<char*>(malloc(length + 1));
    if (!path)
    {
      std::cerr << "ERROR: Couldn't create executable path." << std::endl;
    }
    else
    {
      wai_getExecutablePath(path, length, &dirname_length);
      path[dirname_length] = '\0';
      current_executable_path = std::string(path);
      free(path);
    }
  }
  std::string ipfs_binary_path = Glib::build_filename(current_executable_path, ipfs_binary_name);

  // When working directory is the build/bin folder (relative path), during the build
  // (when package is not installed yet)
  std::string ipfs_binary_path_dev = Glib::build_filename(n_fs::current_path().string(), "..", "..", "go-ipfs", ipfs_binary_name);
  if (Glib::file_test(ipfs_binary_path, Glib::FileTest::FILE_TEST_IS_EXECUTABLE))
  {
    return ipfs_binary_path;
  }
  else if (Glib::file_test(ipfs_binary_path_dev, Glib::FileTest::FILE_TEST_IS_EXECUTABLE))
  {
    return ipfs_binary_path_dev;
  }
  else
  {
    return "";
  }
}

/**
 * \brief Retrieve existing running IPFS PID for **UNIX only** (zero if non-existent)
 * \return Process ID (0 of non-existent)
 */
int IPFSDaemon::get_existing_pid()
{
  int pid = 0;
#ifdef __linux__
  int exitCode = -3;
  std::string stdout;
  try
  {
    Glib::spawn_command_line_sync("pidof -s ipfs", &stdout, nullptr, &exitCode);
    // Process exists
    if (exitCode == 0)
    {
      pid = std::stoi(stdout);
    }
  }
  catch (Glib::SpawnError& error)
  {
    std::cerr << "ERROR: Could not check of running IPFS process. Reason: " << error.what() << std::endl;
  }
  catch (Glib::ShellError& error)
  {
    std::cerr << "ERROR: Could not check of running IPFS process. Reason: " << error.what() << std::endl;
  }
#endif
  return pid;
}

/**
 * Determine if we need to kill any running IPFS process (UNIX only)
 *
 * return - true if it needs to be terminated, otherwise false
 *
 * Should we even want this? We were using:

bool IPFSDaemon::should_process_terminated()
{
#ifdef __linux__
    char pathbuf[1024];
    memset(pathbuf, 0, sizeof(pathbuf));
    std::string path = "/proc/" + std::to_string(pid) + "/exe";
    if (readlink(path.c_str(), pathbuf, sizeof(pathbuf) - 1) > 0)
    {
        char begin_path[] = "/usr/share/libreweb-browser";
        // If the begin path does not match (!= 0), return true,
        // meaning the process will be killed.
        bool should_kill = (strncmp(pathbuf, begin_path, strlen(begin_path)) != 0);

        // Also check the IPFS version
        try {
            std::string expected_string = "version 0.11.0";
            std::string stdout;
            Glib::spawn_command_line_sync(path + " version", &stdout);
            // Current running IPFS version matches our IPFS version, keep process running afterall
            if (stdout.find(expected_string) != std::string::npos)
            {
                should_kill = false;
            }
        }
        catch (Glib::SpawnError &error)
        {
            std::cerr << "ERROR: Could not check IPFS version. Reason: " << error.what() << std::endl;
        }
        catch (Glib::ShellError &error)
        {
            std::cerr << "ERROR: Could not check IPFS version. Reason: " << error.what() << std::endl;
        }
        return should_kill;
    }
#endif
    return false; // fallback; do not kill
}*/