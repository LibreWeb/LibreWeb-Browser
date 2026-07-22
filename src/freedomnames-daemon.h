#ifndef FREEDOMNAMES_DAEMON_H
#define FREEDOMNAMES_DAEMON_H

#include <glibmm/spawn.h>
#include <string>

/**
 * \class FreedomNamesDaemon
 * \brief Freedom Names node process class: starts/stops the freedom-names node as
 * a child process, the same way the browser used to manage the IPFS daemon.
 *
 * The node is bound to the loopback interface (127.0.0.1) so its HTTP API is only
 * reachable locally by this browser instance.
 */
class FreedomNamesDaemon
{
public:
  void spawn();
  void stop();
  int get_pid() const;
  sigc::signal<void, int> exited;

protected:
  void child_watch_exit(Glib::Pid pid, int child_status);

private:
  std::string working_dir_ = ""; // cwd
  Glib::Pid pid_ = 0;
  sigc::connection child_watch_connection_handler;

  static std::string locate_binary();
  static bool adopt_existing_node();
};
#endif
