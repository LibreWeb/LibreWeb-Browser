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
  void set_back_menu_sensitive(bool sensitive);
  void set_forward_menu_sensitive(bool sensitive);
  void set_publish_menu_sensitive(bool sensitive);
  void set_edit_menu_sensitive(bool sensitive);

protected:
  // Child widgets
  Gtk::MenuItem file_menu_item;
  Gtk::MenuItem edit_menu_item;
  Gtk::MenuItem view_menu_item;
  Gtk::MenuItem help_menu_item;
  Gtk::Menu file_menu; /*!< File drop-down menu */
  Gtk::Menu edit_menu; /*!< Edit drop-down menu */
  Gtk::Menu view_menu; /*!< View drop-down menu */
  Gtk::Menu help_menu; /*!< Help drop-down menu */
  Gtk::SeparatorMenuItem separator1;
  Gtk::SeparatorMenuItem separator2;
  Gtk::SeparatorMenuItem separator3;
  Gtk::SeparatorMenuItem separator4;
  Gtk::SeparatorMenuItem separator5;
  Gtk::SeparatorMenuItem separator6;
  Gtk::SeparatorMenuItem separator7;
  Gtk::SeparatorMenuItem separator8;

private:
  Gtk::MenuItem* back_menu_item_;
  Gtk::MenuItem* forward_menu_item_;
  Gtk::MenuItem* publish_menu_item_;
  Gtk::MenuItem* edit_menu_item_;

  Gtk::MenuItem* create_menu_item(const Glib::ustring& label_text);
};
#endif