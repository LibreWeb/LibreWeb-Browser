#ifndef FREEDOMNAMES_STATUS_H
#define FREEDOMNAMES_STATUS_H

#include "freedomnames.h"

#include <atomic>
#include <mutex>
#include <sigc++/connection.h>
#include <string>
#include <thread>
#include <vector>

class MainWindow;

/**
 * \enum NodeState
 * \brief What the last status poll found on the node's HTTP API.
 *
 * Running is the state that carries its weight day to day: a node that is up
 * but has no peers yet used to be reported as "Disconnected", which reads as a
 * broken browser rather than a quiet network.
 *
 * Starting is a guard rather than an observed state. /info answers 500 until
 * the DHT is initialized -- which is why the node docs tell an embedding host to
 * probe /health instead -- but as of node 0.9.x the HTTP listener is only bound
 * after that initialization, so nothing can observe the 500 in practice. It is
 * kept because the alternative is reporting a live node as absent if that
 * ordering ever changes, and the extra /health call only happens on a poll that
 * already failed.
 */
enum class NodeState
{
  Unreachable, /* nothing answered: not started, crashed, or on another port */
  Starting,    /* answered /health with ready=false: the DHT is still coming up */
  Running      /* fully up */
};

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

  NodeState get_state() const;
  std::size_t get_number_of_peers() const;
  std::string get_node_id() const;
  std::string get_mode() const;
  int get_network_size() const;
  std::string get_version() const;
  std::vector<std::string> get_routing_table() const;
  std::vector<std::string> get_listen_addresses() const;
  std::vector<std::string> get_protocols() const;

private:
  MainWindow& main_window_;
  sigc::connection status_timer_handler_;
  std::thread* status_thread_;              /* Status thread pointer */
  std::atomic<bool> is_status_thread_done_; /* Indication when the status calls are done */
  FreedomNames freedom_status_;             /* Client for status calls, so it doesn't conflict with fetch requests */
  NodeState state_;
  std::size_t number_of_peers_;
  std::string node_id_;
  std::string mode_;
  int network_size_;
  std::string version_;
  std::vector<std::string> routing_table_;
  std::vector<std::string> listen_addresses_;
  std::vector<std::string> protocols_;
  mutable std::mutex status_mutex_; /* Protects the status members above; also locked by the const getters */

  void do_status_update_once();
  bool do_status_update();
  void process_status();
  void abort_status();
};

#endif
