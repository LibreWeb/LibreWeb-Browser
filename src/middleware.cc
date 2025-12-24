#include "middleware.h"

#include "file.h"
#include "main-window.h"
#include "md-parser.h"
#include <cmark-gfm.h>
#include <glibmm.h>
#include <glibmm/main.h>

/**
 * Middleware constructor
 */
Middleware::Middleware(MainWindow& main_window, const std::string& timeout)
    : main_window_(main_window),
      // Threading:
      request_thread_(nullptr),
      status_thread_(nullptr),
      is_request_thread_done_(false),
      keep_request_thread_running_(true),
      is_status_thread_done_(false),
      // IPFS:
      ipfs_host_("localhost"),
      ipfs_port_(5001),
      ipfs_timeout_(timeout),
      ipfs_fetch_(ipfs_host_, ipfs_port_, ipfs_timeout_),
      ipfs_status_(ipfs_host_, ipfs_port_, ipfs_timeout_),
      ipfs_number_of_peers_(0),
      ipfs_repo_size_(0),
      ipfs_incoming_rate_("0.0"),
      ipfs_outgoing_rate_("0.0"),
      // Request & Response:
      wait_page_visible_(false)
{
  // Hook up signals to Main Window methods
  request_started_.connect(sigc::mem_fun(main_window, &MainWindow::started_request));
  request_finished_.connect(sigc::mem_fun(main_window, &MainWindow::finished_request));

  // First update status manually (with slight delay), after that the timer below will take care of updates
  Glib::signal_timeout().connect_once(sigc::mem_fun(this, &Middleware::do_ipfs_status_update_once), 550);

  // Create a timer, triggers every 4 seconds
  status_timer_handler_ = Glib::signal_timeout().connect_seconds(sigc::mem_fun(this, &Middleware::do_ipfs_status_update), 4);
}

/**
 * Destructor
 */
Middleware::~Middleware()
{
  status_timer_handler_.disconnect();
  abort_request();
  abort_status();
}

/**
 * Fetch document from disk or IPFS, using threading
 * \param path File path that needs to be opened (either from disk or IPFS network)
 * \param is_set_address_bar If true update the address bar with the file path (default: true)
 * \param is_history_request Set to true if this is an history request call: back/forward (default: false)
 * \param is_disable_editor If true the editor will be disabled if needed (default: true)
 * \param is_parse_content If true the content received will be parsed and displayed as markdown syntax (default: true),
 * set to false if you want to editor the content
 */
void Middleware::do_request(const std::string& path, bool is_set_address_bar, bool is_history_request, bool is_disable_editor, bool is_parse_content)
{
  // Stop any on-going request first, if applicable
  abort_request();

  if (request_thread_ == nullptr)
  {
    std::string title;
    if (path.empty() && request_path_.starts_with("file://"))
    {
      title = File::get_filename(request_path_); // During refresh
    }
    else if (path.starts_with("file://"))
    {
      title = File::get_filename(path);
    }
    // Update main window widgets
    main_window_.pre_request(path, title, is_set_address_bar, is_history_request, is_disable_editor);

    // Start thread
    request_thread_ = new std::thread(&Middleware::process_request, this, path, is_parse_content);
  }
  else
  {
    std::cerr << "ERROR: Could not start request thread. Something went wrong." << std::endl;
  }
}

/**
 * \brief Add current content to IPFS
 * \param path file path in IPFS
 * \return Content identifier (CID)
 */
std::string Middleware::do_add(const std::string& path)
{
  // TODO: We should run this within a separate thread, to avoid blocking the main thread.
  // See also the other status calls we are making, but maybe we should use ipfs_fetch_ anyway.
  return ipfs_status_.add(path, get_content());
}

/**
 * \brief Write file to disk
 * \param path file path to disk
 * \param is_set_address_and_title If true update the address bar & title (default: true)
 */
void Middleware::do_write(const std::string& path, bool is_set_address_and_title)
{
  File::write(path, get_content());
  main_window_.post_write("file://" + path, File::get_filename(path), is_set_address_and_title);
}

/**
 * \brief Set current plain-text content (not parsed)
 */
void Middleware::set_content(const Glib::ustring& content)
{
  current_content_ = content;
}

/**
 * \brief Get current plain content (not parsed)
 * \return content as string
 */
Glib::ustring Middleware::get_content() const
{
  return current_content_;
}

/**
 * \brief Current content parser middleware.
 * Note: Do not forget to free the document: cmark_node_free(root_node;
 * \return AST structure (of type cmark_node)
 */
cmark_node* Middleware::parse_content() const
{
  return Parser::parse_content(current_content_);
}

/**
 * \brief Reset state
 */
void Middleware::reset_content_and_path()
{
  current_content_ = "";
  request_path_ = "";
  final_request_path_ = "";
}

/**
 * \brief Get IPFS number of peers
 * \return number of peers (size_t)
 */
std::size_t Middleware::get_ipfs_number_of_peers() const
{
  return ipfs_number_of_peers_;
}

/**
 * \brief Get IPFS repository size
 * \return repo size (int)
 */
int Middleware::get_ipfs_repo_size() const
{
  return ipfs_repo_size_;
}

/**
 * \brief Get IPFS repository path
 * \return repo path (string)
 */
std::string Middleware::get_ipfs_repo_path() const
{
  return ipfs_repo_path_;
}

/**
 * \brief Get IPFS Incoming rate
 * \return incoming rate (string)
 */
std::string Middleware::get_ipfs_incoming_rate() const
{
  return ipfs_incoming_rate_;
}

/**
 * \brief Get IPFS Outgoing rate
 * \return outgoing rate (string)
 */
std::string Middleware::get_ipfs_outgoing_rate() const
{
  return ipfs_outgoing_rate_;
}

/**
 * \brief Get IPFS version
 * \return version (string)
 */
std::string Middleware::get_ipfs_version() const
{
  return ipfs_version_;
}

/**
 * \brief Get IPFS Client ID
 * \return client ID (string)
 */
std::string Middleware::get_ipfs_client_id() const
{
  return ipfs_client_id_;
}

/**
 * \brief Get IPFS Client Public key
 * \return public key (string)
 */
std::string Middleware::get_ipfs_client_public_key() const
{
  return ipfs_client_public_key_;
}

/************************************************
 * Private methods
 ************************************************/

/**
 * \brief Get the file from disk or IPFS network, from the provided path,
 * parse the content, and display the document.
 * Call this method with empty path, will use the previous request_path_ (thus refresh).
 * \param path File path that needs to be fetched (from disk or IPFS network)
 * \param isParseContent Set to true if you want to parse and display the content as markdown syntax (from disk or IPFS
 * network), set to false if you want to edit the content
 */
void Middleware::process_request(const std::string& path, bool isParseContent)
{
  request_started_.emit(); // Emit started for Main Window
  // Reset private variables
  current_content_ = "";
  wait_page_visible_ = false;

  // Do not update the request_path_ when path is empty,
  // this is used for refreshing the page
  if (!path.empty())
  {
    request_path_ = path;
  }

  if (request_path_.empty())
  {
    std::cerr << "Info: Empty request path." << std::endl;
  }
  // Handle homepage
  else if (request_path_.compare("about:home") == 0)
  {
    Glib::signal_idle().connect_once(sigc::mem_fun(main_window_, &MainWindow::show_homepage));
  }
  // Handle disk or IPFS file paths
  else
  {
    // Check if CID
    if (request_path_.starts_with("ipfs://"))
    {
      final_request_path_ = request_path_;
      final_request_path_.erase(0, 7);
      fetch_from_ipfs(isParseContent);
    }
    else if ((request_path_.length() == 46) && request_path_.starts_with("Qm"))
    {
      // CIDv0
      final_request_path_ = request_path_;
      fetch_from_ipfs(isParseContent);
    }
    else if (request_path_.starts_with("file://"))
    {
      final_request_path_ = request_path_;
      final_request_path_.erase(0, 7);
      open_from_disk(isParseContent);
    }
    else
    {
      // IPFS as fallback / CIDv1
      final_request_path_ = request_path_;
      fetch_from_ipfs(isParseContent);
    }
  }

  request_finished_.emit();       // Emit finished for Main Window
  is_request_thread_done_ = true; // mark thread as done
}

/**
 * \brief Helper method for process_request(), display markdown file from IPFS network.
 * Runs in a separate thread.
 * \param isParseContent Set to true if you want to parse and display the content as markdown syntax (from disk or IPFS
 * network), set to false if you want to edit the content
 */
void Middleware::fetch_from_ipfs(bool isParseContent)
{
  try
  {
    std::stringstream contents;
    ipfs_fetch_.fetch(final_request_path_, &contents);
    // If the thread stops, don't brother to parse the file/update the GTK window
    if (keep_request_thread_running_)
    {
      // Retrieve content to string
      Glib::ustring content = contents.str();
      // Only set content if valid UTF-8
      if (Middleware::validate_utf8(content) && keep_request_thread_running_)
      {
        set_content(content);
        if (isParseContent)
        {
          // TODO: Maybe we want to abort the parser when keep_request_thread_running_ = false,
          // depending time the parser is taking?
          cmark_node* doc = parse_content();
          Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_document), doc));
        }
        else
        {
          // Directly display the plain markdown content
          Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_text), get_content()));
        }
      }
      else
      {
        Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message), "😵 File will not be displayed ",
                                                    "File is not valid UTF-8 encoded, like a markdown or text file."));
      }
    }
  }
  catch (const std::runtime_error& error)
  {
    std::string errorMessage = std::string(error.what());
    // Ignore error reporting when the request was aborted
    if (errorMessage != "Request was aborted")
    {
      std::cerr << "ERROR: IPFS request failed, with message: " << errorMessage << std::endl;
      if (errorMessage.starts_with("HTTP request failed with status code"))
      {
        std::string message;
        // Remove text until ':\n'
        errorMessage.erase(0, errorMessage.find(':') + 2);
        if (!errorMessage.empty() && errorMessage != "")
        {
          try
          {
            auto content = nlohmann::json::parse(errorMessage);
            message = "Message: " + content.value("Message", "");
            if (message.starts_with("context deadline exceeded"))
            {
              message += ". Time-out is set to: " + ipfs_timeout_;
            }
            message += ".\n\n";
          }
          catch (const nlohmann::json::parse_error& parseError)
          {
            std::cerr << "ERROR: Could not parse at byte: " << parseError.byte << std::endl;
          }
        }
        Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message),
                                                    "🎂 We're having trouble finding this site.",
                                                    message + "You could try to reload the page or try increase the time-out (see --help)."));
      }
      else if (errorMessage.starts_with("Couldn't connect to server: Failed to connect to localhost"))
      {
        Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message), "⌛ Please wait...",
                                                    "IPFS daemon is still spinnng-up, page will automatically refresh..."));
        wait_page_visible_ = true; // Please wait page is shown (auto-refresh when network is up)
      }
      else
      {
        Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message), "❌ Something went wrong",
                                                    "Error message: " + std::string(error.what())));
      }
    }
  }
}

/**
 * \brief Helper method for process_request(), display markdown file from disk.
 * Runs in a separate thread.
 * \param isParseContent Set to true if you want to parse and display the content as markdown syntax (from disk or IPFS
 * network), set to false if you want to edit the content
 */
void Middleware::open_from_disk(bool isParseContent)
{
  try
  {
    // TODO: Abort file read if keep_request_thread_running_ = false and throw runtime error, to stop further execution
    // eg. when you are reading a very big file from disk.
    const Glib::ustring content = File::read(final_request_path_);
    // If the thread stops, don't brother to parse the file/update the GTK window
    if (keep_request_thread_running_)
    {
      // Only set content if valid UTF-8
      if (Middleware::validate_utf8(content))
      {
        set_content(content);
        if (isParseContent)
        {
          cmark_node* doc = parse_content();
          Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_document), doc));
        }
        else
        {
          // Directly set the plain markdown content
          Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_text), get_content()));
        }
      }
      else
      {
        Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message), "😵 File will not be displayed ",
                                                    "File is not valid UTF-8 encoded, like a markdown file or text file."));
      }
    }
  }
  catch (const std::ios_base::failure& error)
  {
    std::cerr << "ERROR: Could not read file: " << final_request_path_ << ". Message: " << error.what() << ".\nError code: " << error.code()
              << std::endl;
    Glib::signal_idle().connect_once(
        sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message), "🎂 Could not read file", "Message: " + std::string(error.what())));
  }
  catch (const std::runtime_error& error)
  {
    std::cerr << "ERROR: File request failed, file: " << final_request_path_ << ". Message: " << error.what() << std::endl;
    Glib::signal_idle().connect_once(
        sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message), "🎂 File not found", "Message: " + std::string(error.what())));
  }
}


/**
 * \brief Simple wrapper of the method below with void return
 */
void Middleware::do_ipfs_status_update_once()
{
  do_ipfs_status_update();
}

/**
 * \brief Timeout slot: Update the IPFS connection status every x seconds.
 * Process requests inside a separate thread, to avoid blocking the GUI thread.
 * \return always true, when running as a GTK timeout handler
 */
bool Middleware::do_ipfs_status_update()
{
  // Stop any on-going status calls first, if applicable
  abort_status();

  if (status_thread_ == nullptr)
  {
    status_thread_ = new std::thread(&Middleware::process_ipfs_status, this);
  }
  // Keep going (never disconnect the timer)
  return true;
}

/**
 * Process the IPFS status calls.
 * Runs inside a thread.
 */
void Middleware::process_ipfs_status()
{
  std::lock_guard<std::mutex> guard(status_mutex_);
  try
  {
    ipfs_number_of_peers_ = ipfs_status_.get_nr_peers();
    if (ipfs_number_of_peers_ > 0)
    {
      // Auto-refresh page if needed (when 'Please wait' page was shown)
      if (wait_page_visible_)
        Glib::signal_idle().connect_once(sigc::mem_fun(main_window_, &MainWindow::refresh_request));

      std::map<std::string, std::variant<int, std::string>> repoStats = ipfs_status_.get_repo_stats();
      ipfs_repo_size_ = std::get<int>(repoStats.at("repo-size"));
      ipfs_repo_path_ = std::get<std::string>(repoStats.at("path"));

      std::map<std::string, float> rates = ipfs_status_.get_bandwidth_rates();
      char buf[32];
      ipfs_incoming_rate_ = std::string(buf, std::snprintf(buf, sizeof buf, "%.1f", rates.at("in") / 1000.0));
      ipfs_outgoing_rate_ = std::string(buf, std::snprintf(buf, sizeof buf, "%.1f", rates.at("out") / 1000.0));
    }
    else
    {
      ipfs_repo_size_ = 0;
      ipfs_repo_path_ = "";
      ipfs_incoming_rate_ = "0.0";
      ipfs_outgoing_rate_ = "0.0";
    }

    if (ipfs_client_id_.empty())
      ipfs_client_id_ = ipfs_status_.get_client_id();
    if (ipfs_client_public_key_.empty())
      ipfs_client_public_key_ = ipfs_status_.get_client_public_key();
    if (ipfs_version_.empty())
      ipfs_version_ = ipfs_status_.get_version();

    // Trigger update of all status fields, in a thread-safe manner
    Glib::signal_idle().connect_once(sigc::mem_fun(main_window_, &MainWindow::update_status_popover_and_icon));
  }
  catch (const std::runtime_error& error)
  {
    std::string errorMessage = std::string(error.what());
    if (errorMessage != "Request was aborted")
    {
      // Assume no connection or connection lost; display disconnected
      ipfs_number_of_peers_ = 0;
      ipfs_repo_size_ = 0;
      ipfs_repo_path_ = "";
      ipfs_incoming_rate_ = "0.0";
      ipfs_outgoing_rate_ = "0.0";
      Glib::signal_idle().connect_once(sigc::mem_fun(main_window_, &MainWindow::update_status_popover_and_icon));
    }
  }
}

/**
 * Abort request call and stop the thread, if applicable.
 */
void Middleware::abort_request()
{
  if (request_thread_ && request_thread_->joinable())
  {
    if (is_request_thread_done_)
    {
      request_thread_->join();
    }
    else
    {
      // Trigger the thread to stop now.
      // We call the abort method of the IPFS client.
      ipfs_fetch_.abort();
      keep_request_thread_running_ = false;
      request_thread_->join();
      // Reset states, allowing new threads with new API requests/calls
      ipfs_fetch_.reset();
      keep_request_thread_running_ = true;
    }
    delete request_thread_;
    request_thread_ = nullptr;
    is_request_thread_done_ = false; // reset
  }
}

/**
 * Abort status calls and stop the thread, if applicable.
 */
void Middleware::abort_status()
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
      // We call the abort method of the IPFS client.
      ipfs_status_.abort();
      status_thread_->join();
      // Reset states, allowing new threads with new API status calls
      ipfs_status_.reset();
    }
    delete status_thread_;
    status_thread_ = nullptr;
    is_status_thread_done_ = false; // reset
  }
}

/**
 * \brief Validate if text is valid UTF-8.
 * \param text String that needs to be validated
 * \return true if valid UTF-8
 */
bool Middleware::validate_utf8(const Glib::ustring& text)
{
  return text.validate();
}
