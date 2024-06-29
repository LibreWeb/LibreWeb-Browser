#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "about-dialog.h"
#include "draw.h"
#include "menu.h"
#include "middleware.h"
#include "source-code-dialog.h"
#include "toc-model-cols.h"
#include "toolbar-button.h"

#include <giomm/settings.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/entry.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/fontbutton.h>
#include <gtkmm/grid.h>
#include <gtkmm/listbox.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/modelbutton.h>
#include <gtkmm/paned.h>
#include <gtkmm/popover.h>
#include <gtkmm/popovermenu.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/scale.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/searchbar.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/separator.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/switch.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/window.h>
#include <sigc++/connection.h>
#include <string>
#if defined(__APPLE__)
#include <gtkosxapplication.h>
#endif

struct cmark_node;

/**
 * \class MainWindow
 * \brief Main Application Window
 */
class MainWindow : public Gtk::Window
{
public:
  static const int DefaultFontSize = 10;
  explicit MainWindow(const std::string& timeout);
  void pre_request(const std::string& path, const std::string& title, bool is_set_address_bar, bool is_history_request, bool is_disable_editor);
  void post_write(const std::string& path, const std::string& title, bool is_set_address_and_title);
  void started_request();
  void finished_request();
  void refresh_request();
  void show_homepage();
  void set_text(const Glib::ustring& content);
  void set_document(cmark_node* root_node);
  void set_message(const Glib::ustring& message, const Glib::ustring& details = "");
  void update_status_popover_and_icon();

protected:
  // Signal handlers
  bool delete_window(GdkEventAny* any_event);
  void cut();
  void copy();
  void paste();
  void del();
  void selectAll();
  void on_size_alloc(const Gdk::Rectangle& allocation);
  void on_toc_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
  void new_doc();
  void open();
  void open_and_edit();
  void on_open_dialog_response(int response_id, Gtk::FileChooserDialog* dialog);
  void on_open_edit_dialog_response(int response_id, Gtk::FileChooserDialog* dialog);
  void edit();
  void save();
  void save_as();
  void on_save_as_dialog_response(int response_id, Gtk::FileChooserDialog* dialog);
  void publish();
  void go_home();
  void show_toc();
  void copy_client_id();
  void copy_client_public_key();
  void address_bar_activate();
  void on_search();
  void on_replace();
  void show_search(bool replace);
  void back();
  void forward();
  void on_button_clicked(Glib::ustring data);
  void show_about();
  void hide_about(int response);
  void editor_changed_text();
  void show_source_code_dialog();
  void get_heading();
  void insert_emoji();
  void on_zoom_out();
  void on_zoom_restore();
  void on_zoom_in();
  void on_font_set();
  void on_max_content_width_changed();
  void on_spacing_changed();
  void on_margins_changed();
  void on_indent_changed();
  void on_wrap_toggled(Gtk::WrapMode mode);
  void on_brightness_changed();
  void on_theme_changed();
  void on_reader_view_changed();
  void on_icon_theme_activated(Gtk::ListBoxRow* row);

  Glib::RefPtr<Gtk::AccelGroup> accel_group;                  /*!< Accelerator group, used for keyboard shortcut bindings */
  Glib::RefPtr<Gio::Settings> settings;                       /*!< Settings to store our preferences, even during restarts */
  Glib::RefPtr<Gtk::Adjustment> brightness_adjustment;        /*!< Bridghtness adjustment settings */
  Glib::RefPtr<Gtk::Adjustment> max_content_width_adjustment; /*!< max content width adjustment settings */
  Glib::RefPtr<Gtk::Adjustment> spacing_adjustment;           /*!< Spacing adjustment settings */
  Glib::RefPtr<Gtk::Adjustment> margins_adjustment;           /*!< Margins adjustment settings */
  Glib::RefPtr<Gtk::Adjustment> indent_adjustment;            /*!< Indent adjustment settings */
  Glib::RefPtr<Gtk::CssProvider> draw_css_provider;           /*!< CSS Provider for draw textviews */

  // Child widgets
  Menu menu;
  Draw draw_primary;
  Draw draw_secondary;
  SourceCodeDialog source_code_dialog;
  About about;
  Gtk::TreeView toc_tree_view;
  Glib::RefPtr<Gtk::TreeStore> toc_tree_model;
  Gtk::HPaned paned_root;
  Gtk::HPaned paned_draw;
  Gtk::SearchBar search;
  Gtk::SearchBar search_replace;
  Gtk::SearchEntry search_entry;
  Gtk::Entry search_replace_entry;
  Gtk::Box vbox_main;
  Gtk::Box hbox_browser_toolbar;
  Gtk::Box hbox_standard_editor_toolbar;
  Gtk::Box hbox_formatting_editor_toolbar;
  Gtk::Box hbox_search;
  Gtk::Box hbox_toc;
  Gtk::Box vbox_toc;
  Gtk::Box vbox_search;
  Gtk::Box vbox_status;
  Gtk::Box vbox_settings;
  Gtk::Box hbox_setings_zoom;
  Gtk::Box hbox_setings_brightness;
  Gtk::Box vbox_icon_theme;
  Gtk::ScrolledWindow icon_theme_list_scrolled_window;
  Gtk::ListBox icon_theme_list_box;
  Gtk::Scale scale_settings_brightness;
  Gtk::Entry address_bar;
  Gtk::ToggleButton search_match_case;
  Gtk::Button zoom_out_button;
  Gtk::Button zoom_restore_button;
  Gtk::Button zoom_in_button;
  Gtk::FontButton font_button;
  Gtk::SpinButton max_content_width_spin_button;
  Gtk::SpinButton spacing_spin_button;
  Gtk::SpinButton margins_spin_button;
  Gtk::SpinButton indent_spin_button;
  Gtk::RadioButton::Group wrapping_group;
  Gtk::RadioButton wrap_none;
  Gtk::RadioButton wrap_char;
  Gtk::RadioButton wrap_word;
  Gtk::RadioButton wrap_word_char;
  Gtk::ModelButton icon_theme_button;
  Gtk::ModelButton about_button;
  Gtk::ModelButton icon_theme_back_button;
  Gtk::Grid status_grid;
  Gtk::Grid activity_status_grid;
  Gtk::Grid settings_grid;
  ToolbarButton close_toc_window_button;
  ToolbarButton open_toc_button;
  ToolbarButton back_button;
  ToolbarButton forward_button;
  ToolbarButton refresh_button;
  ToolbarButton home_button;
  Gtk::MenuButton search_button;
  Gtk::MenuButton status_button;
  Gtk::MenuButton settings_button;
  ToolbarButton open_button;
  ToolbarButton save_button;
  ToolbarButton publish_button;
  ToolbarButton cut_button;
  ToolbarButton copy_button;
  ToolbarButton paste_button;
  ToolbarButton undo_button;
  ToolbarButton redo_button;
  Gtk::ComboBoxText headings_combo_box;
  ToolbarButton bold_button;
  ToolbarButton italic_button;
  ToolbarButton strikethrough_button;
  ToolbarButton super_button;
  ToolbarButton sub_button;
  ToolbarButton link_button;
  ToolbarButton image_button;
  ToolbarButton emoji_button;
  ToolbarButton quote_button;
  ToolbarButton code_button;
  ToolbarButton bullet_list_button;
  ToolbarButton numbered_list_button;
  ToolbarButton highlight_button;
  Gtk::Image zoom_out_image;
  Gtk::Image zoom_in_image;
  Gtk::Image brightness_image;
  Gtk::Image toc_icon;
  Gtk::Image back_icon;
  Gtk::Image forward_icon;
  Gtk::Image refresh_icon;
  Gtk::Image home_icon;
  Gtk::Image search_icon;
  Gtk::Image status_icon;
  Glib::RefPtr<Gdk::Pixbuf> status_offline_icon;
  Glib::RefPtr<Gdk::Pixbuf> status_online_icon;
  Gtk::Image settings_icon;
  Gtk::Image open_icon;
  Gtk::Image save_icon;
  Gtk::Image publish_icon;
  Gtk::Image cut_icon;
  Gtk::Image copy_icon;
  Gtk::Image paste_icon;
  Gtk::Image undo_icon;
  Gtk::Image redo_icon;
  Gtk::Image bold_icon;
  Gtk::Image italic_icon;
  Gtk::Image strikethrough_icon;
  Gtk::Image super_icon;
  Gtk::Image sub_icon;
  Gtk::Image link_icon;
  Gtk::Image image_icon;
  Gtk::Image emoji_icon;
  Gtk::Image quote_icon;
  Gtk::Image code_icon;
  Gtk::Image bullet_list_icon;
  Gtk::Image numbered_list_icon;
  Gtk::Image hightlight_icon;
  Gtk::Popover search_popover;
  Gtk::Popover status_popover;
  Gtk::PopoverMenu settings_popover;
  Gtk::ModelButton copy_id_button;
  Gtk::ModelButton copy_public_key_button;
  Gtk::Switch reader_view_switch;
  Gtk::Switch theme_switch;
  Gtk::Label table_of_contents_label;
  Gtk::Label network_heading_label;
  Gtk::Label network_rate_heading_label;
  Gtk::Label connectivity_label;
  Gtk::Label connectivity_status_label;
  Gtk::Label peers_label;
  Gtk::Label peers_status_label;
  Gtk::Label repo_size_label;
  Gtk::Label repo_size_status_label;
  Gtk::Label repo_path_label;
  Gtk::Label repo_path_status_label;
  Gtk::Label ipfs_version_label;
  Gtk::Label ipfs_version_status_label;
  Gtk::Label network_incoming_label;
  Gtk::Label network_incoming_status_label;
  Gtk::Label network_outgoing_label;
  Gtk::Label network_outgoing_status_label;
  Gtk::Label network_kilo_bytes_label;
  Gtk::Label font_label;
  Gtk::Label max_content_width_label;
  Gtk::Label spacing_label;
  Gtk::Label margins_label;
  Gtk::Label indent_label;
  Gtk::Label text_wrapping_label;
  Gtk::Label theme_label;
  Gtk::Label reader_view_label;
  Gtk::Label icon_theme_label;
  std::unique_ptr<Gtk::MessageDialog> content_published_dialog;
  Gtk::ScrolledWindow scrolled_toc;
  Gtk::ScrolledWindow scrolled_window_primary;
  Gtk::ScrolledWindow scrolled_window_secondary;
  Gtk::SeparatorMenuItem separator1;
  Gtk::SeparatorMenuItem separator2;
  Gtk::SeparatorMenuItem separator3;
  Gtk::SeparatorMenuItem separator4;
  Gtk::Separator separator5;
  Gtk::Separator separator6;
  Gtk::Separator separator7;
  Gtk::Separator separator8;
  Gtk::Separator separator9;
  Gtk::Separator separator10;
  TocModelCols toc_columns;
#if defined(__APPLE__)
  GtkosxApplication* osx_app;
#endif

private:
  Middleware middleware_;
  std::string app_name_;
  bool use_current_gtk_icon_theme_;
  std::string icon_theme_flat_;
  std::string icon_theme_filled_;
  std::string icon_theme_none_;
  std::string current_icon_theme_;
  int icon_size_;
  std::string font_family_;
  int default_font_size_;
  int current_font_size_;
  int position_divider_draw_;
  int content_margin_;
  int content_max_width_;
  double font_spacing_;
  int indent_;
  Gtk::WrapMode wrap_mode_;
  double brightness_scale_;
  bool use_dark_theme_;
  bool is_reader_view_enabled_;
  std::string current_file_saved_path_;
  std::size_t current_history_index_;
  std::vector<std::string> history_;
  sigc::connection text_changed_signal_handler_;

  void load_stored_settings();
  void set_gtk_icons();
  void load_icons();
  void init_toolbar_buttons();
  void set_theme();
  void init_search_popover();
  void init_status_popover();
  void init_settings_popover();
  void init_table_of_contents();
  void init_signals();
  void init_mac_os();
  bool is_installed();
  void set_table_of_contents(const std::vector<Glib::RefPtr<Gtk::TextMark>>& headings);
  void enable_edit();
  void disable_edit();
  bool is_editor_enabled();
  std::string get_icon_image_from_theme(const std::string& icon_name, const std::string& typeof_icon);
  void update_margins();
  void update_css();
  void show_notification(const Glib::ustring& title, const Glib::ustring& message = "");
};

#endif
