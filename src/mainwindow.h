#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/menubar.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include "render-area.h"
#include "menu.h"
#include "file.h"
#include "about.h"
#include "source-code-dialog.h"

class MainWindow : public Gtk::Window
{
public:
    MainWindow();

protected:
    // Signal handlers:
    // Our new improved on_button_clicked(). (see below)
    void go_home();
    void input_activate();
    void on_button_clicked(Glib::ustring data);
    void show_about();
    void hide_about(int response);
    void show_source_code_dialog();

    // Child widgets
    Menu m_menu;
    Gtk::Box m_vbox;
    Gtk::Box m_hbox_bar;
    Gtk::Button m_backButton;
    Gtk::Button m_forwardButton;
    Gtk::Button m_refreshButton;
    Gtk::Button m_homeButton;
    Gtk::Entry m_inputField;
    Gtk::Image backIcon;
    Gtk::Image forwardIcon;
    Gtk::Image refreshIcon;
    Gtk::Image homeIcon;
    Gtk::ScrolledWindow m_scrolledWindow;
    RenderArea m_renderArea;
    SourceCodeDialog m_sourceCodeDialog;
    About m_about;
private:
    File m_file;
    std::string requestPath;
    std::string finalRequestPath;
    std::string currentSourceCode;

    void doRequest(const std::string &path = "");
    void refresh();
    void fetchFromIPFS();
    void openFromDisk();
};

#endif