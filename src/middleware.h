#ifndef MIDDLEWARE_H
#define MIDDLEWARE_H

#include "ipfs.h"
#include "middleware-i.h"
#include <atomic>
#include <glibmm/dispatcher.h>
#include <glibmm/ustring.h>
#include <mutex>
#include <sigc++/connection.h>
#include <string>
#include <thread>

/* Forward declarations */
struct cmark_node;
class MainWindow;

/**
 * \class Middleware
 * \brief Handles (IPFS) network requests and File IO from disk towards the GUI
 */
class Middleware : public MiddlewareInterface
{
public:
  explicit Middleware(MainWindow& main_window, const std::string& timeout);
  virtual ~Middleware() override;
  void do_request(const std::string& path = std::string(),
                  bool is_set_address_bar = true,
                  bool is_history_request = false,
                  bool is_disable_editor = true,
                  bool is_parse_content = true) override;
  std::string do_add(const std::string& path) override;
  void do_write(const std::string& path, bool is_set_address_and_title = true) override;
  void set_content(const Glib::ustring& content) override;
  Glib::ustring get_content() const override;
  cmark_node* parse_content() const override;
  void reset_content_and_path() override;
  std::size_t get_ipfs_number_of_peers() const override;
  int get_ipfs_repo_size() const override;
  std::string get_ipfs_repo_path() const override;
  std::string get_ipfs_incoming_rate() const override;
  std::string get_ipfs_outgoing_rate() const override;
  std::string get_ipfs_version() const override;
  std::string get_ipfs_client_id() const override;
  std::string get_ipfs_client_public_key() const override;

private:
  MainWindow& main_window_;
  Glib::Dispatcher request_started_;
  Glib::Dispatcher request_finished_;
  sigc::connection status_timer_handler_;
  // Threading:
  std::thread* request_thread_;                   /* Request thread pointer */
  std::thread* status_thread_;                    /* Status thread pointer */
  std::atomic<bool> is_request_thread_done_;      /* Indication when the single request (fetch) is done */
  std::atomic<bool> keep_request_thread_running_; /* Trigger the request thread to stop/continue */
  std::atomic<bool> is_status_thread_done_;       /* Indication when the status calls are done */

  // IPFS:
  std::string ipfs_host_;    /* IPFS host name */
  int ipfs_port_;            /* IPFS port number */
  std::string ipfs_timeout_; /* IPFS time-out setting */
  IPFS ipfs_fetch_;          /* IPFS object for fetch calls */
  IPFS ipfs_status_;         /* IPFS object for status calls, so it doesn't conflict with the fetch request */
  std::size_t ipfs_number_of_peers_;
  int ipfs_repo_size_;
  std::string ipfs_repo_path_;
  std::string ipfs_incoming_rate_;
  std::string ipfs_outgoing_rate_;
  std::string ipfs_version_;
  std::string ipfs_client_id_;
  std::string ipfs_client_public_key_;
  std::mutex status_mutex_; /* IPFS status mutex to protect class members */

  // Request & Response:
  std::string request_path_;
  std::string final_request_path_;
  Glib::ustring current_content_;
  bool wait_page_visible_;

  void process_request(const std::string& path, bool is_parse_content);
  void fetch_from_ipfs(bool is_parse_content);
  void open_from_disk(bool is_parse_content);
  void do_ipfs_status_update_once();
  bool do_ipfs_status_update();
  void process_ipfs_status();
  void abort_request();
  void abort_status();
  static bool validate_utf8(const Glib::ustring& text);
};

#endif