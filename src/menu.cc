#include "menu.h"

Menu::Menu(const Glib::RefPtr<Gtk::AccelGroup> &accelgroup)
    : m_file("_File", true),
      m_edit("_Edit", true),
      m_view("_View", true),
      m_help("_Help", true)
{
    // Create accelerator group
    // File sub-menu
    auto quitMenuItem = createMenuItem("_Quit");
    quitMenuItem->add_accelerator("activate", accelgroup, GDK_KEY_Q, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);
    quitMenuItem->signal_activate().connect(quit);

    // Edit sub-menu
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
    auto sourceCodeMenuItem = createMenuItem("View _Source");
    sourceCodeMenuItem->signal_activate().connect(source_code);

    // Help subm-enu
    auto aboutMenuItem = createMenuItem("_About");
    aboutMenuItem->signal_activate().connect(about);

    // Add items to sub-menus
    m_fileSubmenu.append(*quitMenuItem);
    m_editSubmenu.append(*cutMenuItem);
    m_editSubmenu.append(*copyMenuItem);
    m_editSubmenu.append(*pasteMenuItem);
    m_editSubmenu.append(*deleteMenuItem);
    m_editSubmenu.append(m_separator1);
    m_editSubmenu.append(*selectAllMenuItem);
    m_editSubmenu.append(m_separator2);
    m_editSubmenu.append(*findMenuItem);    
    m_viewSubmenu.append(*backMenuItem);
    m_viewSubmenu.append(*forwardMenuItem);
    m_viewSubmenu.append(*reloadMenuItem);
    m_viewSubmenu.append(m_separator3);
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

/**
 * \brief Helper method for creating a menu with an image
 * \return GTKWidget menu item pointer
 */
Gtk::MenuItem *Menu::createMenuItem(const Glib::ustring &label_text)
{
    Gtk::MenuItem *item = Gtk::manage(new Gtk::MenuItem(label_text, true));
    return item;
}
