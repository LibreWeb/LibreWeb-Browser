#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "menu.h"
#include "about.h"
#include "source-code-dialog.h"
#include "draw.h"
#include "ipfs.h"

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/menubar.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/button.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/popover.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/searchbar.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/paned.h>
#include <giomm/settings.h>
#include <thread>

/**
 * \class MainWindow
 * \brief Main Application Window
 */
class MainWindow : public Gtk::Window
{
public:
    MainWindow();
    void doRequest(const std::string &path = std::string(), bool setAddressBar = false, bool isHistoryRequest = false);

protected:
    // Signal handlers
    bool delete_window(GdkEventAny* any_event);
    bool update_connection_status();
    void cut();
    void copy();
    void paste();
    void del();
    void selectAll();
    void new_doc();
    void open();
    void on_open_dialog_response(int response_id, Gtk::FileChooserDialog* dialog);
    void save();
    void save_as();
    void on_save_as_dialog_response(int response_id, Gtk::FileChooserDialog* dialog);
    void publish();
    void go_home();
    void show_status();
    void address_bar_activate();
    void on_search();
    void on_replace();
    void show_search(bool replace);
    void back();
    void forward();
    void refresh();
    void on_button_clicked(Glib::ustring data);
    void show_about();
    void hide_about(int response);
    void editor_changed_text();
    void show_source_code_dialog();
    void get_heading();

    Glib::RefPtr<Gtk::AccelGroup> m_accelGroup; /*!< Accelerator group, used for keyboard shortcut bindings */
    Glib::RefPtr<Gio::Settings> m_settings; /*!< Settings to store our preferences, even during restarts */

    // Child widgets
    Menu m_menu;
    Draw m_draw_main;
    Draw m_draw_secondary;
    SourceCodeDialog m_sourceCodeDialog;
    About m_about;
    Gtk::HPaned m_paned;
    Gtk::SearchBar m_search;
    Gtk::SearchBar m_searchReplace;
    Gtk::SearchEntry m_searchEntry;
    Gtk::Entry m_searchReplaceEntry;
    Gtk::Box m_vbox;
    Gtk::Box m_hboxBrowserToolbar;
    Gtk::Box m_hboxStandardEditorToolbar;
    Gtk::Box m_hboxFormattingEditorToolbar;
    Gtk::Box m_hboxBottom;
    Gtk::Entry m_addressBar;
    Gtk::ToggleButton m_searchMatchCase;
    Gtk::Button m_backButton;
    Gtk::Button m_forwardButton;
    Gtk::Button m_refreshButton;
    Gtk::Button m_homeButton;
    Gtk::Button m_statusButton;
    Gtk::Button m_openButton;
    Gtk::Button m_saveButton;
    Gtk::Button m_publishButton;
    Gtk::Button m_cutButton;
    Gtk::Button m_copyButton;
    Gtk::Button m_pasteButton;
    Gtk::Button m_undoButton;
    Gtk::Button m_redoButton;
    Gtk::ComboBoxText m_headingsComboBox;
    Gtk::Button m_boldButton;
    Gtk::Button m_italicButton;
    Gtk::Button m_strikethroughButton;
    Gtk::Button m_superButton;
    Gtk::Button m_subButton;
    Gtk::Button m_linkButton;
    Gtk::Button m_imageButton;
    Gtk::Button m_quoteButton;
    Gtk::Button m_codeButton;
    Gtk::Button m_bulletListButton;
    Gtk::Button m_numberedListButton;
    Gtk::Button m_highlightButton;
    Gtk::Image m_backIcon;
    Gtk::Image m_forwardIcon;
    Gtk::Image m_refreshIcon;
    Gtk::Image m_homeIcon;
    Gtk::Image m_statusIcon;
    Glib::RefPtr<Gdk::Pixbuf> m_statusOfflineIcon;
    Glib::RefPtr<Gdk::Pixbuf> m_statusOnlineIcon;
    Gtk::Image m_openIcon;
    Gtk::Image m_saveIcon;
    Gtk::Image m_publishIcon;
    Gtk::Image m_cutIcon;
    Gtk::Image m_copyIcon;
    Gtk::Image m_pasteIcon;
    Gtk::Image m_undoIcon;
    Gtk::Image m_redoIcon;
    Gtk::Image m_boldIcon;
    Gtk::Image m_italicIcon;
    Gtk::Image m_strikethroughIcon;
    Gtk::Image m_superIcon;
    Gtk::Image m_subIcon;
    Gtk::Image m_linkIcon;
    Gtk::Image m_imageIcon;
    Gtk::Image m_quoteIcon;
    Gtk::Image m_codeIcon;
    Gtk::Image m_bulletListIcon;
    Gtk::Image m_numberedListIcon;
    Gtk::Image m_hightlightIcon;
    Gtk::Popover m_statusPopover;
    Gtk::Label m_statusLabel;
    Gtk::ScrolledWindow m_scrolledWindowMain;
    Gtk::ScrolledWindow m_scrolledWindowSecondary;
    Gtk::Button m_exitBottomButton;
    Gtk::SeparatorMenuItem m_separator1;
    Gtk::SeparatorMenuItem m_separator2;
    Gtk::SeparatorMenuItem m_separator3;
    Gtk::SeparatorMenuItem m_separator4;

private:
    std::string m_appName;
    std::string m_iconTheme;
    bool m_useCurrentGTKIconTheme;
    int m_iconSize;
    std::thread *m_requestThread;
    std::string requestPath;
    std::string finalRequestPath;
    std::string currentContent;
    std::string currentFileSavedPath;
    std::size_t currentHistoryIndex;
    std::vector<std::string> history;
    sigc::connection textChangedSignalHandler;
    sigc::connection statusTimerHandler;
    IPFS ipfs;

    bool isInstalled();
    void enableEdit();
    void disableEdit();
    bool isEditorEnabled();
    void postDoRequest(const std::string &path, bool setAddressBar, bool isHistoryRequest);
    void processRequest(const std::string &path);
    void fetchFromIPFS();
    void openFromDisk();
    std::string getIconImageFromTheme(const std::string &iconName, const std::string &typeofIcon);
};

#endif
