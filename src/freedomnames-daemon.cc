#include "freedomnames-daemon.h"

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

// Binary name of the freedom-names node shipped alongside the browser.
static const char* kFreedomBinaryName = "freedom-names";

/**
 * \brief Spawn the freedom-names node in an async manner using Glib.
 *
 * The node's HTTP API and DNS resolver are bound to the loopback interface via
 * CLI flags so nothing is exposed off-host.
 */
void FreedomNamesDaemon::spawn()
{
  int daemon_pid = FreedomNamesDaemon::get_existing_pid();
  // cppcheck-suppress knownConditionTrueFalse
  if (daemon_pid > 0)
  {
    std::cout << "INFO: Freedom Names node is already running. Do not start another process." << std::endl;
  }
  else
  {
    std::string command = FreedomNamesDaemon::locate_binary();
    if (n_fs::exists(command))
    {
      std::cout << "INFO: Starting Freedom Names node: " << command << "..." << std::endl;
      try
      {
        // Bind the node's services to loopback only. The CLI flags take
        // precedence over any FREEDOM_* environment variables (see
        // freedom-names main.go applyNodeFlags), so the parent environment can
        // be passed along untouched.
        std::vector<std::string> argv;
        argv.push_back(command);
        argv.push_back("--http-addr");
        argv.push_back("127.0.0.1:8420");
        argv.push_back("--dns-addr");
        argv.push_back("127.0.0.1:8053");

        // Send stdout & stderr to /dev/null. Don't reap the child automatically.
        Glib::SpawnFlags flags =
            Glib::SPAWN_STDOUT_TO_DEV_NULL | Glib::SPAWN_STDERR_TO_DEV_NULL | Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_SEARCH_PATH;

        Glib::spawn_async(working_dir_, argv, flags, Glib::SlotSpawnChildSetup(), &pid_);

        if (child_watch_connection_handler.connected())
          child_watch_connection_handler.disconnect();

        child_watch_connection_handler = Glib::signal_child_watch().connect(sigc::mem_fun(*this, &FreedomNamesDaemon::child_watch_exit), pid_);
      }
      catch (Glib::SpawnError& error)
      {
        std::cerr << "ERROR: Freedom Names node could not be started. Reason: " << error.what() << std::endl;
      }
      catch (Glib::ShellError& error)
      {
        std::cerr << "ERROR: Freedom Names node could not be started. Reason: " << error.what() << std::endl;
      }
    }
    else
    {
      std::cerr << "ERROR: Freedom Names node binary is not found. Name resolution will most likely not work!" << std::endl;
    }
  }
}

/**
 * \brief Stop process manually
 */
void FreedomNamesDaemon::stop()
{
  if (pid_ != 0)
    Glib::spawn_close_pid(pid_);
  child_watch_connection_handler.disconnect();
}

/**
 * \brief Exit signal handler for the process.
 *
 * Avoid using this-> calls, this will lead to segmentation faults.
 */
void FreedomNamesDaemon::child_watch_exit(Glib::Pid pid, int child_status)
{
  std::cout << "WARN: Freedom Names node exited, PID: " << pid << ", with status code: " << child_status << std::endl;
  Glib::spawn_close_pid(pid);
  exited.emit(child_status);
}

/**
 * \brief Get Process ID (PID)
 * \return PID
 */
int FreedomNamesDaemon::get_pid() const
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
 * \brief Try to locate the freedom-names binary path.
 * \return full path to the binary, empty string when not found
 */
std::string FreedomNamesDaemon::locate_binary()
{
  std::string binary_name = kFreedomBinaryName;
  std::string current_executable_path;
#if defined(_WIN32)
  binary_name += ".exe";
#elif defined(__APPLE__)
  // Matches the binary name installed by CMake on macOS (freedom-names-darwin)
  binary_name += "-darwin";
#endif
  // Use the current executable directory (bin folder) to locate the node binary.
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
  std::string binary_path = Glib::build_filename(current_executable_path, binary_name);

  // When the working directory is build/bin (during development, before install),
  // fall back to a checked-out freedom-names binary next to the repo.
  std::string binary_path_dev = Glib::build_filename(n_fs::current_path().string(), "..", "..", "freedom-names", binary_name);
  if (Glib::file_test(binary_path, Glib::FileTest::FILE_TEST_IS_EXECUTABLE))
  {
    return binary_path;
  }
  else if (Glib::file_test(binary_path_dev, Glib::FileTest::FILE_TEST_IS_EXECUTABLE))
  {
    return binary_path_dev;
  }
  else
  {
    return "";
  }
}

/**
 * \brief Retrieve existing running freedom-names PID for **UNIX only** (zero if non-existent)
 * \return Process ID (0 if non-existent)
 */
int FreedomNamesDaemon::get_existing_pid()
{
  int pid = 0;
#ifdef __linux__
  int exitCode = -3;
  std::string stdout_str;
  try
  {
    Glib::spawn_command_line_sync("pidof -s freedom-names", &stdout_str, nullptr, &exitCode);
    if (exitCode == 0)
    {
      pid = std::stoi(stdout_str);
    }
  }
  catch (Glib::SpawnError& error)
  {
    std::cerr << "ERROR: Could not check for running Freedom Names process. Reason: " << error.what() << std::endl;
  }
  catch (Glib::ShellError& error)
  {
    std::cerr << "ERROR: Could not check for running Freedom Names process. Reason: " << error.what() << std::endl;
  }
#endif
  return pid;
}
