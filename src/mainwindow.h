#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "about.h"
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
  void on_size_alloc(Gdk::Rectangle& allocation);
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
  SourceCodeDialog sourceCodeDialog;
  About about;
  Gtk::TreeView tocTreeView;
  Glib::RefPtr<Gtk::TreeStore> tocTreeModel;
  Gtk::HPaned panedRoot;
  Gtk::HPaned panedDraw;
  Gtk::SearchBar search;
  Gtk::SearchBar searchReplace;
  Gtk::SearchEntry searchEntry;
  Gtk::Entry searchReplaceEntry;
  Gtk::Box vboxMain;
  Gtk::Box hboxBrowserToolbar;
  Gtk::Box hboxStandardEditorToolbar;
  Gtk::Box hboxFormattingEditorToolbar;
  Gtk::Box hboxSearch;
  Gtk::Box hboxToc;
  Gtk::Box vboxToc;
  Gtk::Box vboxSearch;
  Gtk::Box vboxStatus;
  Gtk::Box vboxSettings;
  Gtk::Box hboxSetingsZoom;
  Gtk::Box hboxSetingsBrightness;
  Gtk::Box vboxIconTheme;
  Gtk::ScrolledWindow iconThemeListScrolledWindow;
  Gtk::ListBox iconThemeListBox;
  Gtk::Scale scaleSettingsBrightness;
  Gtk::Entry addressBar;
  Gtk::ToggleButton searchMatchCase;
  Gtk::Button zoomOutButton;
  Gtk::Button zoomRestoreButton;
  Gtk::Button zoomInButton;
  Gtk::FontButton fontButton;
  Gtk::SpinButton maxContentWidthSpinButton;
  Gtk::SpinButton spacingSpinButton;
  Gtk::SpinButton marginsSpinButton;
  Gtk::SpinButton indentSpinButton;
  Gtk::RadioButton::Group wrappingGroup;
  Gtk::RadioButton wrapNone;
  Gtk::RadioButton wrapChar;
  Gtk::RadioButton wrapWord;
  Gtk::RadioButton wrapWordChar;
  Gtk::ModelButton iconThemeButton;
  Gtk::ModelButton aboutButton;
  Gtk::ModelButton iconThemeBackButton;
  Gtk::Grid statusGrid;
  Gtk::Grid activityStatusGrid;
  Gtk::Grid settingsGrid;
  ToolbarButton closeTocWindowButton;
  ToolbarButton openTocButton;
  ToolbarButton backButton;
  ToolbarButton forwardButton;
  ToolbarButton refreshButton;
  ToolbarButton homeButton;
  Gtk::MenuButton searchButton;
  Gtk::MenuButton statusButton;
  Gtk::MenuButton settingsButton;
  ToolbarButton openButton;
  ToolbarButton saveButton;
  ToolbarButton publishButton;
  ToolbarButton cutButton;
  ToolbarButton copyButton;
  ToolbarButton pasteButton;
  ToolbarButton undoButton;
  ToolbarButton redoButton;
  Gtk::ComboBoxText headingsComboBox;
  ToolbarButton boldButton;
  ToolbarButton italicButton;
  ToolbarButton strikethroughButton;
  ToolbarButton superButton;
  ToolbarButton subButton;
  ToolbarButton linkButton;
  ToolbarButton imageButton;
  ToolbarButton emojiButton;
  ToolbarButton quoteButton;
  ToolbarButton codeButton;
  ToolbarButton bulletListButton;
  ToolbarButton numberedListButton;
  ToolbarButton highlightButton;
  Gtk::Image zoomOutImage;
  Gtk::Image zoomInImage;
  Gtk::Image brightnessImage;
  Gtk::Image tocIcon;
  Gtk::Image backIcon;
  Gtk::Image forwardIcon;
  Gtk::Image refreshIcon;
  Gtk::Image homeIcon;
  Gtk::Image searchIcon;
  Gtk::Image statusIcon;
  Glib::RefPtr<Gdk::Pixbuf> statusOfflineIcon;
  Glib::RefPtr<Gdk::Pixbuf> statusOnlineIcon;
  Gtk::Image settingsIcon;
  Gtk::Image openIcon;
  Gtk::Image saveIcon;
  Gtk::Image publishIcon;
  Gtk::Image cutIcon;
  Gtk::Image copyIcon;
  Gtk::Image pasteIcon;
  Gtk::Image undoIcon;
  Gtk::Image redoIcon;
  Gtk::Image boldIcon;
  Gtk::Image italicIcon;
  Gtk::Image strikethroughIcon;
  Gtk::Image superIcon;
  Gtk::Image subIcon;
  Gtk::Image linkIcon;
  Gtk::Image imageIcon;
  Gtk::Image emojiIcon;
  Gtk::Image quoteIcon;
  Gtk::Image codeIcon;
  Gtk::Image bulletListIcon;
  Gtk::Image numberedListIcon;
  Gtk::Image hightlightIcon;
  Gtk::Popover searchPopover;
  Gtk::Popover statusPopover;
  Gtk::PopoverMenu settingsPopover;
  Gtk::ModelButton copyIDButton;
  Gtk::ModelButton copyPublicKeyButton;
  Gtk::Switch readerViewSwitch;
  Gtk::Switch themeSwitch;
  Gtk::Label tableOfContentsLabel;
  Gtk::Label networkHeadingLabel;
  Gtk::Label networkRateHeadingLabel;
  Gtk::Label connectivityLabel;
  Gtk::Label connectivityStatusLabel;
  Gtk::Label peersLabel;
  Gtk::Label peersStatusLabel;
  Gtk::Label repoSizeLabel;
  Gtk::Label repoSizeStatusLabel;
  Gtk::Label repoPathLabel;
  Gtk::Label repoPathStatusLabel;
  Gtk::Label ipfsVersionLabel;
  Gtk::Label ipfsVersionStatusLabel;
  Gtk::Label networkIncomingLabel;
  Gtk::Label networkIncomingStatusLabel;
  Gtk::Label networkOutcomingLabel;
  Gtk::Label networkOutcomingStatusLabel;
  Gtk::Label networkKiloBytesLabel;
  Gtk::Label fontLabel;
  Gtk::Label maxContentWidthLabel;
  Gtk::Label spacingLabel;
  Gtk::Label marginsLabel;
  Gtk::Label indentLabel;
  Gtk::Label textWrappingLabel;
  Gtk::Label themeLabel;
  Gtk::Label readerViewLabel;
  Gtk::Label iconThemeLabel;
  std::unique_ptr<Gtk::MessageDialog> contentPublishedDialog;
  Gtk::ScrolledWindow scrolledToc;
  Gtk::ScrolledWindow scrolledWindowPrimary;
  Gtk::ScrolledWindow scrolledWindowSecondary;
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
  TocModelCols tocColumns;
#if defined(__APPLE__)
  GtkosxApplication* osx_app;
#endif

private:
  Middleware middleware_;
  std::string app_name_;
  bool use_current_gtk_icon_theme_;
  std::string icon_theme_;
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
  void set_table_of_contents(std::vector<Glib::RefPtr<Gtk::TextMark>> headings);
  void enable_edit();
  void disable_edit();
  bool is_editor_enabled();
  std::string get_icon_image_from_theme(const std::string& icon_name, const std::string& typeof_icon);
  void update_margins();
  void update_css();
  void show_notification(const Glib::ustring& title, const Glib::ustring& message = "");
};

#endif
