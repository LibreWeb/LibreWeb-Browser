#include "menu.h"

Menu::Menu(const Glib::RefPtr<Gtk::AccelGroup>& accelgroup)
    : m_file("_File", true),
      m_edit("_Edit", true),
      m_view("_View", true),
      m_help("_Help", true)
{
  // File sub-menu
  auto newDocumentMenuItem = createMenuItem("_New Document");
  newDocumentMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_N, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  newDocumentMenuItem->signal_activate().connect(new_doc);
  auto openMenuItem = createMenuItem("_Open...");
  openMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_O, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  openMenuItem->signal_activate().connect(open);
  auto openEditMenuItem = createMenuItem("Open & _Edit...");
  openEditMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_E, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  openEditMenuItem->signal_activate().connect(open_edit);
  editMenuItem = createMenuItem("Edit");
  editMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_D, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  editMenuItem->signal_activate().connect(edit);
  auto saveMenuitem = createMenuItem("_Save");
  saveMenuitem->add_accelerator("activate", accelgroup, GDK_KEY_S, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  saveMenuitem->signal_activate().connect(save);
  auto saveAsMenuItem = createMenuItem("Save _As...");
  saveAsMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_S, Gdk::ModifierType::CONTROL_MASK | Gdk::ModifierType::SHIFT_MASK,
                                  Gtk::AccelFlags::ACCEL_VISIBLE);
  saveAsMenuItem->signal_activate().connect(save_as);
  publishMenuItem = createMenuItem("_Publish...");
  publishMenuItem->set_sensitive(false); // disable
  publishMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_P, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  publishMenuItem->signal_activate().connect(publish);
  auto quitMenuItem = createMenuItem("_Quit");
  quitMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_Q, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  quitMenuItem->signal_activate().connect(quit);

  // Edit sub-menu
  auto undoMenuItem = createMenuItem("_Undo");
  undoMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_Z, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  undoMenuItem->signal_activate().connect(undo);
  auto redoMenuItem = createMenuItem("_Redo");
  redoMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_Y, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  redoMenuItem->signal_activate().connect(redo);
  auto cutMenuItem = createMenuItem("Cu_t");
  cutMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_X, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  cutMenuItem->signal_activate().connect(cut);
  auto copyMenuItem = createMenuItem("_Copy");
  copyMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_C, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  copyMenuItem->signal_activate().connect(copy);
  auto pasteMenuItem = createMenuItem("_Paste");
  pasteMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_V, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  pasteMenuItem->signal_activate().connect(paste);
  auto deleteMenuItem = createMenuItem("_Delete");
  deleteMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_Delete, (Gdk::ModifierType)0, Gtk::AccelFlags::ACCEL_VISIBLE);
  deleteMenuItem->signal_activate().connect(del);
  auto selectAllMenuItem = createMenuItem("Select _All");
  selectAllMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_A, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  selectAllMenuItem->signal_activate().connect(select_all);
  auto findMenuItem = createMenuItem("_Find");
  findMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_F, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  findMenuItem->signal_activate().connect(find);
  auto replaceMenuItem = createMenuItem("_Replace");
  replaceMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_H, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  replaceMenuItem->signal_activate().connect(replace);

  // View sub-menu
  backMenuItem = createMenuItem("_Previous Page");
  backMenuItem->set_sensitive(false);
  backMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_Left, Gdk::ModifierType::MOD1_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  backMenuItem->signal_activate().connect(back);
  forwardMenuItem = createMenuItem("_Next page");
  forwardMenuItem->set_sensitive(false);
  forwardMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_Right, Gdk::ModifierType::MOD1_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  forwardMenuItem->signal_activate().connect(forward);
  auto reloadMenuItem = createMenuItem("_Reload Page");
  reloadMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_R, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  reloadMenuItem->signal_activate().connect(reload);
  auto homePageMenuItem = createMenuItem("_Homepage");
  homePageMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_Home, Gdk::ModifierType::MOD1_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
  homePageMenuItem->signal_activate().connect(home);
  auto sourceCodeMenuItem = createMenuItem("View _Source");
  sourceCodeMenuItem->signal_activate().connect(source_code);

  // Help subm-enu
  auto aboutMenuItem = createMenuItem("_About");
  aboutMenuItem->signal_activate().connect(about);

  // Add items to sub-menus
  m_fileSubmenu.append(*newDocumentMenuItem);
  m_fileSubmenu.append(*openMenuItem);
  m_fileSubmenu.append(*openEditMenuItem);
  m_fileSubmenu.append(*editMenuItem);
  m_fileSubmenu.append(m_separator1);
  m_fileSubmenu.append(*saveMenuitem);
  m_fileSubmenu.append(*saveAsMenuItem);
  m_fileSubmenu.append(m_separator2);
  m_fileSubmenu.append(*publishMenuItem);
  m_fileSubmenu.append(m_separator3);
  m_fileSubmenu.append(*quitMenuItem);
  m_editSubmenu.append(*undoMenuItem);
  m_editSubmenu.append(*redoMenuItem);
  m_editSubmenu.append(m_separator4);
  m_editSubmenu.append(*cutMenuItem);
  m_editSubmenu.append(*copyMenuItem);
  m_editSubmenu.append(*pasteMenuItem);
  m_editSubmenu.append(*deleteMenuItem);
  m_editSubmenu.append(m_separator5);
  m_editSubmenu.append(*selectAllMenuItem);
  m_editSubmenu.append(m_separator6);
  m_editSubmenu.append(*findMenuItem);
  m_editSubmenu.append(*replaceMenuItem);
  m_viewSubmenu.append(*backMenuItem);
  m_viewSubmenu.append(*forwardMenuItem);
  m_viewSubmenu.append(*reloadMenuItem);
  m_viewSubmenu.append(*homePageMenuItem);
  m_viewSubmenu.append(m_separator7);
  m_viewSubmenu.append(*sourceCodeMenuItem);
  m_helpSubmenu.append(*aboutMenuItem);

  // Add sub-menus to menus
  m_file.set_submenu(m_fileSubmenu);
  m_edit.set_submenu(m_editSubmenu);
  m_view.set_submenu(m_viewSubmenu);
  m_help.set_submenu(m_helpSubmenu);
  // Add menus to menu bar
  append(m_file);
  append(m_edit);
  append(m_view);
  append(m_help);
}

Menu::~Menu()
{
}

void Menu::setBackMenuSensitive(bool sensitive)
{
  backMenuItem->set_sensitive(sensitive);
}

void Menu::setForwardMenuSensitive(bool sensitive)
{
  forwardMenuItem->set_sensitive(sensitive);
}

void Menu::setPublishMenuSensitive(bool sensitive)
{
  publishMenuItem->set_sensitive(sensitive);
}

void Menu::setEditMenuSensitive(bool sensitive)
{
  editMenuItem->set_sensitive(sensitive);
}

/**
 * \brief Helper method for creating a menu with an image
 * \return GTKWidget menu item pointer
 */
Gtk::MenuItem* Menu::createMenuItem(const Glib::ustring& label_text)
{
  Gtk::MenuItem* item = Gtk::manage(new Gtk::MenuItem(label_text, true));
  return item;
}
