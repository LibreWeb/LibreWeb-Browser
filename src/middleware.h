#ifndef MIDDLEWARE_H
#define MIDDLEWARE_H

#include "freedomnames.h"
#include "middleware-i.h"
#include <atomic>
#include <functional>
#include <glibmm/dispatcher.h>
#include <glibmm/ustring.h>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/* Forward declarations */
struct cmark_node;
class MainWindow;
class Tab;

/**
 * \class Middleware
 * \brief Handles (Freedom Names) network requests and File IO from disk towards the GUI.
 * Each tab has its own middleware instance, so every tab can load its own page.
 */
class Middleware : public MiddlewareInterface
{
public:
  Middleware(MainWindow& main_window, Tab& tab, const std::string& timeout);
  virtual ~Middleware() override;
  void do_request(const std::string& path = std::string(),
                  bool is_set_address_bar = true,
                  bool is_history_request = false,
                  bool is_disable_editor = true,
                  bool is_parse_content = true) override;
  std::string do_add(const std::string& path) override;
  std::string do_add_file(const std::string& path) override;
  void fetch_image(const std::string& path, const std::function<void(const std::string& data)>& callback) override;
  void do_write(const std::string& path, bool is_set_address_and_title = true) override;
  void set_content(const Glib::ustring& content) override;
  Glib::ustring get_content() const override;
  cmark_node* parse_content() const override;
  void reset_content_and_path() override;
  bool is_wait_page_visible() const;

private:
  MainWindow& main_window_;
  Tab& tab_;
  Glib::Dispatcher request_started_;
  Glib::Dispatcher request_finished_;
  // Threading:
  std::thread* request_thread_;                   /* Request thread pointer */
  std::atomic<bool> is_request_thread_done_;      /* Indication when the single request (fetch) is done */
  std::atomic<bool> keep_request_thread_running_; /* Trigger the request thread to stop/continue */

  // Freedom Names node:
  std::string freedom_host_;    /* Freedom Names node host name */
  int freedom_port_;            /* Freedom Names node HTTP API port */
  std::string freedom_timeout_; /* Request time-out setting */
  FreedomNames freedom_fetch_;  /* Client for content/resolve calls */

  // Image fetches: one background thread + dedicated client per image, so
  // inline images load concurrently and never share abort state with the
  // page request. Guarded by image_fetches_mutex_.
  struct ImageFetch
  {
    std::shared_ptr<FreedomNames> client;
    std::thread thread;
    std::shared_ptr<std::atomic<bool>> done;
  };
  std::vector<ImageFetch> image_fetches_;
  std::mutex image_fetches_mutex_;

  // Request & Response:
  std::string request_path_;
  std::string final_request_path_;
  Glib::ustring current_content_;
  std::atomic<bool> wait_page_visible_;

  void process_request(const std::string& path, bool is_parse_content);
  void fetch_from_freedomnames(bool is_parse_content);
  void open_from_disk(bool is_parse_content);
  void abort_request();
  void abort_image_fetches(bool wait);
  static bool validate_utf8(const Glib::ustring& text);
};

#endif
