#ifndef FREEDOMNAMES_STATUS_H
#define FREEDOMNAMES_STATUS_H

#include "freedomnames.h"

#include <atomic>
#include <mutex>
#include <sigc++/connection.h>
#include <string>
#include <thread>

class MainWindow;

/**
 * \class FreedomNamesStatus
 * \brief Periodically polls the local Freedom Names node for status information
 * (peers, node ID, mode, network size, version). Shared by all tabs.
 */
class FreedomNamesStatus
{
public:
  FreedomNamesStatus(MainWindow& main_window, const std::string& timeout);
  ~FreedomNamesStatus();

  std::size_t get_number_of_peers() const;
  std::string get_node_id() const;
  std::string get_mode() const;
  int get_network_size() const;
  std::string get_version() const;

private:
  MainWindow& main_window_;
  sigc::connection status_timer_handler_;
  std::thread* status_thread_;              /* Status thread pointer */
  std::atomic<bool> is_status_thread_done_; /* Indication when the status calls are done */
  FreedomNames freedom_status_;             /* Client for status calls, so it doesn't conflict with fetch requests */
  std::size_t number_of_peers_;
  std::string node_id_;
  std::string mode_;
  int network_size_;
  std::string version_;
  mutable std::mutex status_mutex_; /* Protects the status members above; also locked by the const getters */

  void do_status_update_once();
  bool do_status_update();
  void process_status();
  void abort_status();
};

#endif
