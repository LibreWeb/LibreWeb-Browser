#include "menu.h"

Menu::Menu()
: m_file("_File", true),
  m_view("_View", true),
  m_help("_Help", true)
{
    // File submenu
    auto exit_menuitem = createMenuItem("Exit");
    exit_menuitem->signal_activate().connect(quit);

    // View submenu
    auto source_code_menuitem = createMenuItem("View Source");
    source_code_menuitem->signal_activate().connect(source_code);

    // Help submenu
    auto about_menuitem = createMenuItem("About");
    about_menuitem->signal_activate().connect(show_about);

    // Add items to sub-menus
    m_file_submenu.append(*exit_menuitem);
    m_view_submenu.append(*source_code_menuitem);
    m_help_submenu.append(*about_menuitem);

    // Add sub-menus to menus
    m_file.set_submenu(m_file_submenu);
    m_view.set_submenu(m_view_submenu);
    m_help.set_submenu(m_help_submenu);
    // Add menus to menu bar
    append(m_file);
    append(m_view);
    append(m_help);
}

Menu::~Menu() {
}

/**
 * \brief Helper method for creating a menu with an image
 * \return GTKWidget menu item pointer
 */
Gtk::MenuItem* Menu::createMenuItem(const Glib::ustring& label_text) {
    Gtk::MenuItem* item = Gtk::manage(new Gtk::MenuItem(label_text));
    return item;
}
