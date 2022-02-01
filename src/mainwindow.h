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
#include <gtkmm/menubar.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/modelbutton.h>
#include <gtkmm/paned.h>
#include <gtkmm/popover.h>
#include <gtkmm/popovermenu.h>
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

struct cmark_node;

/**
 * \class MainWindow
 * \brief Main Application Window
 */
class MainWindow : public Gtk::Window
{
public:
  static const int DEFAULT_FONT_SIZE = 10;
  explicit MainWindow(const std::string& timeout);
  void preRequest(const std::string& path, const std::string& title, bool isSetAddressBar, bool isHistoryRequest, bool isDisableEditor);
  void postWrite(const std::string& path, const std::string& title, bool isSetAddressAndTitle);
  void startedRequest();
  void finishedRequest();
  void refreshRequest();
  void showStartpage();
  void setText(const Glib::ustring& content);
  void setDocument(cmark_node* rootNode);
  void setMessage(const Glib::ustring& message, const Glib::ustring& details = "");
  void updateStatusPopoverAndIcon();

protected:
  // Signal handlers
  bool delete_window(GdkEventAny* any_event);
  void cut();
  void copy();
  void paste();
  void del();
  void selectAll();
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
  void on_spacing_changed();
  void on_margins_changed();
  void on_indent_changed();
  void on_brightness_changed();
  void on_theme_changed();
  void on_icon_theme_activated(Gtk::ListBoxRow* row);

  Glib::RefPtr<Gtk::AccelGroup> m_accelGroup;           /*!< Accelerator group, used for keyboard shortcut bindings */
  Glib::RefPtr<Gio::Settings> m_settings;               /*!< Settings to store our preferences, even during restarts */
  Glib::RefPtr<Gtk::Adjustment> m_brightnessAdjustment; /*!< Bridghtness adjustment settings */
  Glib::RefPtr<Gtk::Adjustment> m_spacingAdjustment;    /*!< Spacing adjustment settings */
  Glib::RefPtr<Gtk::Adjustment> m_marginsAdjustment;    /*!< Margins adjustment settings */
  Glib::RefPtr<Gtk::Adjustment> m_indentAdjustment;     /*!< Indent adjustment settings */
  Glib::RefPtr<Gtk::CssProvider> m_drawCSSProvider;     /*!< CSS Provider for draw textviews */

  // Child widgets
  Menu m_menu;
  Draw m_draw_main;
  Draw m_draw_secondary;
  SourceCodeDialog m_sourceCodeDialog;
  About m_about;
  Gtk::TreeView tocTreeView;
  Glib::RefPtr<Gtk::TreeStore> m_tocTreeModel;
  Gtk::HPaned m_panedRoot;
  Gtk::HPaned m_panedDraw;
  Gtk::SearchBar m_search;
  Gtk::SearchBar m_searchReplace;
  Gtk::SearchEntry m_searchEntry;
  Gtk::Entry m_searchReplaceEntry;
  Gtk::Box m_vbox;
  Gtk::Box m_hboxBrowserToolbar;
  Gtk::Box m_hboxStandardEditorToolbar;
  Gtk::Box m_hboxFormattingEditorToolbar;
  Gtk::Box m_hboxSearch;
  Gtk::Box m_vboxSearch;
  Gtk::Box m_vboxStatus;
  Gtk::Box m_vboxSettings;
  Gtk::Box m_hboxSetingsZoom;
  Gtk::Box m_hboxSetingsBrightness;
  Gtk::Box m_vboxIconTheme;
  Gtk::ScrolledWindow m_iconThemeListScrolledWindow;
  Gtk::ListBox m_iconThemeListBox;
  Gtk::Scale m_scaleSettingsBrightness;
  Gtk::Entry m_addressBar;
  Gtk::ToggleButton m_searchMatchCase;
  Gtk::Button m_zoomOutButton;
  Gtk::Button m_zoomRestoreButton;
  Gtk::Button m_zoomInButton;
  Gtk::FontButton m_fontButton;
  Gtk::SpinButton m_spacingSpinButton;
  Gtk::SpinButton m_marginsSpinButton;
  Gtk::SpinButton m_indentSpinButton;
  Gtk::ModelButton m_iconThemeButton;
  Gtk::ModelButton m_aboutButton;
  Gtk::ModelButton m_iconThemeBackButton;
  Gtk::Grid m_statusGrid;
  Gtk::Grid m_activityStatusGrid;
  Gtk::Grid m_settingsGrid;
  ToolbarButton m_tocButton;
  ToolbarButton m_backButton;
  ToolbarButton m_forwardButton;
  ToolbarButton m_refreshButton;
  ToolbarButton m_homeButton;
  Gtk::MenuButton m_searchButton;
  Gtk::MenuButton m_statusButton;
  Gtk::MenuButton m_settingsButton;
  ToolbarButton m_openButton;
  ToolbarButton m_saveButton;
  ToolbarButton m_publishButton;
  ToolbarButton m_cutButton;
  ToolbarButton m_copyButton;
  ToolbarButton m_pasteButton;
  ToolbarButton m_undoButton;
  ToolbarButton m_redoButton;
  Gtk::ComboBoxText m_headingsComboBox;
  ToolbarButton m_boldButton;
  ToolbarButton m_italicButton;
  ToolbarButton m_strikethroughButton;
  ToolbarButton m_superButton;
  ToolbarButton m_subButton;
  ToolbarButton m_linkButton;
  ToolbarButton m_imageButton;
  ToolbarButton m_emojiButton;
  ToolbarButton m_quoteButton;
  ToolbarButton m_codeButton;
  ToolbarButton m_bulletListButton;
  ToolbarButton m_numberedListButton;
  ToolbarButton m_highlightButton;
  Gtk::Image m_zoomOutImage;
  Gtk::Image m_zoomInImage;
  Gtk::Image m_brightnessImage;
  Gtk::Image m_tocIcon;
  Gtk::Image m_backIcon;
  Gtk::Image m_forwardIcon;
  Gtk::Image m_refreshIcon;
  Gtk::Image m_homeIcon;
  Gtk::Image m_searchIcon;
  Gtk::Image m_statusIcon;
  Glib::RefPtr<Gdk::Pixbuf> m_statusOfflineIcon;
  Glib::RefPtr<Gdk::Pixbuf> m_statusOnlineIcon;
  Gtk::Image m_settingsIcon;
  Gtk::Image m_openIcon;
  Gtk::Image m_saveIcon;
  Gtk::Image m_publishIcon;
  Gtk::Image m_cutIcon;
  Gtk::Image m_copyIcon;
  Gtk::Image m_pasteIcon;
  Gtk::Image m_undoIcon;
  Gtk::Image m_redoIcon;
  Gtk::Image m_boldIcon;
  Gtk::Image m_italicIcon;
  Gtk::Image m_strikethroughIcon;
  Gtk::Image m_superIcon;
  Gtk::Image m_subIcon;
  Gtk::Image m_linkIcon;
  Gtk::Image m_imageIcon;
  Gtk::Image m_emojiIcon;
  Gtk::Image m_quoteIcon;
  Gtk::Image m_codeIcon;
  Gtk::Image m_bulletListIcon;
  Gtk::Image m_numberedListIcon;
  Gtk::Image m_hightlightIcon;
  Gtk::Popover m_searchPopover;
  Gtk::Popover m_statusPopover;
  Gtk::PopoverMenu m_settingsPopover;
  Gtk::ModelButton m_copyIDButton;
  Gtk::ModelButton m_copyPublicKeyButton;
  Gtk::Switch m_themeSwitch;
  Gtk::Label m_networkHeadingLabel;
  Gtk::Label m_networkRateHeadingLabel;
  Gtk::Label m_connectivityLabel;
  Gtk::Label m_connectivityStatusLabel;
  Gtk::Label m_peersLabel;
  Gtk::Label m_peersStatusLabel;
  Gtk::Label m_repoSizeLabel;
  Gtk::Label m_repoSizeStatusLabel;
  Gtk::Label m_repoPathLabel;
  Gtk::Label m_repoPathStatusLabel;
  Gtk::Label m_ipfsVersionLabel;
  Gtk::Label m_ipfsVersionStatusLabel;
  Gtk::Label m_networkIncomingLabel;
  Gtk::Label m_networkIncomingStatusLabel;
  Gtk::Label m_networkOutcomingLabel;
  Gtk::Label m_networkOutcomingStatusLabel;
  Gtk::Label m_networkKiloBytesLabel;
  Gtk::Label m_fontLabel;
  Gtk::Label m_spacingLabel;
  Gtk::Label m_marginsLabel;
  Gtk::Label m_indentLabel;
  Gtk::Label m_themeLabel;
  Gtk::Label m_iconThemeLabel;
  std::unique_ptr<Gtk::MessageDialog> m_contentPublishedDialog;
  Gtk::ScrolledWindow m_scrolledToc;
  Gtk::ScrolledWindow m_scrolledWindowMain;
  Gtk::ScrolledWindow m_scrolledWindowSecondary;
  Gtk::SeparatorMenuItem m_separator1;
  Gtk::SeparatorMenuItem m_separator2;
  Gtk::SeparatorMenuItem m_separator3;
  Gtk::SeparatorMenuItem m_separator4;
  Gtk::Separator m_separator5;
  Gtk::Separator m_separator6;
  Gtk::Separator m_separator7;
  Gtk::Separator m_separator8;
  Gtk::Separator m_separator9;
  Gtk::Separator m_separator10;
  TocModelCols m_tocColumns;

private:
  Middleware middleware_;
  std::string appName_;
  bool useCurrentGTKIconTheme_;
  std::string iconTheme_;
  int iconSize_;
  std::string fontFamily_;
  int defaultFontSize_;
  int currentFontSize_;
  int fontSpacing_;
  double brightnessScale_;
  bool useDarkTheme_;
  std::string currentFileSavedPath_;
  std::size_t currentHistoryIndex_;
  std::vector<std::string> history_;
  sigc::connection textChangedSignalHandler_;

  void loadStoredSettings();
  void setGTKIcons();
  void loadIcons();
  void initButtons();
  void setTheme();
  void initSearchPopover();
  void initStatusPopover();
  void initSettingsPopover();
  void initSignals();
  bool isInstalled();
  void enableEdit();
  void disableEdit();
  bool isEditorEnabled();
  std::string getIconImageFromTheme(const std::string& iconName, const std::string& typeofIcon);
  void updateCSS();
  void showNotification(const Glib::ustring& title, const Glib::ustring& message = "");
};

#endif
