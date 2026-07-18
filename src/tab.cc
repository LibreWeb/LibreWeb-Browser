#include "tab.h"

/**
 * Tab constructor
 * \param main_window Reference to the main window, used by the middleware for GUI callbacks
 * \param timeout Freedom Names time-out setting, passed to the middleware
 */
Tab::Tab(MainWindow& main_window, const std::string& timeout)
    : middleware(main_window, *this, timeout),
      draw_primary(middleware),
      draw_secondary(middleware),
      tab_label_hbox(Gtk::ORIENTATION_HORIZONTAL, 4),
      tab_label("New Tab"),
      current_history_index(0),
      is_editor_active(false),
      is_loading(false)
{
  // Primary drawing area
  scrolled_window_primary.add(draw_primary);
  scrolled_window_primary.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  // Secondary drawing area (used by the editor preview)
  draw_secondary.set_view_source_menu_item(false);
  scrolled_window_secondary.add(draw_secondary);
  scrolled_window_secondary.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  pack1(scrolled_window_primary, true, false);
  pack2(scrolled_window_secondary, true, true);

  // Tab header: title label + close button
  tab_label.set_ellipsize(Pango::EllipsizeMode::ELLIPSIZE_END);
  tab_label.set_width_chars(15);
  tab_label.set_xalign(0.0);
  tab_close_icon.set_from_icon_name("window-close-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  tab_close_button.add(tab_close_icon);
  tab_close_button.set_relief(Gtk::RELIEF_NONE);
  tab_close_button.set_focus_on_click(false);
  tab_close_button.set_tooltip_text("Close tab (Ctrl+W)");
  tab_label_hbox.pack_start(tab_label, true, true, 0);
  tab_label_hbox.pack_end(tab_close_button, false, false, 0);
  tab_label_hbox.show_all();
}

Tab::~Tab()
{
}

/**
 * \brief Update the text shown in the notebook tab header
 * \param text Tab title text
 */
void Tab::set_tab_label(const Glib::ustring& text)
{
  tab_label.set_text(text.empty() ? "New Tab" : text);
  tab_label.set_tooltip_text(text);
}
