#ifndef MENU_H
#define MENU_H

#include <gtkmm/menu.h>
#include <gtkmm/menubar.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/separatormenuitem.h>
#include <signal.h>

/**
 * \class Menu
 * \brief The top main-menu
 */
class Menu : public Gtk::MenuBar
{
public:
  sigc::signal<void> new_doc;
  sigc::signal<void> open;
  sigc::signal<void> open_edit;
  sigc::signal<void> edit;
  sigc::signal<void> save;
  sigc::signal<void> save_as;
  sigc::signal<void> publish;
  sigc::signal<void> quit;
  sigc::signal<void> undo;
  sigc::signal<void> redo;
  sigc::signal<void> cut;
  sigc::signal<void> copy;
  sigc::signal<void> paste;
  sigc::signal<void> del;
  sigc::signal<void> select_all;
  sigc::signal<void> find;
  sigc::signal<void> replace;
  sigc::signal<void> back;
  sigc::signal<void> forward;
  sigc::signal<void> reload;
  sigc::signal<void> home;
  sigc::signal<void> toc;
  sigc::signal<void> source_code;
  sigc::signal<void> about;

  explicit Menu(const Glib::RefPtr<Gtk::AccelGroup>& accelgroup);
  virtual ~Menu();
  void setBackMenuSensitive(bool sensitive);
  void setForwardMenuSensitive(bool sensitive);
  void setPublishMenuSensitive(bool sensitive);
  void setEditMenuSensitive(bool sensitive);

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
  Gtk::SeparatorMenuItem m_separator4;
  Gtk::SeparatorMenuItem m_separator5;
  Gtk::SeparatorMenuItem m_separator6;
  Gtk::SeparatorMenuItem m_separator7;
  Gtk::SeparatorMenuItem m_separator8;

private:
  Gtk::MenuItem* createMenuItem(const Glib::ustring& label_text);
  Gtk::MenuItem* backMenuItem;
  Gtk::MenuItem* forwardMenuItem;
  Gtk::MenuItem* publishMenuItem;
  Gtk::MenuItem* editMenuItem;
};
#endif