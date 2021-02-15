#ifndef MENU_H
#define MENU_H

#include <signal.h>
#include <gtkmm/menubar.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/separatormenuitem.h>

/**
 * \class Menu
 * \brief The top main-menu
 */
class Menu : public Gtk::MenuBar
{
public:
    sigc::signal<void> quit;
    sigc::signal<void> cut;
    sigc::signal<void> copy;
    sigc::signal<void> paste;
    sigc::signal<void> del;
    sigc::signal<void> select_all;
    sigc::signal<void> find;
    sigc::signal<void> back;
    sigc::signal<void> forward;
    sigc::signal<void> reload;
    sigc::signal<void> source_code;
    sigc::signal<void> about;

    explicit Menu(const Glib::RefPtr<Gtk::AccelGroup> &accelgroup);
    virtual ~Menu();
    void setBackMenuSensitive(bool sensitive);
    void setForwardMenuSensitive(bool sensitive);

protected:
    // Child widgets
    Gtk::MenuItem m_file;
    Gtk::MenuItem m_edit;
    Gtk::MenuItem m_view;
    Gtk::MenuItem m_help;
    Gtk::Menu m_fileSubmenu; /*!< File sub menu */
    Gtk::Menu m_editSubmenu; /*!< Edit sub menu */
    Gtk::Menu m_viewSubmenu; /*!< View sub menu */
    Gtk::Menu m_helpSubmenu; /*!< Help sub menu */
    Gtk::SeparatorMenuItem m_separator1;
    Gtk::SeparatorMenuItem m_separator2;
    Gtk::SeparatorMenuItem m_separator3;

private:
    Gtk::MenuItem *createMenuItem(const Glib::ustring &label_text);
    Gtk::MenuItem *backMenuItem;
    Gtk::MenuItem *forwardMenuItem;
};
#endif