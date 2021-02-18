#include "mainwindow.h"

#include <gtkmm/menuitem.h>
#include <gtkmm/image.h>
#include <cmark-gfm.h>
#include <pthread.h>
#include <iostream>
#include <nlohmann/json.hpp>

#include "md-parser.h"
#include "menu.h"

MainWindow::MainWindow()
    : accelGroup(Gtk::AccelGroup::create()),
      m_menu(accelGroup),
      m_draw(*this),
      m_vbox(Gtk::ORIENTATION_VERTICAL, 0),
      m_hboxBar(Gtk::ORIENTATION_HORIZONTAL, 0),
      m_hboxBottom(Gtk::ORIENTATION_HORIZONTAL, 0),
      m_requestThread(nullptr),
      requestPath(""),
      finalRequestPath(""),
      currentContent(""),
      currentHistoryIndex(0)
{
    set_title("DWeb Browser");
    set_default_size(1000, 800);
    set_position(Gtk::WIN_POS_CENTER);
    add_accel_group(accelGroup);

    // Connect signals
    m_menu.new_doc.connect(sigc::mem_fun(this, &MainWindow::new_doc));                                               /*!< Menu item for new document */
    m_menu.quit.connect(sigc::mem_fun(this, &MainWindow::hide));                                                     /*!< hide main window and therefor closes the app */
    m_menu.cut.connect(sigc::mem_fun(m_draw, &Draw::cut));                                                           /*!< Menu item for cut text */
    m_menu.copy.connect(sigc::mem_fun(m_draw, &Draw::copy));                                                         /*!< Menu item for copy text */
    m_menu.paste.connect(sigc::mem_fun(m_draw, &Draw::paste));                                                       /*!< Menu item for paste text */
    m_menu.del.connect(sigc::mem_fun(m_draw, &Draw::del));                                                           /*!< Menu item for deleting selected text */
    m_menu.select_all.connect(sigc::mem_fun(m_draw, &Draw::selectAll));                                              /*!< Menu item for selecting all text */
    m_menu.find.connect(sigc::mem_fun(this, &MainWindow::show_search));                                              /*!< Menu item for finding text */
    m_menu.back.connect(sigc::mem_fun(this, &MainWindow::back));                                                     /*!< Menu item for previous page */
    m_menu.forward.connect(sigc::mem_fun(this, &MainWindow::forward));                                               /*!< Menu item for next page */
    m_menu.reload.connect(sigc::mem_fun(this, &MainWindow::refresh));                                                /*!< Menu item for reloading the page */
    m_menu.home.connect(sigc::mem_fun(this, &MainWindow::go_home));                                                  /*!< Menu item for home page */
    m_menu.source_code.connect(sigc::mem_fun(this, &MainWindow::show_source_code_dialog));                           /*!< Source code dialog */
    m_sourceCodeDialog.signal_response().connect(sigc::mem_fun(m_sourceCodeDialog, &SourceCodeDialog::hide_dialog)); /*!< Close source code dialog */
    m_menu.about.connect(sigc::mem_fun(m_about, &About::show_about));                                                /*!< Display about dialog */
    m_draw.source_code.connect(sigc::mem_fun(this, &MainWindow::show_source_code_dialog));                           /*!< Open source code dialog */
    m_about.signal_response().connect(sigc::mem_fun(m_about, &About::hide_about));                                   /*!< Close about dialog */
    m_backButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::back));                                   /*!< Button for previous page */
    m_forwardButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::forward));                             /*!< Button for next page */
    m_refreshButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::refresh));                             /*!< Button for reloading the page */
    m_homeButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::go_home));                                /*!< Button for home page */
    m_addressBar.signal_activate().connect(sigc::mem_fun(this, &MainWindow::address_bar_activate));                  /*!< User pressed enter the address bar */
    m_searchEntry.signal_activate().connect(sigc::mem_fun(this, &MainWindow::do_search));                            /*!< Execute the text search */

    m_vbox.pack_start(m_menu, false, false, 0);

    // Horizontal bar
    auto styleBack = m_backButton.get_style_context();
    styleBack->add_class("circular");
    auto styleForward = m_forwardButton.get_style_context();
    styleForward->add_class("circular");
    auto styleRefresh = m_refreshButton.get_style_context();
    styleRefresh->add_class("circular");
    m_backButton.set_relief(Gtk::RELIEF_NONE);
    m_forwardButton.set_relief(Gtk::RELIEF_NONE);
    m_refreshButton.set_relief(Gtk::RELIEF_NONE);
    m_homeButton.set_relief(Gtk::RELIEF_NONE);

    // Add icons to buttons
    backIcon.set_from_icon_name("go-previous", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_backButton.add(backIcon);
    forwardIcon.set_from_icon_name("go-next", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_forwardButton.add(forwardIcon);
    refreshIcon.set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_refreshButton.add(refreshIcon);
    homeIcon.set_from_icon_name("go-home", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_homeButton.add(homeIcon);

    // Disable back/forward button on start-up
    m_backButton.set_sensitive(false);
    m_forwardButton.set_sensitive(false);

    m_hboxBar.pack_start(m_backButton, false, false, 0);
    m_hboxBar.pack_start(m_forwardButton, false, false, 0);
    m_hboxBar.pack_start(m_refreshButton, false, false, 0);
    m_hboxBar.pack_start(m_homeButton, false, false, 0);
    m_hboxBar.pack_start(m_addressBar, true, true, 8);
    m_vbox.pack_start(m_hboxBar, false, false, 6);

    // Browser text drawing area
    m_scrolledWindow.add(m_draw);
    m_scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    // Bottom Search bar
    m_search.connect_entry(m_searchEntry);
    m_exitBottomButton.set_relief(Gtk::RELIEF_NONE);
    m_exitBottomButton.set_label("\u2716");
    m_exitBottomButton.signal_clicked().connect(sigc::mem_fun(m_hboxBottom, &Gtk::Box::hide));
    m_hboxBottom.pack_start(m_exitBottomButton, false, false, 10);
    m_hboxBottom.pack_start(m_searchEntry, false, false, 10);
    m_vbox.pack_start(m_scrolledWindow, true, true, 0);
    m_vbox.pack_end(m_hboxBottom, false, true, 6);

    add(m_vbox);
    show_all_children();
    // Hide bottom horizontal bar by default
    m_hboxBottom.hide();

    // Grap focus to input field by default
    m_addressBar.grab_focus();

    // Show start page by default
    // Load test.md file on start-up
    doRequest("file:///media/melroy/Data/Projects/browser/test.md", true);
    //go_home();
}

/**
 * Fetch document from disk or IPFS, using threading
 */
void MainWindow::doRequest(const std::string &path, bool setAddressBar, bool isHistoryRequest)
{
    if (m_requestThread)
    {
        if (m_requestThread->joinable())
        {
            pthread_cancel(m_requestThread->native_handle());
            m_requestThread->join();
            delete m_requestThread;
            m_requestThread = nullptr;
        }
    }

    if (m_requestThread == nullptr)
    {
        m_requestThread = new std::thread(&MainWindow::processRequest, this, path);
        postDoRequest(path, setAddressBar, isHistoryRequest);
    }
}

/**
 * Post processing request actions
 */
void MainWindow::postDoRequest(const std::string &path, bool setAddressBar, bool isHistoryRequest)
{
    if (setAddressBar)
        m_addressBar.set_text(path);

    // Do not insert history back/forward calls into the history (again)
    if (!isHistoryRequest)
    {
        if (history.size() == 0)
        {
            history.push_back(path);
            currentHistoryIndex = history.size() - 1;
        }
        else if (history.size() > 0 && !path.empty() && (history.back().compare(path) != 0))
        {
            history.push_back(path);
            currentHistoryIndex = history.size() - 1;
        }
    }

    // Enable back/forward buttons when possible
    m_backButton.set_sensitive(currentHistoryIndex > 0);
    m_menu.setBackMenuSensitive(currentHistoryIndex > 0);
    m_forwardButton.set_sensitive(currentHistoryIndex < history.size() - 1);
    m_menu.setForwardMenuSensitive(currentHistoryIndex < history.size() - 1);
}

void MainWindow::new_doc()
{
    // Inform about new document in draw
    m_draw.newDocument();
}

void MainWindow::go_home()
{
    this->requestPath = "";
    this->finalRequestPath = "";
    this->currentContent = "";
    this->m_addressBar.set_text("");
    m_draw.showStartPage();
}

/**
 * Trigger when pressed enter in the search entry
 */
void MainWindow::do_search()
{
    std::cout << "Search for: " << m_searchEntry.get_text() << std::endl;
}

/**
 * Trigger when pressed enter in the address bar
 */
void MainWindow::address_bar_activate()
{
    doRequest(m_addressBar.get_text());
}

void MainWindow::show_search()
{
    if (m_hboxBottom.is_visible())
    {
        m_hboxBottom.hide();
        m_addressBar.grab_focus();
    }
    else
    {
        m_hboxBottom.show();
        m_searchEntry.grab_focus();
    }
}

void MainWindow::back()
{
    if (currentHistoryIndex > 0)
    {
        currentHistoryIndex--;
        doRequest(history.at(currentHistoryIndex), true, true);
    }
}

void MainWindow::forward()
{
    if (currentHistoryIndex < history.size() - 1)
    {
        currentHistoryIndex++;
        doRequest(history.at(currentHistoryIndex), true, true);
    }
}

void MainWindow::refresh()
{
    doRequest();
}

/**
 * Get the file from disk or IPFS network, from the provided path,
 * parse the content, and display the document
 */
void MainWindow::processRequest(const std::string &path)
{
    currentContent = "";
    if (!path.empty())
    {
        requestPath = path;
    }
    if (requestPath.empty())
    {
        std::cerr << "Info: Empty request path." << std::endl;
    }
    else
    {
        // Check if CID
        if (requestPath.rfind("ipfs://", 0) == 0)
        {
            finalRequestPath = requestPath;
            finalRequestPath.erase(0, 7);
            fetchFromIPFS();
        }
        else if ((requestPath.length() == 46) && (requestPath.rfind("Qm", 0) == 0))
        {
            // CIDv0
            finalRequestPath = requestPath;
            fetchFromIPFS();
        }
        else if (requestPath.rfind("file://", 0) == 0)
        {
            finalRequestPath = requestPath;
            finalRequestPath.erase(0, 7);
            openFromDisk();
        }
        else
        {
            // IPFS as fallback / CIDv1
            finalRequestPath = requestPath;
            fetchFromIPFS();
        }
    }
}

/**
 * Helper method for processRequest(),
 * Display markdown file from IPFS network.
 */
void MainWindow::fetchFromIPFS()
{
    // TODO: Execute the code in a seperate thread/process?
    //  Since otherwise this may block the UI if it takes too long!
    try
    {
        currentContent = m_file.fetch(finalRequestPath);
        cmark_node *doc = Parser::parseContent(currentContent);
        m_draw.processDocument(doc);
        cmark_node_free(doc);
    }
    catch (const std::runtime_error &error)
    {
        std::string errorMessage = std::string(error.what());
        std::cerr << "Error: IPFS request failed, with message: " << errorMessage << std::endl;
        if (errorMessage.starts_with("HTTP request failed with status code"))
        {
            // Remove text until ':\n'
            errorMessage.erase(0, errorMessage.find(':') + 2);
            auto content = nlohmann::json::parse(errorMessage);
            std::string message = content.value("Message", "");
            m_draw.showMessage("Page not found!", message);
        }
        else if (errorMessage.starts_with("Couldn't connect to server: Failed to connect to localhost"))
        {
            m_draw.showMessage("Please wait...", "IPFS daemon is still spinnng-up, please try to refresh shortly...");
        }
        else
        {
            m_draw.showMessage("Something went wrong", "Error message: " + std::string(error.what()));
        }
    }
}

/**
 * Helper method for processRequest(),
 * Display markdown file from disk.
 */
void MainWindow::openFromDisk()
{
    try
    {
        currentContent = m_file.read(finalRequestPath);
        cmark_node *doc = Parser::parseContent(currentContent);
        m_draw.processDocument(doc);
        cmark_node_free(doc);
    }
    catch (const std::runtime_error &error)
    {
        std::cerr << "Error: File request failed, with message: " << error.what() << std::endl;
        m_draw.showMessage("Page not found!", "Error message: " + std::string(error.what()));
    }
}

/// Show source code dialog window with the current content
void MainWindow::show_source_code_dialog()
{
    m_sourceCodeDialog.setText(currentContent);
    m_sourceCodeDialog.run();
}