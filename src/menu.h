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
class Menu: public Gtk::MenuBar
{
public:
  sigc::signal<void> reload;
  sigc::signal<void> source_code;
  sigc::signal<void> quit;
  sigc::signal<void> show_about;

  Menu();
  virtual ~Menu();
  Gtk::Menu* GetMachineMenu();

protected:
  // Child widgets
  Gtk::MenuItem m_file;
  Gtk::MenuItem m_view;
  Gtk::MenuItem m_help;
  Gtk::Menu m_file_submenu; /*!< File sub menu */
  Gtk::Menu m_view_submenu; /*!< Help sub menu */
  Gtk::Menu m_help_submenu; /*!< Help sub menu */
  Gtk::SeparatorMenuItem m_separator1;

private:
  Gtk::MenuItem* createMenuItem(const Glib::ustring& label_text);
};
#endif