#ifndef MIDDLEWARE_H
#define MIDDLEWARE_H

#include "freedomnames.h"
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
 * \brief Handles (Freedom Names) network requests and File IO from disk towards the GUI
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
  std::size_t get_freedom_number_of_peers() const override;
  std::string get_freedom_node_id() const override;
  std::string get_freedom_mode() const override;
  int get_freedom_network_size() const override;

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

  // Freedom Names node:
  std::string freedom_host_;    /* Freedom Names node host name */
  int freedom_port_;            /* Freedom Names node HTTP API port */
  std::string freedom_timeout_; /* Request time-out setting */
  FreedomNames freedom_fetch_;  /* Client for content/resolve calls */
  FreedomNames freedom_status_; /* Client for status calls, so it doesn't conflict with the fetch request */
  std::size_t freedom_number_of_peers_;
  std::string freedom_node_id_;
  std::string freedom_mode_;
  int freedom_network_size_;
  mutable std::mutex status_mutex_; /* Protects the status members above; also locked by the const getters */

  // Request & Response:
  std::string request_path_;
  std::string final_request_path_;
  Glib::ustring current_content_;
  bool wait_page_visible_;

  void process_request(const std::string& path, bool is_parse_content);
  void fetch_from_freedomnames(bool is_parse_content);
  void open_from_disk(bool is_parse_content);
  void do_freedom_status_update_once();
  bool do_freedom_status_update();
  void process_freedom_status();
  void abort_request();
  void abort_status();
  static bool validate_utf8(const Glib::ustring& text);
};

#endif