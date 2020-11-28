#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <gtkmm/window.h>
#include <gtkmm/scrolledwindow.h>
#include "render-area.h"

class Parser;

class MainWindow : public Gtk::Window
{
public:
    MainWindow();
    virtual ~MainWindow();

protected:
    // Signal handlers:
    // Our new improved on_button_clicked(). (see below)
    void on_button_clicked(Glib::ustring data);

    // Child widgets
    Gtk::ScrolledWindow m_scrolledWindow;
    RenderArea m_renderArea;
private:
    Parser *parser;

     void setupParser();
};

#endif