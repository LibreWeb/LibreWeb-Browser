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
      m_hboxToolbar(Gtk::ORIENTATION_HORIZONTAL, 0),
      m_hboxBottom(Gtk::ORIENTATION_HORIZONTAL, 0),
      m_appName("DWeb Browser"),
      m_requestThread(nullptr),
      requestPath(""),
      finalRequestPath(""),
      currentContent(""),
      currentHistoryIndex(0)
{
    set_title(m_appName);
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
    m_addressBar.signal_activate().connect(sigc::mem_fun(this, &MainWindow::address_bar_activate));                  /*!< User pressed enter the address bar */
    m_backButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::back));                                   /*!< Button for previous page */
    m_forwardButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::forward));                             /*!< Button for next page */
    m_refreshButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::refresh));                             /*!< Button for reloading the page */
    m_homeButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::go_home));                                /*!< Button for home page */
    m_searchEntry.signal_activate().connect(sigc::mem_fun(this, &MainWindow::do_search));                            /*!< Execute the text search */

    m_vbox.pack_start(m_menu, false, false, 0);

    // Editor buttons
    m_headingsComboBox.signal_changed().connect(sigc::mem_fun(this, &MainWindow::get_heading));
    m_boldButton.signal_clicked().connect(sigc::mem_fun(m_draw, &Draw::make_bold));
    m_italicButton.signal_clicked().connect(sigc::mem_fun(m_draw, &Draw::make_italic));
    m_strikethroughButton.signal_clicked().connect(sigc::mem_fun(m_draw, &Draw::make_strikethrough));
    m_superButton.signal_clicked().connect(sigc::mem_fun(m_draw, &Draw::make_super));
    m_subButton.signal_clicked().connect(sigc::mem_fun(m_draw, &Draw::make_sub));
    m_quoteButton.signal_clicked().connect(sigc::mem_fun(m_draw, &Draw::make_quote));
    m_codeButton.signal_clicked().connect(sigc::mem_fun(m_draw, &Draw::make_code));
    m_linkButton.signal_clicked().connect(sigc::mem_fun(m_draw, &Draw::insert_link));
    m_imageButton.signal_clicked().connect(sigc::mem_fun(m_draw, &Draw::insert_image));
    m_bulletListButton.signal_clicked().connect(sigc::mem_fun(m_draw, &Draw::insert_bullet_list));
    m_numberedListButton.signal_clicked().connect(sigc::mem_fun(m_draw, &Draw::insert_numbered_list));
    m_highlightButton.signal_clicked().connect(sigc::mem_fun(m_draw, &Draw::make_highlight));

    // Add icons to the editor buttons
    boldIcon.set_from_icon_name("format-text-bold-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_boldButton.add(boldIcon);
    italicIcon.set_from_icon_name("format-text-italic-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_italicButton.add(italicIcon);
    strikethroughIcon.set_from_icon_name("format-text-strikethrough-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_strikethroughButton.add(strikethroughIcon);
    superIcon.set_from_icon_name("format-text-bold-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_superButton.add(superIcon);
    subIcon.set_from_icon_name("format-text-bold-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_subButton.add(subIcon);
    quoteIcon.set_from_icon_name("format-text-bold-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_quoteButton.add(quoteIcon);
    codeIcon.set_from_icon_name("view-paged-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_codeButton.add(codeIcon);
    linkIcon.set_from_icon_name("insert-link-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_linkButton.add(linkIcon);
    imageIcon.set_from_icon_name("insert-image-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_imageButton.add(imageIcon);
    bulletIcon.set_from_icon_name("view-list-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_bulletListButton.add(bulletIcon);
    numberedIcon.set_from_icon_name("view-list-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_numberedListButton.add(numberedIcon);
    hightlightIcon.set_from_icon_name("format-text-bold-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_highlightButton.add(hightlightIcon);

    // Disable focus on editor buttons
    m_headingsComboBox.set_can_focus(false);
    m_headingsComboBox.set_focus_on_click(false);
    m_boldButton.set_can_focus(false);
    m_italicButton.set_can_focus(false);
    m_strikethroughButton.set_can_focus(false);
    m_superButton.set_can_focus(false);
    m_subButton.set_can_focus(false);
    m_quoteButton.set_can_focus(false);
    m_codeButton.set_can_focus(false);
    m_linkButton.set_can_focus(false);
    m_imageButton.set_can_focus(false);
    m_bulletListButton.set_can_focus(false);
    m_numberedListButton.set_can_focus(false);
    m_highlightButton.set_can_focus(false);

    // Populate the heading comboboxtext
    m_headingsComboBox.append("", "Select Heading");
    m_headingsComboBox.append("1", "Heading 1");
    m_headingsComboBox.append("2", "Heading 2");
    m_headingsComboBox.append("3", "Heading 3");
    m_headingsComboBox.append("4", "Heading 4");
    m_headingsComboBox.append("5", "Heading 5");
    m_headingsComboBox.append("6", "Heading 6");
    m_headingsComboBox.set_active(0);

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

    // Add icons to the toolbar buttons
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

    // Toolbar
    m_backButton.set_margin_left(6);
    m_hboxToolbar.pack_start(m_backButton, false, false, 0);
    m_hboxToolbar.pack_start(m_forwardButton, false, false, 0);
    m_hboxToolbar.pack_start(m_refreshButton, false, false, 0);
    m_hboxToolbar.pack_start(m_homeButton, false, false, 0);
    m_hboxToolbar.pack_start(m_addressBar, true, true, 8);
    m_vbox.pack_start(m_hboxToolbar, false, false, 6);

    // Editor bar
    m_headingsComboBox.set_margin_left(4);
    m_hboxEditor.pack_start(m_headingsComboBox, false, false, 2);
    m_hboxEditor.pack_start(m_boldButton, false, false, 2);
    m_hboxEditor.pack_start(m_italicButton, false, false, 2);
    m_hboxEditor.pack_start(m_strikethroughButton, false, false, 2);
    m_hboxEditor.pack_start(m_superButton, false, false, 2);
    m_hboxEditor.pack_start(m_subButton, false, false, 2);
    m_hboxEditor.pack_start(m_quoteButton, false, false, 2);
    m_hboxEditor.pack_start(m_codeButton, false, false, 2);
    m_hboxEditor.pack_start(m_linkButton, false, false, 2);
    m_hboxEditor.pack_start(m_imageButton, false, false, 2);
    m_hboxEditor.pack_start(m_bulletListButton, false, false, 2);
    m_hboxEditor.pack_start(m_numberedListButton, false, false, 2);
    m_hboxEditor.pack_start(m_highlightButton, false, false, 2);
    m_vbox.pack_start(m_hboxEditor, false, false, 6);

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
    // Hide by default the bottom & editor box
    m_hboxBottom.hide();
    m_hboxEditor.hide();

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
        this->postDoRequest(path, setAddressBar, isHistoryRequest);
    }
}

/**
 * Post processing request actions
 */
void MainWindow::postDoRequest(const std::string &path, bool setAddressBar, bool isHistoryRequest)
{
    if (setAddressBar)
        m_addressBar.set_text(path);

    this->disableEditing();

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
    // Inform the Draw class about the new document
    m_draw.newDocument();
    // Enable editing mode
    this->enableEditing();
}

void MainWindow::go_home()
{
    this->requestPath = "";
    this->finalRequestPath = "";
    this->currentContent = "";
    this->m_addressBar.set_text("");
    this->disableEditing();
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

void MainWindow::enableEditing()
{
    this->m_hboxEditor.show();
    set_title("Untitled * - " + m_appName);
}

void MainWindow::disableEditing()
{
    if (m_hboxEditor.is_visible())
    {
        this->m_hboxEditor.hide();
        set_title(m_appName);
    }
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

/**
 * Show source code dialog window with the current content
 */
void MainWindow::show_source_code_dialog()
{
    m_sourceCodeDialog.setText(currentContent);
    m_sourceCodeDialog.run();
}

/**
 * Retrieve selected heading from combobox.
 * Send to draw class
 */
void MainWindow::get_heading()
{
    std::string active = m_headingsComboBox.get_active_id();
    m_headingsComboBox.set_active(0); // Reset
    if (active != "") {
        std::string::size_type sz;
        try
        {
            int headingLevel = std::stoi(active, &sz, 10);
            m_draw.make_heading(headingLevel);
        }
        catch (const std::invalid_argument&)
        {
            // ignore
            std::cerr << "Error: heading combobox id is invalid (not a number)." << std::endl;
        }
        catch (const std::out_of_range&)
        {
            // ignore
        }
    }
}