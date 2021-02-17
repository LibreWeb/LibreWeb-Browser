#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/menubar.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/searchbar.h>
#include <gtkmm/searchentry.h>
#include <thread>
#include "menu.h"
#include "file.h"
#include "about.h"
#include "source-code-dialog.h"
#include "draw.h"

/**
 * \class MainWindow
 * \brief Main Application Window
 */
class MainWindow : public Gtk::Window
{
public:
    MainWindow();
    void doRequest(const std::string &path= std::string(), bool setAddressBar = false, bool isHistoryRequest = false);

protected:
    // Signal handlers
    void go_home();
    void address_bar_activate();
    void do_search();
    void show_search();
    void back();
    void forward();
    void refresh();
    void on_button_clicked(Glib::ustring data);
    void show_about();
    void hide_about(int response);
    void show_source_code_dialog();

    Glib::RefPtr<Gtk::AccelGroup> accelGroup; /*!< Accelerator group, used for keyboard shortcut bindings */

    // Child widgets
    Menu m_menu;
    Draw m_draw;
    SourceCodeDialog m_sourceCodeDialog;
    About m_about;
    Gtk::SearchBar m_search;
    Gtk::SearchEntry m_searchEntry;
    Gtk::Box m_vbox;
    Gtk::Box m_hboxBar;
    Gtk::Box m_hboxBottom;
    Gtk::Button m_backButton;
    Gtk::Button m_forwardButton;
    Gtk::Button m_refreshButton;
    Gtk::Button m_homeButton;
    Gtk::Entry m_addressBar;
    Gtk::Image backIcon;
    Gtk::Image forwardIcon;
    Gtk::Image refreshIcon;
    Gtk::Image homeIcon;
    Gtk::ScrolledWindow m_scrolledWindow;

private:
    File m_file;
    std::thread *m_requestThread;
    std::string requestPath;
    std::string finalRequestPath;
    std::string currentContent;
    std::size_t currentHistoryIndex;
    std::vector<std::string> history;

    void postDoRequest(const std::string &path, bool setAddressBar, bool isHistoryRequest);
    void processRequest(const std::string &path);
    void fetchFromIPFS();
    void openFromDisk();
};

#endif