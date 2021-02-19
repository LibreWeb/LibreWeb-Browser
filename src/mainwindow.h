#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/menubar.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
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
    void new_doc();
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
    void get_heading();

    Glib::RefPtr<Gtk::AccelGroup> accelGroup; /*!< Accelerator group, used for keyboard shortcut bindings */

    // Child widgets
    Menu m_menu;
    Draw m_draw;
    SourceCodeDialog m_sourceCodeDialog;
    About m_about;
    Gtk::SearchBar m_search;
    Gtk::SearchEntry m_searchEntry;
    Gtk::Box m_vbox;
    Gtk::Box m_hboxToolbar;
    Gtk::Box m_hboxEditor;
    Gtk::Box m_hboxBottom;
    Gtk::Entry m_addressBar;
    Gtk::Button m_backButton;
    Gtk::Button m_forwardButton;
    Gtk::Button m_refreshButton;
    Gtk::Button m_homeButton;
    Gtk::ComboBoxText m_headingsComboBox;
    Gtk::Button m_boldButton;
    Gtk::Button m_italicButton;
    Gtk::Button m_strikethroughButton;
    Gtk::Button m_superButton;
    Gtk::Button m_subButton;
    Gtk::Button m_inlineCodeButton;
    Gtk::Button m_quoteButton;
    Gtk::Button m_codeBlockButton;
    Gtk::Button m_linkButton;
    Gtk::Button m_imageButton;
    Gtk::Button m_bulletListButton;
    Gtk::Button m_numberedListButton;
    Gtk::Button m_highlightButton;
    Gtk::Image backIcon;
    Gtk::Image forwardIcon;
    Gtk::Image refreshIcon;
    Gtk::Image homeIcon;
    Gtk::Image boldIcon;
    Gtk::Image italicIcon;
    Gtk::Image strikethroughIcon;
    Gtk::Image superIcon;
    Gtk::Image subIcon;
    Gtk::Image inlineCodeIcon;
    Gtk::Image quoteIcon;
    Gtk::Image codeBlockIcon;
    Gtk::Image linkIcon;
    Gtk::Image imageIcon;
    Gtk::Image bulletIcon;
    Gtk::Image numberedIcon;
    Gtk::Image hightlightIcon;
    Gtk::ScrolledWindow m_scrolledWindow;
    Gtk::Button m_exitBottomButton;

private:
    std::string m_appName;
    File m_file;
    std::thread *m_requestThread;
    std::string requestPath;
    std::string finalRequestPath;
    std::string currentContent;
    std::size_t currentHistoryIndex;
    std::vector<std::string> history;

    void enableEditing();
    void disableEditing();
    void postDoRequest(const std::string &path, bool setAddressBar, bool isHistoryRequest);
    void processRequest(const std::string &path);
    void fetchFromIPFS();
    void openFromDisk();
};

#endif
