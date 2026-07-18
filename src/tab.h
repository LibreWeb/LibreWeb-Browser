#ifndef TAB_H
#define TAB_H

#include "draw.h"
#include "middleware.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <sigc++/connection.h>
#include <string>
#include <vector>

class MainWindow;

/**
 * \class Tab
 * \brief A single browser tab: primary/secondary drawing areas with their own middleware,
 * browsing history and editor state. The Tab itself is the paned widget between the
 * primary text-view (left) and the secondary text-view (right, used as editor preview).
 */
class Tab : public Gtk::HPaned
{
public:
  Tab(MainWindow& main_window, const std::string& timeout);
  virtual ~Tab();

  void set_tab_label(const Glib::ustring& text);

  Middleware middleware;
  Draw draw_primary;
  Draw draw_secondary;
  Gtk::ScrolledWindow scrolled_window_primary;
  Gtk::ScrolledWindow scrolled_window_secondary;

  // Tab header widgets (displayed inside the notebook tab bar)
  Gtk::Box tab_label_hbox;
  Gtk::Label tab_label;
  Gtk::Button tab_close_button;
  Gtk::Image tab_close_icon;

  // Per-tab state
  std::vector<std::string> history;
  std::size_t current_history_index;
  std::string current_file_saved_path; /*!< Current file path for the 'save' feature */
  std::string address;                 /*!< Address bar text of this tab */
  Glib::ustring title;                 /*!< Page title of this tab */
  bool is_editor_active;               /*!< True when this tab is in editor mode */
  bool is_loading;                     /*!< True while a request is in progress */
  sigc::connection text_changed_signal_handler;
};

#endif
