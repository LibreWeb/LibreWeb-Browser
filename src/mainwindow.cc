#include "mainwindow.h"

#include <gtkmm/menuitem.h>
#include <gtkmm/image.h>
#include <cmark-gfm.h>
#include <pthread.h>
#include <iostream>

#include "md-parser.h"

MainWindow::MainWindow()
    : m_vbox(Gtk::ORIENTATION_VERTICAL, 0),
      m_hbox_bar(Gtk::ORIENTATION_HORIZONTAL, 0),
      m_requestThread(nullptr),
      requestPath(""),
      finalRequestPath(""),
      currentContent("")
{
    set_title("DBrowser");
    set_default_size(1000, 800);
    set_position(Gtk::WIN_POS_CENTER);

    // Connect signals
    m_menu.quit.connect(sigc::mem_fun(this, &MainWindow::hide));                                                     /*!< hide main window and therefor closes the app */
    m_menu.reload.connect(sigc::mem_fun(this, &MainWindow::refresh));                                                /*!< Menu item for reloading the page */
    m_menu.source_code.connect(sigc::mem_fun(this, &MainWindow::show_source_code_dialog));                           /*!< Source code dialog */
    m_sourceCodeDialog.signal_response().connect(sigc::mem_fun(m_sourceCodeDialog, &SourceCodeDialog::hide_dialog)); /*!< Close source code dialog */
    m_menu.about.connect(sigc::mem_fun(m_about, &About::show_about));                                                /*!< Display about dialog */
    m_about.signal_response().connect(sigc::mem_fun(m_about, &About::hide_about));                                   /*!< Close about dialog */
    m_refreshButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::refresh));                             /*!< Button for reloading the page */
    m_homeButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::go_home));                                /*!< Button for home page */
    m_inputField.signal_activate().connect(sigc::mem_fun(this, &MainWindow::input_activate));                        /*!< User pressed enter in the input */

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

    m_hbox_bar.pack_start(m_backButton, false, false, 0);
    m_hbox_bar.pack_start(m_forwardButton, false, false, 0);
    m_hbox_bar.pack_start(m_refreshButton, false, false, 0);
    m_hbox_bar.pack_start(m_homeButton, false, false, 0);
    m_hbox_bar.pack_start(m_inputField, true, true, 8);
    m_vbox.pack_start(m_hbox_bar, false, false, 6);

    // Browser text drawing area
    m_scrolledWindow.add(m_draw);
    m_scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    m_vbox.pack_end(m_scrolledWindow, true, true, 0);
    add(m_vbox);
    show_all_children();

    // Grap focus to input field by default
    m_inputField.grab_focus();

    // Show start page by default
    go_home();
}

void MainWindow::go_home()
{
    this->requestPath = "";
    this->finalRequestPath = "";
    this->currentContent = "";
    this->m_inputField.set_text("");
    m_draw.showStartPage();
}

/**
 * Trigger when user input text in address bar
 */
void MainWindow::input_activate()
{
    // QmQzhn6hEfbYdCfwzYFsSt3eWpubVKA1dNqsgUwci5vHwq

    // Stop running thread (if applicable)
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
        m_requestThread = new std::thread(&MainWindow::doRequest, this, m_inputField.get_text());
}

/**
 * Refresh page
 */
void MainWindow::refresh()
{
    if (m_requestThread == nullptr)
        m_requestThread = new std::thread(&MainWindow::doRequest, this, "");
}

/**
 * Get the file from disk or IPFS network, from the provided path,
 * parse the content, and display the document
 */
void MainWindow::doRequest(const std::string &path)
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
 * Helper method for doRequest(),
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
        std::cerr << "Error: IPFS request failed, with message: " << error.what() << std::endl;
        // Show not found (or any other issue)
        m_draw.showMessage("Page not found!", "Detailed error message: " + std::string(error.what()));
    }
}

/**
 * Helper method for doRequest(),
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
        m_draw.showMessage("Page not found!", "Detailed error message: " + std::string(error.what()));
    }
}

/// Show source code dialog window with the current content
void MainWindow::show_source_code_dialog()
{
    m_sourceCodeDialog.setText(currentContent);
    m_sourceCodeDialog.run();
}