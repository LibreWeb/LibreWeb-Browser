#include "middleware.h"

#include "file.h"
#include "main-window.h"
#include "md-parser.h"
#include "tab.h"
#include <cmark-gfm.h>
#include <glibmm.h>
#include <glibmm/main.h>
#include <nlohmann/json.hpp>

/**
 * Middleware constructor
 * \param main_window Reference to the main window, receives the GUI callbacks
 * \param tab Reference to the tab this middleware belongs to
 * \param timeout Freedom Names request time-out setting
 */
Middleware::Middleware(MainWindow& main_window, Tab& tab, const std::string& timeout)
    : main_window_(main_window),
      tab_(tab),
      // Threading:
      request_thread_(nullptr),
      is_request_thread_done_(false),
      keep_request_thread_running_(true),
      // Freedom Names node:
      freedom_host_("127.0.0.1"),
      freedom_port_(8420),
      freedom_timeout_(timeout),
      freedom_fetch_(freedom_host_, freedom_port_, freedom_timeout_),
      // Request & Response:
      wait_page_visible_(false)
{
  // Hook up signals to Main Window methods, for the tab this middleware belongs to
  request_started_.connect(sigc::bind(sigc::mem_fun(main_window, &MainWindow::started_request), &tab_));
  request_finished_.connect(sigc::bind(sigc::mem_fun(main_window, &MainWindow::finished_request), &tab_));
}

/**
 * Destructor
 */
Middleware::~Middleware()
{
  abort_request();
  abort_image_fetches(true);
}

/**
 * Fetch document from disk or the Freedom Names network, using threading
 * \param path File path that needs to be opened (either from disk or the Freedom Names network)
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
  // Also stop in-flight image fetches of the previous page (without blocking on the threads)
  abort_image_fetches(false);

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
    main_window_.pre_request(&tab_, path, title, is_set_address_bar, is_history_request, is_disable_editor);

    // Start thread
    request_thread_ = new std::thread(&Middleware::process_request, this, path, is_parse_content);
  }
  else
  {
    std::cerr << "ERROR: Could not start request thread. Something went wrong." << std::endl;
  }
}

/**
 * \brief Add current content to the Freedom Names content network.
 * \param path file path (currently unused; kept for interface compatibility)
 * \return Content hash (base36 sha2-256 multihash)
 *
 * Uses a dedicated client instance: freedom_fetch_ is owned by the request
 * thread, whose periodic abort()+reset() cycles would cancel an in-flight
 * publish. A local client has no shared abort state.
 * TODO: Run this within a separate thread, to avoid blocking the main thread.
 */
std::string Middleware::do_add(const std::string& path)
{
  (void)path;
  FreedomNames freedom_publish(freedom_host_, freedom_port_, freedom_timeout_);
  return freedom_publish.add_content(get_content());
}

/**
 * \brief Add a file from disk (eg. an image) to the Freedom Names content network.
 * \param path File path on disk
 * \throw std::runtime_error when the file can't be read or the upload fails
 * \return Content hash (base36 sha2-256 multihash)
 *
 * Uses a dedicated client instance for the same reason as do_add() above.
 * TODO: Run this within a separate thread, to avoid blocking the main thread.
 */
std::string Middleware::do_add_file(const std::string& path)
{
  FreedomNames freedom_publish(freedom_host_, freedom_port_, freedom_timeout_);
  return freedom_publish.add_content(File::read(path, true));
}

/**
 * \brief Fetch the raw bytes of an inline resource (eg. an image inside a markdown page) in a
 * background thread. Each fetch gets its own thread + client, so multiple images load concurrently
 * and never share abort state with the page request thread (freedom_fetch_).
 * \param path Image location: a fn:// name, a bare content hash, a file:// path, or a
 * (relative) disk path which is resolved against the current document location
 * \param callback Invoked from the fetch thread with the raw bytes (empty string on failure)
 */
void Middleware::fetch_image(const std::string& path, const std::function<void(const std::string& data)>& callback)
{
  // Reap the threads of fetches that finished earlier
  {
    std::lock_guard<std::mutex> guard(image_fetches_mutex_);
    for (auto it = image_fetches_.begin(); it != image_fetches_.end();)
    {
      if (*(it->done))
      {
        it->thread.join();
        it = image_fetches_.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  auto client = std::make_shared<FreedomNames>(freedom_host_, freedom_port_, freedom_timeout_);
  auto done = std::make_shared<std::atomic<bool>>(false);
  std::string base_path = request_path_; // Current document location, for resolving relative image paths
  std::thread thread(
      [client, done, path, base_path, callback]()
      {
        std::string data;
        try
        {
          if (path.starts_with("file://"))
          {
            data = File::read(path.substr(7), true);
          }
          else if (path.starts_with("fn://") || path.ends_with(".fn") || path.find(".fn/") != std::string::npos)
          {
            std::string name = path;
            if (name.starts_with("fn://"))
              name.erase(0, 5);
            std::stringstream contents;
            // Same routing heuristic as fetch_from_freedomnames(): a Freedom Names
            // *name* always contains dots, a bare content hash never does
            if (name.find('.') == std::string::npos)
              client->get_content(name, &contents);
            else
              client->resolve_content(name, &contents);
            data = contents.str();
          }
          else if (path.find('.') == std::string::npos)
          {
            // Bare content hash
            std::stringstream contents;
            client->get_content(path, &contents);
            data = contents.str();
          }
          else
          {
            // Disk path; resolve a relative path against the current document directory
            std::string file_path = path;
            if (!path.starts_with("/") && base_path.starts_with("file://"))
            {
              std::string dir = base_path.substr(7);
              dir.erase(dir.find_last_of('/') + 1);
              file_path = dir + path;
            }
            data = File::read(file_path, true);
          }
        }
        catch (const std::runtime_error& error)
        {
          std::string errorMessage = std::string(error.what());
          if (errorMessage != "Request was aborted")
          {
            std::cerr << "ERROR: Image fetch failed (" << path << "), with message: " << errorMessage << std::endl;
          }
        }
        callback(data);
        *done = true;
      });
  std::lock_guard<std::mutex> guard(image_fetches_mutex_);
  image_fetches_.push_back(ImageFetch{client, std::move(thread), done});
}

/**
 * \brief Write file to disk
 * \param path file path to disk
 * \param is_set_address_and_title If true update the address bar & title (default: true)
 */
void Middleware::do_write(const std::string& path, bool is_set_address_and_title)
{
  File::write(path, get_content());
  main_window_.post_write(&tab_, "file://" + path, File::get_filename(path), is_set_address_and_title);
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
 * \brief Check if the 'Please wait' page is currently shown (node still spinning-up)
 * \return true when the wait page is visible, meaning the page should be auto-refreshed once the node is up
 */
bool Middleware::is_wait_page_visible() const
{
  return wait_page_visible_;
}

/************************************************
 * Private methods
 ************************************************/

/**
 * \brief Get the file from disk or the Freedom Names network, from the provided path,
 * parse the content, and display the document.
 * Call this method with empty path, will use the previous request_path_ (thus refresh).
 * \param path File path that needs to be fetched (from disk or the Freedom Names network)
 * \param isParseContent Set to true if you want to parse and display the content as markdown syntax (from disk or the
 * Freedom Names network), set to false if you want to edit the content
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
    Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::show_homepage), &tab_));
  }
  // Handle disk or Freedom Names paths
  else
  {
    // Check for a Freedom Names URL
    if (request_path_.starts_with("fn://"))
    {
      final_request_path_ = request_path_;
      final_request_path_.erase(0, 5);
      fetch_from_freedomnames(isParseContent);
    }
    else if (request_path_.starts_with("file://"))
    {
      final_request_path_ = request_path_;
      final_request_path_.erase(0, 7);
      open_from_disk(isParseContent);
    }
    else if (request_path_.ends_with(".fn") || request_path_.find(".fn/") != std::string::npos)
    {
      // Bare Freedom Names name without the scheme prefix
      final_request_path_ = request_path_;
      fetch_from_freedomnames(isParseContent);
    }
    else
    {
      // Freedom Names as fallback
      final_request_path_ = request_path_;
      fetch_from_freedomnames(isParseContent);
    }
  }

  request_finished_.emit();       // Emit finished for Main Window
  is_request_thread_done_ = true; // mark thread as done
}

/**
 * \brief Helper method for process_request(), display markdown page from the Freedom Names network.
 * Runs in a separate thread.
 * \param isParseContent Set to true if you want to parse and display the content as markdown syntax,
 * set to false if you want to edit the content
 */
void Middleware::fetch_from_freedomnames(bool isParseContent)
{
  try
  {
    std::stringstream contents;
    // A Freedom Names *name* ("label.<pubKeyID>.fn") always contains dots; a bare
    // content hash (base36 multihash, e.g. from the publish dialog) never does.
    if (final_request_path_.find('.') == std::string::npos)
    {
      freedom_fetch_.get_content(final_request_path_, &contents);
    }
    else
    {
      freedom_fetch_.resolve_content(final_request_path_, &contents);
    }
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
          Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_document), &tab_, doc));
        }
        else
        {
          // Directly display the plain markdown content
          Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_text), &tab_, get_content()));
        }
      }
      else
      {
        Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message), &tab_,
                                                    Glib::ustring("😵 File will not be displayed "),
                                                    Glib::ustring("File is not valid UTF-8 encoded, like a markdown or text file.")));
      }
    }
  }
  catch (const std::runtime_error& error)
  {
    std::string errorMessage = std::string(error.what());
    // Ignore error reporting when the request was aborted
    if (errorMessage != "Request was aborted")
    {
      std::cerr << "ERROR: Freedom Names request failed, with message: " << errorMessage << std::endl;
      if (errorMessage.starts_with("HTTP request failed with status code"))
      {
        // Format: "HTTP request failed with status code <N>: <body>" (see FreedomNames::http_get).
        std::string message;
        std::string body = errorMessage.substr(errorMessage.find(':') + 2);
        if (!body.empty())
        {
          // The node returns either a JSON {"error":"..."} body or a plain-text reason.
          try
          {
            auto content = nlohmann::json::parse(body);
            message = "Message: " + content.value("error", body) + ".\n\n";
          }
          catch (const nlohmann::json::parse_error&)
          {
            message = "Message: " + body + ".\n\n";
          }
        }
        else
        {
          message = "Message: " + errorMessage + ".\n\n";
        }
        Glib::signal_idle().connect_once(
            sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message), &tab_, Glib::ustring("🎂 We're having trouble finding this site."),
                       Glib::ustring(message + "You could try to reload the page or try increase the time-out (see --help).")));
      }
      else if (errorMessage.starts_with("Request timed out"))
      {
        Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message), &tab_,
                                                    Glib::ustring("⏰ Request timed out"),
                                                    Glib::ustring("The lookup took too long. Time-out is set to: " + freedom_timeout_ +
                                                                  ".\n\nYou could try to reload the page or increase the time-out (see --help).")));
      }
      else if (errorMessage.starts_with("Couldn't connect to server"))
      {
        Glib::signal_idle().connect_once(
            sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message), &tab_, Glib::ustring("⌛ Please wait..."),
                       Glib::ustring("The Freedom Names node is still spinning-up, page will automatically refresh...")));
        wait_page_visible_ = true; // Please wait page is shown (auto-refresh when network is up)
      }
      else
      {
        Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message), &tab_,
                                                    Glib::ustring("❌ Something went wrong"),
                                                    Glib::ustring("Error message: " + std::string(error.what()))));
      }
    }
  }
}

/**
 * \brief Helper method for process_request(), display markdown file from disk.
 * Runs in a separate thread.
 * \param isParseContent Set to true if you want to parse and display the content as markdown syntax (from disk or the
 * Freedom Names network), set to false if you want to edit the content
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
          Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_document), &tab_, doc));
        }
        else
        {
          // Directly set the plain markdown content
          Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_text), &tab_, get_content()));
        }
      }
      else
      {
        Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message), &tab_,
                                                    Glib::ustring("😵 File will not be displayed "),
                                                    Glib::ustring("File is not valid UTF-8 encoded, like a markdown file or text file.")));
      }
    }
  }
  catch (const std::ios_base::failure& error)
  {
    std::cerr << "ERROR: Could not read file: " << final_request_path_ << ". Message: " << error.what() << ".\nError code: " << error.code()
              << std::endl;
    Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message), &tab_, Glib::ustring("🎂 Could not read file"),
                                                Glib::ustring("Message: " + std::string(error.what()))));
  }
  catch (const std::runtime_error& error)
  {
    std::cerr << "ERROR: File request failed, file: " << final_request_path_ << ". Message: " << error.what() << std::endl;
    Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(main_window_, &MainWindow::set_message), &tab_, Glib::ustring("🎂 File not found"),
                                                Glib::ustring("Message: " + std::string(error.what()))));
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
      // We call the abort method of the Freedom Names client.
      freedom_fetch_.abort();
      keep_request_thread_running_ = false;
      request_thread_->join();
      // Reset states, allowing new threads with new API requests/calls
      freedom_fetch_.reset();
      keep_request_thread_running_ = true;
    }
    delete request_thread_;
    request_thread_ = nullptr;
    is_request_thread_done_ = false; // reset
  }
}

/**
 * \brief Abort all in-flight image fetches (on page navigation or shutdown)
 * \param wait If true also join the fetch threads (used by the destructor); if false let them
 * wind down in the background - they are reaped on the next fetch_image() call
 */
void Middleware::abort_image_fetches(bool wait)
{
  std::lock_guard<std::mutex> guard(image_fetches_mutex_);
  for (ImageFetch& fetch : image_fetches_)
  {
    fetch.client->abort();
  }
  if (wait)
  {
    for (ImageFetch& fetch : image_fetches_)
    {
      if (fetch.thread.joinable())
        fetch.thread.join();
    }
    image_fetches_.clear();
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
