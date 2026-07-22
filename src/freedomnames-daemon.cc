#include "freedomnames-daemon.h"

#include "freedomnames.h"
#include <glibmm/fileutils.h>
#include <glibmm/main.h>
#include <glibmm/miscutils.h>
#include <glibmm/shell.h>
#include <iostream>
#include <stdexcept>
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

// Loopback endpoint the node is spawned on and probed at. Both must agree:
// the probe answers "is a usable node already on the port I am about to use?".
static const char* kNodeHost = "127.0.0.1";
static const int kNodePort = 8420;
static const char* kNodeDnsAddr = "127.0.0.1:8053";
// Short time-out for the start-up probe, so a missing node does not delay
// start-up. On loopback a closed port is refused immediately anyway.
static const char* kProbeTimeout = "2s";

/**
 * \brief Spawn the freedom-names node in an async manner using Glib.
 *
 * The node's HTTP API and DNS resolver are bound to the loopback interface via
 * CLI flags so nothing is exposed off-host.
 */
void FreedomNamesDaemon::spawn()
{
  // A usable node may already be serving the port we would bind; leave it be.
  if (!FreedomNamesDaemon::adopt_existing_node())
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
        argv.push_back(std::string(kNodeHost) + ":" + std::to_string(kNodePort));
        argv.push_back("--dns-addr");
        argv.push_back(kNodeDnsAddr);

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
  int length, dirname_length;
  length = wai_getExecutablePath(NULL, 0, &dirname_length);
  if (length > 0)
  {
    char* path = static_cast<char*>(malloc(length + 1));
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
 * \brief Check whether an already-running node on our endpoint should be adopted.
 *
 * Probes GET /health on the very address the node would be spawned on. This asks
 * the question that actually matters -- "is a usable node reachable on the port I
 * am about to bind?" -- rather than the old process-name match, which also picked
 * up bootstrap nodes and orphans, and only ever worked on Linux.
 *
 * A bootstrap node is deliberately not adopted: it serves no DNS and is not the
 * node the browser needs. Since node 0.8.4 a bootstrap node defaults to :8430, so
 * this only happens when someone explicitly points one at our port.
 *
 * \return true when an existing node is adopted (do not spawn), false to spawn
 */
bool FreedomNamesDaemon::adopt_existing_node()
{
  FreedomNames probe(kNodeHost, kNodePort, kProbeTimeout);
  FreedomHealth health;
  try
  {
    health = probe.get_health();
  }
  catch (const std::runtime_error&)
  {
    // Nothing listening, or it does not speak the node API: spawn our own.
    return false;
  }

  const std::string endpoint = std::string(kNodeHost) + ":" + std::to_string(kNodePort);
  if (health.role == "bootstrap")
  {
    std::cerr << "WARN: A Freedom Names *bootstrap* node is listening on " << endpoint << ", which LibreWeb needs for its own node." << std::endl
              << "WARN: Move it to another port (bootstrap nodes default to 127.0.0.1:8430 since node 0.8.4)." << std::endl
              << "WARN: Starting our own node anyway; it will fail to bind and name resolution will not work until then." << std::endl;
    return false;
  }

  // role "node", or empty on a node older than 0.8.4 (adopt, as before).
  std::cout << "INFO: Found a running Freedom Names node on " << endpoint << " (version " << (health.version.empty() ? "unknown" : health.version)
            << ", role " << (health.role.empty() ? "unreported" : health.role) << ")." << std::endl
            << "INFO: Reusing it. It was not started by LibreWeb, so it keeps running when the browser exits." << std::endl;
  return true;
}
