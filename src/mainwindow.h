#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/menubar.h>
#include <gtkmm/scrolledwindow.h>
#include "render-area.h"
#include "menu.h"
#include "file.h"
#include "about.h"

class MainWindow : public Gtk::Window
{
public:
    MainWindow();

protected:
    // Signal handlers:
    // Our new improved on_button_clicked(). (see below)
    void on_button_clicked(Glib::ustring data);
    void show_about();
    void hide_about(int response);

    // Child widgets
    Menu m_menu;
    Gtk::Box m_vbox;
    Gtk::ScrolledWindow m_scrolledWindow;
    RenderArea m_renderArea;
    About m_about;
private:
    File m_file;

    void demo();
};

#endif