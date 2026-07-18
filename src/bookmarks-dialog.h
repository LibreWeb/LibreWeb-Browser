#ifndef BOOKMARKS_DIALOG_H
#define BOOKMARKS_DIALOG_H

#include "bookmarks-storage.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treeview.h>

/**
 * \class BookmarksDialog
 * \brief Pop-up window to manage (rename, delete, import & export) your bookmarks
 */
class BookmarksDialog : public Gtk::Dialog
{
public:
  sigc::signal<void> bookmarks_updated;                 /*!< Emitted after the bookmarks are changed in any way */
  sigc::signal<void, const std::string&> open_bookmark; /*!< Emitted when a bookmark row is double-clicked */

  explicit BookmarksDialog(BookmarksStorage& storage);
  virtual ~BookmarksDialog();
  void show_dialog();

protected:
  /**
   * \class ModelColumns
   * \brief TreeView model columns of the bookmarks list
   */
  class ModelColumns : public Gtk::TreeModel::ColumnRecord
  {
  public:
    ModelColumns()
    {
      add(name);
      add(address);
    }
    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Glib::ustring> address;
  };

  // Child widgets
  ModelColumns columns;
  Glib::RefPtr<Gtk::ListStore> list_model;
  Gtk::TreeView tree_view;
  Gtk::ScrolledWindow scrolled_window;
  Gtk::Box hbox_buttons;
  Gtk::Button open_button;
  Gtk::Button delete_button;
  Gtk::Button import_button;
  Gtk::Button export_button;
  Gtk::Button close_button;

private:
  BookmarksStorage& storage_;

  void populate_list();
  bool get_selected_index(std::size_t& index);
  void on_name_edited(const Glib::ustring& path, const Glib::ustring& new_text);
  void on_address_edited(const Glib::ustring& path, const Glib::ustring& new_text);
  void on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
  void on_open_clicked();
  void on_delete_clicked();
  void on_import_clicked();
  void on_export_clicked();
};
#endif
