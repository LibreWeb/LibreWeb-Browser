#ifndef IPFS_DAEMON_H
#define IPFS_DAEMON_H

#include <glibmm/spawn.h>
#include <string>

/**
 * \class IPFSDaemon
 * \brief IPFS Daemon process class to start/stop IPFS daemon as a child process
 */
class IPFSDaemon
{
public:
  void spawn();
  void stop();
  int get_pid() const;
  sigc::signal<void, int> exited;

protected:
  // Signals
  void child_watch_exit(Glib::Pid pid, int child_status);

private:
  std::string working_dir_ = ""; // cwd
  Glib::Pid pid_ = 0;
  sigc::connection child_watch_connection_handler;

  static std::string locate_ipfs_binary();
  static int get_existing_pid();
  // bool should_process_terminated();
};
#endif