#include "menu.h"

Menu::Menu(const Glib::RefPtr<Gtk::AccelGroup>& accel_group)
    : file_menu_item("_File", true),
      edit_menu_item("_Edit", true),
      view_menu_item("_View", true),
      help_menu_item("_Help", true)
{
  // File dropdown menu
  auto new_document_menu_item = create_menu_item("_New Document");
  new_document_menu_item->add_accelerator("activate", accel_group, GDK_KEY_N, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  new_document_menu_item->signal_activate().connect(new_doc);
  auto open_menu_item = create_menu_item("_Open...");
  open_menu_item->add_accelerator("activate", accel_group, GDK_KEY_O, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  open_menu_item->signal_activate().connect(open);
  auto open_edit_menu_item = create_menu_item("Open & _Edit...");
  open_edit_menu_item->add_accelerator("activate", accel_group, GDK_KEY_E, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  open_edit_menu_item->signal_activate().connect(open_edit);
  edit_menu_item_ = create_menu_item("Edit");
  edit_menu_item_->add_accelerator("activate", accel_group, GDK_KEY_D, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  edit_menu_item_->signal_activate().connect(edit);
  auto save_menuitem = create_menu_item("_Save");
  save_menuitem->add_accelerator("activate", accel_group, GDK_KEY_S, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  save_menuitem->signal_activate().connect(save);
  auto save_as_menu_item = create_menu_item("Save _As...");
  save_as_menu_item->add_accelerator("activate", accel_group, GDK_KEY_S, Gdk::ModifierType::CONTROL_MASK | Gdk::ModifierType::SHIFT_MASK,
                                     Gtk::AccelFlags::ACCEL_VISIBLE);
  save_as_menu_item->signal_activate().connect(save_as);
  publish_menu_item_ = create_menu_item("_Publish...");
  publish_menu_item_->set_sensitive(false); // disable
  publish_menu_item_->add_accelerator("activate", accel_group, GDK_KEY_P, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  publish_menu_item_->signal_activate().connect(publish);
  auto quit_menu_item = create_menu_item("_Quit");
  quit_menu_item->add_accelerator("activate", accel_group, GDK_KEY_Q, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  quit_menu_item->signal_activate().connect(quit);

  // Edit dropdown menu
  auto undo_menu_item = create_menu_item("_Undo");
  undo_menu_item->add_accelerator("activate", accel_group, GDK_KEY_Z, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  undo_menu_item->signal_activate().connect(undo);
  auto redo_menu_item = create_menu_item("_Redo");
  redo_menu_item->add_accelerator("activate", accel_group, GDK_KEY_Y, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  redo_menu_item->signal_activate().connect(redo);
  auto cut_menu_item = create_menu_item("Cu_t");
  cut_menu_item->add_accelerator("activate", accel_group, GDK_KEY_X, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  cut_menu_item->signal_activate().connect(cut);
  auto copy_menu_item = create_menu_item("_Copy");
  copy_menu_item->add_accelerator("activate", accel_group, GDK_KEY_C, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  copy_menu_item->signal_activate().connect(copy);
  auto paste_menu_item = create_menu_item("_Paste");
  paste_menu_item->add_accelerator("activate", accel_group, GDK_KEY_V, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  paste_menu_item->signal_activate().connect(paste);
  auto delete_menu_item = create_menu_item("_Delete");
  delete_menu_item->add_accelerator("activate", accel_group, GDK_KEY_Delete, (Gdk::ModifierType)0, Gtk::AccelFlags::ACCEL_VISIBLE);
  delete_menu_item->signal_activate().connect(del);
  auto select_all_menu_item = create_menu_item("Select _All");
  select_all_menu_item->add_accelerator("activate", accel_group, GDK_KEY_A, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  select_all_menu_item->signal_activate().connect(select_all);
  auto find_menu_item = create_menu_item("_Find");
  find_menu_item->add_accelerator("activate", accel_group, GDK_KEY_F, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  find_menu_item->signal_activate().connect(find);
  auto replace_menu_item = create_menu_item("_Replace");
  replace_menu_item->add_accelerator("activate", accel_group, GDK_KEY_H, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  replace_menu_item->signal_activate().connect(replace);

  // View dropdown menu
  back_menu_item_ = create_menu_item("_Previous Page");
  back_menu_item_->set_sensitive(false);
  back_menu_item_->add_accelerator("activate", accel_group, GDK_KEY_Left, Gdk::ModifierType::MOD1_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  back_menu_item_->signal_activate().connect(back);
  forward_menu_item_ = create_menu_item("_Next page");
  forward_menu_item_->set_sensitive(false);
  forward_menu_item_->add_accelerator("activate", accel_group, GDK_KEY_Right, Gdk::ModifierType::MOD1_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  forward_menu_item_->signal_activate().connect(forward);
  auto reload_menu_item = create_menu_item("_Reload Page");
  reload_menu_item->add_accelerator("activate", accel_group, GDK_KEY_R, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  reload_menu_item->signal_activate().connect(reload);
  auto home_page_menu_item = create_menu_item("_Homepage");
  home_page_menu_item->add_accelerator("activate", accel_group, GDK_KEY_Home, Gdk::ModifierType::MOD1_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  home_page_menu_item->signal_activate().connect(home);
  auto toc_menu_item = create_menu_item("_Table of Contents");
  toc_menu_item->add_accelerator("activate", accel_group, GDK_KEY_T, Gdk::ModifierType::CONTROL_MASK | Gdk::ModifierType::SHIFT_MASK,
                                 Gtk::AccelFlags::ACCEL_VISIBLE);
  toc_menu_item->signal_activate().connect(toc);
  auto source_code_menu_item = create_menu_item("View _Source");
  source_code_menu_item->signal_activate().connect(source_code);

  // Help dropdown menu
  auto about_menu_item = create_menu_item("_About");
  about_menu_item->signal_activate().connect(about);

  // Add items to sub-menus
  file_menu.append(*new_document_menu_item);
  file_menu.append(*open_menu_item);
  file_menu.append(*open_edit_menu_item);
  file_menu.append(*edit_menu_item_);
  file_menu.append(separator1);
  file_menu.append(*save_menuitem);
  file_menu.append(*save_as_menu_item);
  file_menu.append(separator2);
  file_menu.append(*publish_menu_item_);
  file_menu.append(separator3);
  file_menu.append(*quit_menu_item);
  edit_menu.append(*undo_menu_item);
  edit_menu.append(*redo_menu_item);
  edit_menu.append(separator4);
  edit_menu.append(*cut_menu_item);
  edit_menu.append(*copy_menu_item);
  edit_menu.append(*paste_menu_item);
  edit_menu.append(*delete_menu_item);
  edit_menu.append(separator5);
  edit_menu.append(*select_all_menu_item);
  edit_menu.append(separator6);
  edit_menu.append(*find_menu_item);
  edit_menu.append(*replace_menu_item);
  view_menu.append(*back_menu_item_);
  view_menu.append(*forward_menu_item_);
  view_menu.append(*reload_menu_item);
  view_menu.append(*home_page_menu_item);
  view_menu.append(separator7);
  view_menu.append(*toc_menu_item);
  view_menu.append(separator8);
  view_menu.append(*source_code_menu_item);
  help_menu.append(*about_menu_item);

  // Add sub-menus to menus
  file_menu_item.set_submenu(file_menu);
  edit_menu_item.set_submenu(edit_menu);
  view_menu_item.set_submenu(view_menu);
  help_menu_item.set_submenu(help_menu);
  // Add menus to menu bar
  append(file_menu_item);
  append(edit_menu_item);
  append(view_menu_item);
  append(help_menu_item);
}

Menu::~Menu()
{
}

void Menu::set_back_menu_sensitive(bool sensitive)
{
  back_menu_item_->set_sensitive(sensitive);
}

void Menu::set_forward_menu_sensitive(bool sensitive)
{
  forward_menu_item_->set_sensitive(sensitive);
}

void Menu::set_publish_menu_sensitive(bool sensitive)
{
  publish_menu_item_->set_sensitive(sensitive);
}

void Menu::set_edit_menu_sensitive(bool sensitive)
{
  edit_menu_item_->set_sensitive(sensitive);
}

/**
 * \brief Helper method for creating a menu with an image
 * \return GTKWidget menu item pointer
 */
Gtk::MenuItem* Menu::create_menu_item(const Glib::ustring& label_text)
{
  Gtk::MenuItem* item = Gtk::manage(new Gtk::MenuItem(label_text, true));
  return item;
}
