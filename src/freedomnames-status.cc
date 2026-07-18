#include "freedomnames-status.h"

#include "main-window.h"
#include <glibmm/main.h>
#include <iostream>

/**
 * FreedomNamesStatus constructor
 * \param main_window Reference to the main window, used to update the status pop-over & icon
 * \param timeout Request time-out setting
 */
FreedomNamesStatus::FreedomNamesStatus(MainWindow& main_window, const std::string& timeout)
    : main_window_(main_window),
      status_thread_(nullptr),
      is_status_thread_done_(false),
      freedom_status_("127.0.0.1", 8420, timeout),
      number_of_peers_(0),
      network_size_(-1)
{
  // First update status manually (with slight delay), after that the timer below will take care of updates
  Glib::signal_timeout().connect_once(sigc::mem_fun(this, &FreedomNamesStatus::do_status_update_once), 550);

  // Create a timer, triggers every 4 seconds
  status_timer_handler_ = Glib::signal_timeout().connect_seconds(sigc::mem_fun(this, &FreedomNamesStatus::do_status_update), 4);
}

/**
 * Destructor
 */
FreedomNamesStatus::~FreedomNamesStatus()
{
  status_timer_handler_.disconnect();
  abort_status();
}

/**
 * \brief Get Freedom Names number of connected peers
 * \return number of peers (size_t)
 */
std::size_t FreedomNamesStatus::get_number_of_peers() const
{
  std::lock_guard<std::mutex> guard(status_mutex_);
  return number_of_peers_;
}

/**
 * \brief Get Freedom Names node ID (libp2p peer ID)
 * \return node ID (string)
 */
std::string FreedomNamesStatus::get_node_id() const
{
  std::lock_guard<std::mutex> guard(status_mutex_);
  return node_id_;
}

/**
 * \brief Get Freedom Names DHT mode (Auto/Client/Server)
 * \return mode (string)
 */
std::string FreedomNamesStatus::get_mode() const
{
  std::lock_guard<std::mutex> guard(status_mutex_);
  return mode_;
}

/**
 * \brief Get estimated Freedom Names network size
 * \return network size (int, -1 if unknown)
 */
int FreedomNamesStatus::get_network_size() const
{
  std::lock_guard<std::mutex> guard(status_mutex_);
  return network_size_;
}

/**
 * \brief Get Freedom Names node version
 * \return version (string)
 */
std::string FreedomNamesStatus::get_version() const
{
  std::lock_guard<std::mutex> guard(status_mutex_);
  return version_;
}

/************************************************
 * Private methods
 ************************************************/

/**
 * \brief Simple wrapper of the method below with void return
 */
void FreedomNamesStatus::do_status_update_once()
{
  do_status_update();
}

/**
 * \brief Timeout slot: Update the Freedom Names node status every x seconds.
 * Process requests inside a separate thread, to avoid blocking the GUI thread.
 * \return always true, when running as a GTK timeout handler
 */
bool FreedomNamesStatus::do_status_update()
{
  // Stop any on-going status calls first, if applicable
  abort_status();

  if (status_thread_ == nullptr)
  {
    status_thread_ = new std::thread(&FreedomNamesStatus::process_status, this);
  }
  // Keep going (never disconnect the timer)
  return true;
}

/**
 * Process the Freedom Names status calls.
 * Runs inside a thread.
 */
void FreedomNamesStatus::process_status()
{
  try
  {
    // Network I/O runs outside the lock, so GUI-thread getters never block on it
    FreedomInfo info = freedom_status_.get_info();
    // Version never changes for a running node; fetch it only until we have it
    std::string version;
    if (get_version().empty())
      version = freedom_status_.get_health().version;
    {
      std::lock_guard<std::mutex> guard(status_mutex_);
      number_of_peers_ = info.peers;
      mode_ = info.mode;
      network_size_ = info.network_size;
      if (node_id_.empty())
        node_id_ = info.node_id;
      if (!version.empty())
        version_ = version;
    }

    // Auto-refresh tabs that are showing the 'Please wait' page, now the node is up
    Glib::signal_idle().connect_once(sigc::mem_fun(main_window_, &MainWindow::refresh_waiting_tabs));

    // Trigger update of all status fields, in a thread-safe manner
    Glib::signal_idle().connect_once(sigc::mem_fun(main_window_, &MainWindow::update_status_popover_and_icon));
  }
  catch (const std::runtime_error& error)
  {
    std::string errorMessage = std::string(error.what());
    if (errorMessage != "Request was aborted")
    {
      // Assume no connection or connection lost; display disconnected
      {
        std::lock_guard<std::mutex> guard(status_mutex_);
        number_of_peers_ = 0;
        mode_ = "";
        network_size_ = -1;
      }
      Glib::signal_idle().connect_once(sigc::mem_fun(main_window_, &MainWindow::update_status_popover_and_icon));
    }
  }
  is_status_thread_done_ = true;
}

/**
 * Abort status calls and stop the thread, if applicable.
 */
void FreedomNamesStatus::abort_status()
{
  if (status_thread_ && status_thread_->joinable())
  {
    if (is_status_thread_done_)
    {
      status_thread_->join();
    }
    else
    {
      // Trigger the thread to stop now.
      // We call the abort method of the Freedom Names client.
      freedom_status_.abort();
      status_thread_->join();
      // Reset states, allowing new threads with new API status calls
      freedom_status_.reset();
    }
    delete status_thread_;
    status_thread_ = nullptr;
    is_status_thread_done_ = false; // reset
  }
}
