#include "bookmarks-dialog.h"

#include <gtkmm/cellrenderertext.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/filefilter.h>
#include <gtkmm/messagedialog.h>

BookmarksDialog::BookmarksDialog(BookmarksStorage& storage) : hbox_buttons(Gtk::ORIENTATION_HORIZONTAL, 6), storage_(storage)
{
  set_title("Bookmarks");
  set_default_size(650, 400);

  list_model = Gtk::ListStore::create(columns);
  tree_view.set_model(list_model);
  tree_view.append_column_editable("Name", columns.name);
  tree_view.append_column_editable("Address", columns.address);
  tree_view.get_column(0)->set_resizable(true);
  tree_view.get_column(0)->set_min_width(220);
  tree_view.get_column(1)->set_resizable(true);
  if (auto* name_renderer = dynamic_cast<Gtk::CellRendererText*>(tree_view.get_column_cell_renderer(0)))
    name_renderer->signal_edited().connect(sigc::mem_fun(this, &BookmarksDialog::on_name_edited));
  if (auto* address_renderer = dynamic_cast<Gtk::CellRendererText*>(tree_view.get_column_cell_renderer(1)))
    address_renderer->signal_edited().connect(sigc::mem_fun(this, &BookmarksDialog::on_address_edited));
  tree_view.signal_row_activated().connect(sigc::mem_fun(this, &BookmarksDialog::on_row_activated));

  scrolled_window.add(tree_view);
  scrolled_window.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

  open_button.add_label("_Open", true);
  open_button.signal_clicked().connect(sigc::mem_fun(this, &BookmarksDialog::on_open_clicked));
  delete_button.add_label("_Delete", true);
  delete_button.signal_clicked().connect(sigc::mem_fun(this, &BookmarksDialog::on_delete_clicked));
  import_button.add_label("_Import...", true);
  import_button.signal_clicked().connect(sigc::mem_fun(this, &BookmarksDialog::on_import_clicked));
  export_button.add_label("_Export...", true);
  export_button.signal_clicked().connect(sigc::mem_fun(this, &BookmarksDialog::on_export_clicked));
  close_button.add_label("_Close", true);
  close_button.signal_clicked().connect(sigc::mem_fun(this, &BookmarksDialog::hide));

  hbox_buttons.set_margin_top(10);
  hbox_buttons.pack_start(open_button, false, false, 0);
  hbox_buttons.pack_start(delete_button, false, false, 0);
  hbox_buttons.pack_start(import_button, false, false, 0);
  hbox_buttons.pack_start(export_button, false, false, 0);
  hbox_buttons.pack_end(close_button, false, false, 0);

  auto vbox = get_content_area();
  vbox->set_margin_left(10);
  vbox->set_margin_right(10);
  vbox->set_margin_top(10);
  vbox->set_margin_bottom(10);
  vbox->pack_start(scrolled_window, true, true, 0);
  vbox->pack_start(hbox_buttons, false, false, 0);

  show_all_children();
}

BookmarksDialog::~BookmarksDialog()
{
}

/**
 * \brief Show the bookmarks dialog with an up-to-date bookmarks list
 */
void BookmarksDialog::show_dialog()
{
  populate_list();
  show();
  present();
}

/**
 * \brief Reload the list from the bookmarks storage
 */
void BookmarksDialog::populate_list()
{
  list_model->clear();
  for (const Bookmark& bookmark : storage_.get_bookmarks())
  {
    Gtk::TreeModel::Row row = *(list_model->append());
    row[columns.name] = bookmark.name;
    row[columns.address] = bookmark.address;
  }
}

/**
 * \brief Retrieve the index of the currently selected bookmark row
 * \param[out] index Index of the selected row within the bookmarks storage
 * \return True when a row is selected
 */
bool BookmarksDialog::get_selected_index(std::size_t& index)
{
  Gtk::TreeModel::iterator iter = tree_view.get_selection()->get_selected();
  if (!iter)
    return false;
  index = static_cast<std::size_t>(list_model->get_path(iter)[0]);
  return true;
}

/**
 * \brief Called when the bookmark name cell is edited
 */
void BookmarksDialog::on_name_edited(const Glib::ustring& path, const Glib::ustring& new_text)
{
  std::size_t index = static_cast<std::size_t>(Gtk::TreeModel::Path(path)[0]);
  const std::vector<Bookmark>& bookmarks = storage_.get_bookmarks();
  if (index < bookmarks.size())
  {
    storage_.update(index, new_text, bookmarks[index].address);
    bookmarks_updated.emit();
  }
}

/**
 * \brief Called when the bookmark address cell is edited
 */
void BookmarksDialog::on_address_edited(const Glib::ustring& path, const Glib::ustring& new_text)
{
  std::size_t index = static_cast<std::size_t>(Gtk::TreeModel::Path(path)[0]);
  const std::vector<Bookmark>& bookmarks = storage_.get_bookmarks();
  if (index < bookmarks.size())
  {
    storage_.update(index, bookmarks[index].name, new_text);
    bookmarks_updated.emit();
  }
}

/**
 * \brief Called when a bookmark row is double-clicked, opens the bookmark
 */
void BookmarksDialog::on_row_activated(const Gtk::TreeModel::Path& path, __attribute__((unused)) Gtk::TreeViewColumn* column)
{
  std::size_t index = static_cast<std::size_t>(path[0]);
  const std::vector<Bookmark>& bookmarks = storage_.get_bookmarks();
  if (index < bookmarks.size())
  {
    open_bookmark.emit(bookmarks[index].address);
    hide();
  }
}

/**
 * \brief Open the selected bookmark
 */
void BookmarksDialog::on_open_clicked()
{
  std::size_t index;
  const std::vector<Bookmark>& bookmarks = storage_.get_bookmarks();
  if (get_selected_index(index) && index < bookmarks.size())
  {
    open_bookmark.emit(bookmarks[index].address);
    hide();
  }
}

/**
 * \brief Delete the selected bookmark
 */
void BookmarksDialog::on_delete_clicked()
{
  std::size_t index;
  if (get_selected_index(index))
  {
    storage_.remove(index);
    populate_list();
    bookmarks_updated.emit();
  }
}

/**
 * \brief Import bookmarks from a JSON file
 */
void BookmarksDialog::on_import_clicked()
{
  Gtk::FileChooserDialog dialog(*this, "Import bookmarks", Gtk::FILE_CHOOSER_ACTION_OPEN);
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Import", Gtk::RESPONSE_OK);
  auto filter = Gtk::FileFilter::create();
  filter->set_name("Bookmarks JSON (*.json)");
  filter->add_pattern("*.json");
  dialog.add_filter(filter);
  if (dialog.run() == Gtk::RESPONSE_OK)
  {
    try
    {
      std::size_t imported = storage_.import_from(dialog.get_filename());
      populate_list();
      bookmarks_updated.emit();
      Gtk::MessageDialog message(*this, "Imported " + std::to_string(imported) + " new bookmark(s).", false, Gtk::MESSAGE_INFO);
      message.set_title("Import bookmarks");
      message.run();
    }
    catch (const std::exception& error)
    {
      Gtk::MessageDialog message(*this, "Could not import bookmarks. Is the file a valid bookmarks JSON file?", false, Gtk::MESSAGE_ERROR);
      message.set_title("Import bookmarks");
      message.run();
    }
  }
}

/**
 * \brief Export bookmarks to a JSON file
 */
void BookmarksDialog::on_export_clicked()
{
  Gtk::FileChooserDialog dialog(*this, "Export bookmarks", Gtk::FILE_CHOOSER_ACTION_SAVE);
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Export", Gtk::RESPONSE_OK);
  dialog.set_do_overwrite_confirmation(true);
  dialog.set_current_name("bookmarks.json");
  auto filter = Gtk::FileFilter::create();
  filter->set_name("Bookmarks JSON (*.json)");
  filter->add_pattern("*.json");
  dialog.add_filter(filter);
  if (dialog.run() == Gtk::RESPONSE_OK)
  {
    try
    {
      storage_.export_to(dialog.get_filename());
    }
    catch (const std::exception& error)
    {
      Gtk::MessageDialog message(*this, "Could not export bookmarks to the selected location.", false, Gtk::MESSAGE_ERROR);
      message.set_title("Export bookmarks");
      message.run();
    }
  }
}
