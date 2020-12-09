#include "mainwindow.h"

#include <gtkmm/menuitem.h>
#include <gtkmm/image.h>

#ifdef LEGACY_CXX
#include <experimental/filesystem>
namespace n_fs = ::std::experimental::filesystem;
#else
#include <filesystem>
namespace n_fs = ::std::filesystem;
#endif

MainWindow::MainWindow() 
: m_vbox(Gtk::ORIENTATION_VERTICAL, 0),
  m_hbox_bar(Gtk::ORIENTATION_HORIZONTAL, 0)
{
    set_title("Browser");
    set_default_size(1000, 800);
    set_position(Gtk::WIN_POS_CENTER);

    // Connect signals
    m_menu.quit.connect(sigc::mem_fun(this, &MainWindow::hide)); /*!< hide main window and therefor closes the app */
    m_menu.reload.connect(sigc::mem_fun(this, &MainWindow::demo)); /*!< Menu item for reloading the page */
    m_menu.about.connect(sigc::mem_fun(m_about, &About::show_about)); /*!< Display about dialog */
    m_about.signal_response().connect(sigc::mem_fun(m_about, &About::hide_about)); /*!< Close about dialog */
    m_refreshButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::demo)); /*!< Button for reloading the page */

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

    m_hbox_bar.pack_start(m_backButton, false, false , 0);
    m_hbox_bar.pack_start(m_forwardButton, false, false , 0);
    m_hbox_bar.pack_start(m_refreshButton, false, false , 0);
    m_hbox_bar.pack_start(m_homeButton, false, false , 0);
    m_hbox_bar.pack_start(m_inputField, true, true , 8);
    m_vbox.pack_start(m_hbox_bar, false, false, 6);

    // Main browser rendering area
    m_scrolledWindow.add(m_renderArea);
    m_scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    m_vbox.pack_end(m_scrolledWindow, true, true, 0);
    add(m_vbox);    
    show_all_children();

    // Grap focus to input field by default
    m_inputField.grab_focus();

    demo();
}

void MainWindow::demo()
{
/*
    // From disk
    std::string exePath = n_fs::current_path().string();
    std::string filePath = exePath.append("/../../test.md");
    cmark_node *readDoc = m_file.read(filePath);
    if (readDoc != NULL) {
        m_renderArea.processDocument(readDoc);
        m_file.free(readDoc);
    }
*/
    // From IPFS
    try {
        cmark_node *fetchDoc = m_file.fetch("QmQzhn6hEfbYdCfwzYFsSt3eWpubVKA1dNqsgUwci5vHwq");
        m_renderArea.processDocument(fetchDoc);
        m_file.free(fetchDoc);
    } catch (const std::runtime_error &error) {
        std::cerr << "IPFS Deamon is most likely down: " << error.what() << std::endl;
        // Not found (or any other issue)
        m_renderArea.showMessage("Page not found!", "Detailed error message: " + std::string(error.what()));
    }
}


