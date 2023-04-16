#include "main-window.h"

#include "menu.h"
#include "project_config.h"
#include <cstdint>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <giomm/file.h>
#include <giomm/notification.h>
#include <giomm/settingsschemasource.h>
#include <giomm/themedicon.h>
#include <glibmm/convert.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <gtkmm/image.h>
#include <gtkmm/listboxrow.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/settings.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string.h>
#include <whereami.h>

#if defined(__APPLE__)
static void osx_will_quit_cb(__attribute__((unused)) GtkosxApplication* app, __attribute__((unused)) gpointer data)
{
  gtk_main_quit();
}
#endif

MainWindow::MainWindow(const std::string& timeout)
    : accel_group(Gtk::AccelGroup::create()),
      settings(),
      brightness_adjustment(Gtk::Adjustment::create(1.0, 0.0, 1.0, 0.05, 0.1)),
      max_content_width_adjustment(Gtk::Adjustment::create(700, 0, 99999, 10, 20)),
      spacing_adjustment(Gtk::Adjustment::create(0.0, -50.0, 50.0, 0.2, 0.1)),
      margins_adjustment(Gtk::Adjustment::create(10, 0, 1000, 10, 20)),
      indent_adjustment(Gtk::Adjustment::create(0, 0, 1000, 5, 10)),
      draw_css_provider(Gtk::CssProvider::create()),
      menu(accel_group),
      draw_primary(middleware_),
      draw_secondary(middleware_),
      about(*this),
      vbox_main(Gtk::ORIENTATION_VERTICAL, 0),
      vbox_toc(Gtk::ORIENTATION_VERTICAL),
      vbox_search(Gtk::ORIENTATION_VERTICAL),
      vbox_status(Gtk::ORIENTATION_VERTICAL),
      vbox_settings(Gtk::ORIENTATION_VERTICAL),
      vbox_icon_theme(Gtk::ORIENTATION_VERTICAL),
      search_match_case("Match _Case", true),
      wrap_none(wrapping_group, "None"),
      wrap_char(wrapping_group, "Char"),
      wrap_word(wrapping_group, "Word"),
      wrap_word_char(wrapping_group, "Word+Char"),
      close_toc_window_button("Close table of contents"),
      open_toc_button("Show table of contents (Ctrl+Shift+T)", true),
      back_button("Go back one page (Alt+Left arrow)", true),
      forward_button("Go forward one page (Alt+Right arrow)", true),
      refresh_button("Reload current page (Ctrl+R)", true),
      home_button("Home page (Alt+Home)", true),
      open_button("Open document (Ctrl+O)"),
      save_button("Save document (Ctrl+S)"),
      publish_button("Publish document... (Ctrl+P)"),
      cut_button("Cut (Ctrl+X)"),
      copy_button("Copy (Ctrl+C)"),
      paste_button("Paste (Ctrl+V)"),
      undo_button("Undo text (Ctrl+Z)"),
      redo_button("Redo text (Ctrl+Y)"),
      bold_button("Add bold text"),
      italic_button("Add italic text"),
      strikethrough_button("Add strikethrough text"),
      super_button("Add superscript text"),
      sub_button("Add subscript text"),
      link_button("Add a link"),
      image_button("Add an image"),
      emoji_button("Insert emoji"),
      quote_button("Insert a quote"),
      code_button("Insert code"),
      bullet_list_button("Add a bullet list"),
      numbered_list_button("Add a numbered list"),
      highlight_button("Add highlight text"),
      table_of_contents_label("Table of Contents"),
      network_heading_label("IPFS Network"),
      network_rate_heading_label("Network rate"),
      connectivity_label("Status:"),
      peers_label("Connected peers:"),
      repo_size_label("Repo size:"),
      repo_path_label("Repo path:"),
      ipfs_version_label("IPFS version:"),
      network_incoming_label("Incoming"),
      network_outgoing_label("Outgoing"),
      network_kilo_bytes_label("Kilobytes/s"),
      font_label("Font"),
      max_content_width_label("Content width"),
      spacing_label("Spacing"),
      margins_label("Margins"),
      indent_label("Indent"),
      text_wrapping_label("Wrapping"),
      theme_label("Dark Theme"),
      reader_view_label("Reader View"),
      icon_theme_label("Active Theme"),
      // Private members
      middleware_(*this, timeout),
      app_name_("LibreWeb Browser"),
      use_current_gtk_icon_theme_(false), // Use LibreWeb icon theme or the GTK icons
      icon_theme_flat_("flat"),
      icon_theme_filled_("filled"),
      icon_theme_none_("none"),
      current_icon_theme_(icon_theme_flat_), // Default is flat built-in theme
      icon_size_(18),
      font_family_("Sans"),
      default_font_size_(10),
      current_font_size_(10),
      position_divider_draw_(-1),
      content_margin_(20),
      content_max_width_(700),
      font_spacing_(0.0),
      indent_(0),
      wrap_mode_(Gtk::WRAP_WORD_CHAR),
      brightness_scale_(1.0),
      use_dark_theme_(false),
      is_reader_view_enabled_(true),
      current_history_index_(0)
{
  set_title(app_name_);
  set_default_size(1000, 800);
  set_position(Gtk::WIN_POS_CENTER);
  add_accel_group(accel_group);

  load_stored_settings();
  load_icons();
  init_toolbar_buttons();
  set_theme();
  init_search_popover();
  init_status_popover();
  init_settings_popover();
  init_table_of_contents();
  init_signals();
  init_mac_os();

  // Add custom CSS Provider to draw textviews
  auto stylePrimary = draw_primary.get_style_context();
  auto styleSecondary = draw_secondary.get_style_context();
  stylePrimary->add_provider(draw_css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
  styleSecondary->add_provider(draw_css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
  // Load the default font family and font size
  update_css();

  // Primary drawing area
  scrolled_window_primary.add(draw_primary);
  scrolled_window_primary.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  // Secondary drawing area
  draw_secondary.set_view_source_menu_item(false);
  scrolled_window_secondary.add(draw_secondary);
  scrolled_window_secondary.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  paned_draw.pack1(scrolled_window_primary, true, false);
  paned_draw.pack2(scrolled_window_secondary, true, true);
  // Left the vbox for the table of contents,
  // right the drawing paned windows (primary/secondary).
  paned_root.pack1(vbox_toc, true, false);
  paned_root.pack2(paned_draw, true, false);
  // Main virtual box
  vbox_main.pack_start(menu, false, false, 0);
  vbox_main.pack_start(hbox_browser_toolbar, false, false, 6);
  vbox_main.pack_start(hbox_standard_editor_toolbar, false, false, 6);
  vbox_main.pack_start(hbox_formatting_editor_toolbar, false, false, 6);
  vbox_main.pack_start(paned_root, true, true, 0);
  add(vbox_main);
  show_all_children();

  // Hide by default the table of contents, secondary textview, replace entry and editor toolbars
  vbox_toc.hide();
  scrolled_window_secondary.hide();
  search_replace_entry.hide();
  hbox_standard_editor_toolbar.hide();
  hbox_formatting_editor_toolbar.hide();

  // Grap focus to input field by default
  address_bar.grab_focus();

// Show homepage if debugging is disabled
#ifdef NDEBUG
  go_home();
#else
  std::cout << "INFO: Running as Debug mode, opening test.md." << std::endl;
  // Load test file during development
  middleware_.do_request("file://../../test.md");
#endif
}

/**
 * \brief Called before the requests begins.
 * \param path File path (on disk or IPFS) that needs to be processed.
 * \param title Application title
 * \param is_set_address_bar If true update the address bar with the file path
 * \param is_history_request Set to true if this is an history request call: back/forward
 * \param is_disable_editor If true the editor will be disabled if needed
 */
void MainWindow::pre_request(
    const std::string& path, const std::string& title, bool is_set_address_bar, bool is_history_request, bool is_disable_editor)
{
  if (is_set_address_bar)
    address_bar.set_text(path);
  if (!title.empty())
    set_title(title + " - " + app_name_);
  else
    set_title(app_name_);
  if (is_disable_editor && is_editor_enabled())
    disable_edit();

  // Do not insert history back/forward calls into the history (again)
  if (!is_history_request && !path.empty())
  {
    if (history_.empty())
    {
      history_.push_back(path);
      current_history_index_ = history_.size() - 1;
    }
    else if (history_.back().compare(path) != 0)
    {
      history_.push_back(path);
      current_history_index_ = history_.size() - 1;
    }
  }
  // Enable back/forward buttons when possible
  back_button.set_sensitive(current_history_index_ > 0);
  menu.set_back_menu_sensitive(current_history_index_ > 0);
  forward_button.set_sensitive(current_history_index_ < history_.size() - 1);
  menu.set_forward_menu_sensitive(current_history_index_ < history_.size() - 1);

  // Clear table of contents (ToC)
  toc_tree_model->clear();
}

/**
 * \brief Called after file is written to disk.
 */
void MainWindow::post_write(const std::string& path, const std::string& title, bool is_set_address_and_title)
{
  if (is_set_address_and_title)
  {
    address_bar.set_text(path);
    set_title(title + " - " + app_name_);
  }
}

/**
 * \brief Called when request started (from thread).
 */
void MainWindow::started_request()
{
  // Start spinning icon
  refresh_icon.get_style_context()->add_class("spinning");
}

/**
 * \brief Called when request is finished (from thread).
 */
void MainWindow::finished_request()
{
  // Stop spinning icon
  refresh_icon.get_style_context()->remove_class("spinning");
}

/**
 * \brief Refresh the current page
 */
void MainWindow::refresh_request()
{
  // Only allow refresh if editor is disabled (doesn't make sense otherwise to refresh)
  if (!is_editor_enabled())
    // Reload existing file, don't need to update the address bar, don't disable the editor
    middleware_.do_request("", false, false, false);
}

/**
 * \brief Show home page
 */
void MainWindow::show_homepage()
{
  draw_primary.show_homepage();
}

/**
 * \brief Set plain text
 * \param content content string
 */
void MainWindow::set_text(const Glib::ustring& content)
{
  draw_primary.set_text(content);
}

/**
 * \brief Set markdown document (common mark) on primary window. cmark_node pointer will be freed automatically.
 * And set the ToC.
 * \param root_node cmark root data struct
 */
void MainWindow::set_document(cmark_node* root_node)
{
  draw_primary.set_document(root_node);
  set_table_of_contents(draw_primary.get_headings());
}

/**
 * \brief Set message with optionally additional details
 * \param message Message string
 * \param details Details string
 */
void MainWindow::set_message(const Glib::ustring& message, const Glib::ustring& details)
{
  draw_primary.set_message(message, details);
}

/**
 * \brief Update all status fields in status pop-over menu + status icon
 */
void MainWindow::update_status_popover_and_icon()
{
  std::string networkStatus;
  std::size_t nrOfPeers = middleware_.get_ipfs_number_of_peers();
  // Update status icon
  if (nrOfPeers > 0)
  {
    networkStatus = "Connected";
    if (use_current_gtk_icon_theme_)
    {
      status_icon.set_from_icon_name("network-wired-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    }
    else
    {
      status_icon.set(status_online_icon);
    }
  }
  else
  {
    networkStatus = "Disconnected";
    if (use_current_gtk_icon_theme_)
    {
      status_icon.set_from_icon_name("network-wired-disconnected-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    }
    else
    {
      status_icon.set(status_offline_icon);
    }
  }
  connectivity_status_label.set_markup("<b>" + networkStatus + "</b>");
  peers_status_label.set_text(std::to_string(nrOfPeers));
  repo_size_status_label.set_text(std::to_string(middleware_.get_ipfs_repo_size()) + " MB");
  repo_path_status_label.set_text(middleware_.get_ipfs_repo_path());
  network_incoming_status_label.set_text(middleware_.get_ipfs_incoming_rate());
  network_outgoing_status_label.set_text(middleware_.get_ipfs_outgoing_rate());
  ipfs_version_status_label.set_text(middleware_.get_ipfs_version());
}

/**
 * Load stored settings from GSettings scheme file
 */
void MainWindow::load_stored_settings()
{
  // Set additional schema directory, when browser is not yet installed
  if (!is_installed())
  {
    // Relative to the binary path
    std::vector<std::string> relative_path{".."};
    std::string schema_dir = Glib::build_path(G_DIR_SEPARATOR_S, relative_path);
    std::cout << "INFO: Binary not installed. Try to find the gschema file one directory up (..)." << std::endl;
    Glib::setenv("GSETTINGS_SCHEMA_DIR", schema_dir);
  }

  // Load schema settings file
  auto schema_source = Gio::SettingsSchemaSource::get_default()->lookup("org.libreweb.browser", true);
  if (schema_source)
  {
    settings = Gio::Settings::create("org.libreweb.browser");
    // Apply global settings
    set_default_size(settings->get_int("width"), settings->get_int("height"));
    if (settings->get_boolean("maximized"))
      maximize();
    position_divider_draw_ = settings->get_int("position-divider-draw");
    paned_draw.set_position(position_divider_draw_);
    font_family_ = settings->get_string("font-family");
    current_font_size_ = default_font_size_ = settings->get_int("font-size");
    font_button.set_font_name(font_family_ + " " + std::to_string(current_font_size_));

    content_max_width_ = settings->get_int("max-content-width");
    font_spacing_ = settings->get_double("spacing");
    content_margin_ = settings->get_int("margins");
    indent_ = settings->get_int("indent");
    wrap_mode_ = static_cast<Gtk::WrapMode>(settings->get_enum("wrap-mode"));
    max_content_width_adjustment->set_value(content_max_width_);
    spacing_adjustment->set_value(font_spacing_);
    margins_adjustment->set_value(content_margin_);
    indent_adjustment->set_value(indent_);
    draw_primary.set_indent(indent_);
    int tocDividerPosition = settings->get_int("position-divider-toc");
    paned_root.set_position(tocDividerPosition);
    current_icon_theme_ = settings->get_string("icon-theme");
    use_current_gtk_icon_theme_ = settings->get_boolean("icon-gtk-theme");
    brightness_scale_ = settings->get_double("brightness");
    use_dark_theme_ = settings->get_boolean("dark-theme");
    is_reader_view_enabled_ = settings->get_boolean("reader-view");
    switch (wrap_mode_)
    {
    case Gtk::WRAP_NONE:
      wrap_none.set_active(true);
      break;
    case Gtk::WRAP_CHAR:
      wrap_char.set_active(true);
      break;
    case Gtk::WRAP_WORD:
      wrap_word.set_active(true);
      break;
    case Gtk::WRAP_WORD_CHAR:
      wrap_word_char.set_active(true);
      break;
    default:
      wrap_word_char.set_active(true);
    }
  }
  else
  {
    std::cerr << "ERROR: Gsettings schema file could not be found!" << std::endl;
    // Select default fallback wrap mode
    wrap_word_char.set_active(true);
    // Fallback adjustment controls
    max_content_width_adjustment->set_value(content_max_width_);
    spacing_adjustment->set_value(font_spacing_);
    margins_adjustment->set_value(content_max_width_);
    indent_adjustment->set_value(indent_);
    // Fallback ToC paned divider
    paned_root.set_position(300);
  }
  // Apply settings that needs to be applied now
  // Note: margins are getting automatically applied (on resize),
  // and some other attributes are part of CSS.
  draw_primary.set_indent(indent_);
  draw_primary.set_wrap_mode(wrap_mode_);
}

/**
 * \brief set GTK Icons
 */
void MainWindow::set_gtk_icons()
{
  // Toolbox buttons
  toc_icon.set_from_icon_name("view-list-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  back_icon.set_from_icon_name("go-previous", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  forward_icon.set_from_icon_name("go-next", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  refresh_icon.set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  home_icon.set_from_icon_name("go-home", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  search_icon.set_from_icon_name("edit-find-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  settings_icon.set_from_icon_name("open-menu-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  // Settings pop-over buttons
  zoom_out_image.set_from_icon_name("zoom-out-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  zoom_in_image.set_from_icon_name("zoom-in-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  brightness_image.set_from_icon_name("display-brightness-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
}

/**
 * Load all icon images from theme/disk. Or reload them.
 */
void MainWindow::load_icons()
{
  try
  {
    // Editor buttons
    open_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("open_folder", "folders"), icon_size_, icon_size_));
    save_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("floppy_disk", "basic"), icon_size_, icon_size_));
    publish_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("upload", "basic"), icon_size_, icon_size_));
    cut_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("cut", "editor"), icon_size_, icon_size_));
    copy_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("copy", "editor"), icon_size_, icon_size_));
    paste_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("clipboard", "editor"), icon_size_, icon_size_));
    undo_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("undo", "editor"), icon_size_, icon_size_));
    redo_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("redo", "editor"), icon_size_, icon_size_));
    bold_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("bold", "editor"), icon_size_, icon_size_));
    italic_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("italic", "editor"), icon_size_, icon_size_));
    strikethrough_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("strikethrough", "editor"), icon_size_, icon_size_));
    super_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("superscript", "editor"), icon_size_, icon_size_));
    sub_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("subscript", "editor"), icon_size_, icon_size_));
    link_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("link", "editor"), icon_size_, icon_size_));
    image_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("shapes", "editor"), icon_size_, icon_size_));
    emoji_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("smile", "smiley"), icon_size_, icon_size_));
    quote_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("quote", "editor"), icon_size_, icon_size_));
    code_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("code", "editor"), icon_size_, icon_size_));
    bullet_list_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("bullet_list", "editor"), icon_size_, icon_size_));
    numbered_list_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("number_list", "editor"), icon_size_, icon_size_));
    hightlight_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("highlighter", "editor"), icon_size_, icon_size_));

    if (use_current_gtk_icon_theme_)
    {
      set_gtk_icons();
    }
    else
    {
      // Toolbox buttons
      toc_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("square_list", "editor"), icon_size_, icon_size_));
      back_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("right_arrow_1", "arrows"), icon_size_, icon_size_)->flip());
      forward_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("right_arrow_1", "arrows"), icon_size_, icon_size_));
      refresh_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("reload_centered", "arrows"), icon_size_ * 1.13, icon_size_));
      home_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("home", "basic"), icon_size_, icon_size_));
      search_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("search", "basic"), icon_size_, icon_size_));
      settings_icon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("menu", "basic"), icon_size_, icon_size_));

      // Settings pop-over buttons
      zoom_out_image.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("zoom_out", "basic"), icon_size_, icon_size_));
      zoom_in_image.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("zoom_in", "basic"), icon_size_, icon_size_));
      brightness_image.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("brightness", "basic"), icon_size_, icon_size_));
      status_offline_icon = Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("network_disconnected", "network"), icon_size_, icon_size_);
      status_online_icon = Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("network_connected", "network"), icon_size_, icon_size_);
    }
  }
  catch (const Glib::FileError& error)
  {
    std::cerr << "ERROR: Icon could not be loaded, file error: " << error.what() << ".\nContinue nevertheless, with GTK icons as fallback..."
              << std::endl;
    set_gtk_icons();
    use_current_gtk_icon_theme_ = true;
  }
  catch (const Gdk::PixbufError& error)
  {
    std::cerr << "ERROR: Icon could not be loaded, pixbuf error: " << error.what() << ".\nContinue nevertheless, with GTK icons as fallback..."
              << std::endl;
    set_gtk_icons();
    use_current_gtk_icon_theme_ = true;
  }
}

/**
 * Init all buttons / comboboxes from the toolbars
 */
void MainWindow::init_toolbar_buttons()
{
  // Add icons to the toolbar editor buttons
  open_button.add(open_icon);
  save_button.add(save_icon);
  publish_button.add(publish_icon);
  cut_button.add(cut_icon);
  copy_button.add(copy_icon);
  paste_button.add(paste_icon);
  undo_button.add(undo_icon);
  redo_button.add(redo_icon);
  bold_button.add(bold_icon);
  italic_button.add(italic_icon);
  strikethrough_button.add(strikethrough_icon);
  super_button.add(super_icon);
  sub_button.add(sub_icon);
  link_button.add(link_icon);
  image_button.add(image_icon);
  emoji_button.add(emoji_icon);
  quote_button.add(quote_icon);
  code_button.add(code_icon);
  bullet_list_button.add(bullet_list_icon);
  numbered_list_button.add(numbered_list_icon);
  highlight_button.add(hightlight_icon);

  // Disable focus the other buttons as well
  search_match_case.set_can_focus(false);
  headings_combo_box.set_can_focus(false);
  headings_combo_box.set_focus_on_click(false);

  // Populate the heading comboboxtext
  headings_combo_box.append("", "Select Heading");
  headings_combo_box.append("1", "Heading 1");
  headings_combo_box.append("2", "Heading 2");
  headings_combo_box.append("3", "Heading 3");
  headings_combo_box.append("4", "Heading 4");
  headings_combo_box.append("5", "Heading 5");
  headings_combo_box.append("6", "Heading 6");
  headings_combo_box.set_active(0);

  // Horizontal bar
  back_button.get_style_context()->add_class("circular");
  forward_button.get_style_context()->add_class("circular");
  refresh_button.get_style_context()->add_class("circular");
  search_button.set_popover(search_popover);
  status_button.set_popover(status_popover);
  settings_button.set_popover(settings_popover);
  search_button.set_relief(Gtk::RELIEF_NONE);
  status_button.set_relief(Gtk::RELIEF_NONE);
  settings_button.set_relief(Gtk::RELIEF_NONE);

  // Add icons to the toolbar buttons
  open_toc_button.add(toc_icon);
  back_button.add(back_icon);
  forward_button.add(forward_icon);
  refresh_button.add(refresh_icon);
  home_button.add(home_icon);
  search_button.add(search_icon);
  status_button.add(status_icon);
  settings_button.add(settings_icon);

  // Add spinning CSS class to refresh icon
  auto screen = Gdk::Screen::get_default();
  if (screen)
  {
    auto cssProvider = Gtk::CssProvider::create();
    std::string spinningCSS = "@keyframes spin {  to { -gtk-icon-transform: rotate(1turn); }} .spinning { animation-name: spin;  "
                              "animation-duration: 1s;  animation-timing-function: linear;  animation-iteration-count: infinite;}";
    if (!cssProvider->load_from_data(spinningCSS))
    {
      std::cerr << "ERROR: CSS data parsing went wrong." << std::endl;
    }
    auto refreshIconStyle = refresh_icon.get_style_context();
    refreshIconStyle->add_provider_for_screen(screen, cssProvider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }
  // Add tooltips to the toolbar buttons
  search_button.set_tooltip_text("Find");
  status_button.set_tooltip_text("IPFS Network Status");
  settings_button.set_tooltip_text("Settings");
  // Disable back/forward buttons on start-up
  back_button.set_sensitive(false);
  forward_button.set_sensitive(false);

  /*
   * Adding the buttons to the boxes
   */
  // Browser Toolbar
  open_toc_button.set_margin_left(6);
  hbox_browser_toolbar.pack_start(open_toc_button, false, false, 0);
  hbox_browser_toolbar.pack_start(back_button, false, false, 0);
  hbox_browser_toolbar.pack_start(forward_button, false, false, 0);
  hbox_browser_toolbar.pack_start(refresh_button, false, false, 0);
  hbox_browser_toolbar.pack_start(home_button, false, false, 0);
  hbox_browser_toolbar.pack_start(address_bar, true, true, 4);
  hbox_browser_toolbar.pack_start(search_button, false, false, 0);
  hbox_browser_toolbar.pack_start(status_button, false, false, 0);
  hbox_browser_toolbar.pack_start(settings_button, false, false, 0);

  // Standard editor toolbar
  headings_combo_box.set_margin_left(4);
  hbox_standard_editor_toolbar.pack_start(open_button, false, false, 2);
  hbox_standard_editor_toolbar.pack_start(save_button, false, false, 2);
  hbox_standard_editor_toolbar.pack_start(publish_button, false, false, 2);
  hbox_standard_editor_toolbar.pack_start(separator1, false, false, 0);
  hbox_standard_editor_toolbar.pack_start(cut_button, false, false, 2);
  hbox_standard_editor_toolbar.pack_start(copy_button, false, false, 2);
  hbox_standard_editor_toolbar.pack_start(paste_button, false, false, 2);
  hbox_standard_editor_toolbar.pack_start(separator2, false, false, 0);
  hbox_standard_editor_toolbar.pack_start(undo_button, false, false, 2);
  hbox_standard_editor_toolbar.pack_start(redo_button, false, false, 2);

  // Formatting toolbar
  headings_combo_box.set_margin_left(4);
  hbox_formatting_editor_toolbar.pack_start(headings_combo_box, false, false, 2);
  hbox_formatting_editor_toolbar.pack_start(bold_button, false, false, 2);
  hbox_formatting_editor_toolbar.pack_start(italic_button, false, false, 2);
  hbox_formatting_editor_toolbar.pack_start(strikethrough_button, false, false, 2);
  hbox_formatting_editor_toolbar.pack_start(super_button, false, false, 2);
  hbox_formatting_editor_toolbar.pack_start(sub_button, false, false, 2);
  hbox_formatting_editor_toolbar.pack_start(separator3, false, false, 0);
  hbox_formatting_editor_toolbar.pack_start(link_button, false, false, 2);
  hbox_formatting_editor_toolbar.pack_start(image_button, false, false, 2);
  hbox_formatting_editor_toolbar.pack_start(emoji_button, false, false, 2);
  hbox_formatting_editor_toolbar.pack_start(separator4, false, false, 0);
  hbox_formatting_editor_toolbar.pack_start(quote_button, false, false, 2);
  hbox_formatting_editor_toolbar.pack_start(code_button, false, false, 2);
  hbox_formatting_editor_toolbar.pack_start(bullet_list_button, false, false, 2);
  hbox_formatting_editor_toolbar.pack_start(numbered_list_button, false, false, 2);
  hbox_formatting_editor_toolbar.pack_start(highlight_button, false, false, 2);
}

/**
 * \brief Prefer dark or light theme
 */
void MainWindow::set_theme()
{
  auto settings_default = Gtk::Settings::get_default();
  if (settings_default)
    settings_default->property_gtk_application_prefer_dark_theme().set_value(use_dark_theme_);
}

/**
 * \brief Popover search bar
 */
void MainWindow::init_search_popover()
{
  search_entry.set_placeholder_text("Find");
  search_replace_entry.set_placeholder_text("Replace");
  search.connect_entry(search_entry);
  search_replace.connect_entry(search_replace_entry);
  search_entry.set_size_request(250, -1);
  search_replace_entry.set_size_request(250, -1);
  vbox_search.set_margin_left(8);
  vbox_search.set_margin_right(8);
  vbox_search.set_spacing(8);
  hbox_search.set_spacing(8);

  hbox_search.pack_start(search_entry, false, false);
  hbox_search.pack_start(search_match_case, false, false);
  vbox_search.pack_start(hbox_search, false, false, 4);
  vbox_search.pack_end(search_replace_entry, false, false, 4);
  search_popover.set_position(Gtk::POS_BOTTOM);
  search_popover.set_size_request(300, 50);
  search_popover.add(vbox_search);
  search_popover.show_all_children();
}

/**
 * Init the IPFS status pop-over
 */
void MainWindow::init_status_popover()
{
  connectivity_label.set_xalign(0.0);
  peers_label.set_xalign(0.0);
  repo_size_label.set_xalign(0.0);
  repo_path_label.set_xalign(0.0);
  ipfs_version_label.set_xalign(0.0);
  connectivity_status_label.set_xalign(1.0);
  peers_status_label.set_xalign(1.0);
  repo_size_status_label.set_xalign(1.0);
  repo_path_status_label.set_xalign(1.0);
  ipfs_version_status_label.set_xalign(1.0);
  connectivity_label.get_style_context()->add_class("dim-label");
  peers_label.get_style_context()->add_class("dim-label");
  repo_size_label.get_style_context()->add_class("dim-label");
  repo_path_label.get_style_context()->add_class("dim-label");
  ipfs_version_label.get_style_context()->add_class("dim-label");
  // Status popover grid
  status_grid.set_column_homogeneous(true);
  status_grid.set_margin_start(6);
  status_grid.set_margin_top(6);
  status_grid.set_margin_bottom(6);
  status_grid.set_margin_end(12);
  status_grid.set_row_spacing(10);
  status_grid.set_column_spacing(6);
  status_grid.attach(connectivity_label, 0, 0);
  status_grid.attach(connectivity_status_label, 1, 0);
  status_grid.attach(peers_label, 0, 1);
  status_grid.attach(peers_status_label, 1, 1);
  status_grid.attach(repo_size_label, 0, 2);
  status_grid.attach(repo_size_status_label, 1, 2);
  status_grid.attach(repo_path_label, 0, 3);
  status_grid.attach(repo_path_status_label, 1, 3);
  status_grid.attach(ipfs_version_label, 0, 4);
  status_grid.attach(ipfs_version_status_label, 1, 4);
  // IPFS Network activity status grid
  network_kilo_bytes_label.get_style_context()->add_class("dim-label");
  activity_status_grid.set_column_homogeneous(true);
  activity_status_grid.set_margin_start(6);
  activity_status_grid.set_margin_top(6);
  activity_status_grid.set_margin_bottom(6);
  activity_status_grid.set_margin_end(6);
  activity_status_grid.set_row_spacing(10);
  activity_status_grid.set_column_spacing(6);
  activity_status_grid.attach(network_incoming_label, 1, 0);
  activity_status_grid.attach(network_outgoing_label, 2, 0);
  activity_status_grid.attach(network_kilo_bytes_label, 0, 1);
  activity_status_grid.attach(network_incoming_status_label, 1, 1);
  activity_status_grid.attach(network_outgoing_status_label, 2, 1);

  network_heading_label.get_style_context()->add_class("dim-label");
  network_rate_heading_label.get_style_context()->add_class("dim-label");
  // Copy ID & public key buttons
  copy_id_button.set_label("Copy your ID");
  copy_public_key_button.set_label("Copy Public Key");
  copy_id_button.set_margin_start(6);
  copy_id_button.set_margin_end(6);
  copy_public_key_button.set_margin_start(6);
  copy_public_key_button.set_margin_end(6);
  // Add all items to status box & status popover
  vbox_status.set_margin_start(10);
  vbox_status.set_margin_end(10);
  vbox_status.set_margin_top(10);
  vbox_status.set_margin_bottom(10);
  vbox_status.set_spacing(6);
  vbox_status.add(network_heading_label);
  vbox_status.add(status_grid);
  vbox_status.add(separator9);
  vbox_status.add(network_rate_heading_label);
  vbox_status.add(activity_status_grid);
  vbox_status.add(separator10);
  vbox_status.add(copy_public_key_button);
  vbox_status.add(copy_id_button);
  status_popover.set_position(Gtk::POS_BOTTOM);
  status_popover.set_size_request(100, 250);
  status_popover.set_margin_end(2);
  status_popover.add(vbox_status);
  status_popover.show_all_children();
  // Set fallback values for all status fields + status icon
  update_status_popover_and_icon();
}

/**
 * \brief Init table of contents window (left side-panel)
 */
void MainWindow::init_table_of_contents()
{
  close_toc_window_button.set_image_from_icon_name("window-close-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
  table_of_contents_label.set_margin_start(6);
  hbox_toc.pack_start(table_of_contents_label, false, false);
  hbox_toc.pack_end(close_toc_window_button, false, false);
  toc_tree_view.append_column("Level", toc_columns.col_level);
  toc_tree_view.append_column("Name", toc_columns.col_heading);
  toc_tree_view.set_activate_on_single_click(true);
  toc_tree_view.set_headers_visible(false);
  toc_tree_view.set_tooltip_column(2);
  scrolled_toc.add(toc_tree_view);
  scrolled_toc.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  toc_tree_model = Gtk::TreeStore::create(toc_columns);
  toc_tree_view.set_model(toc_tree_model);
  vbox_toc.pack_start(hbox_toc, Gtk::PackOptions::PACK_SHRINK);
  vbox_toc.pack_end(scrolled_toc);
}

/**
 * \brief Init the settings pop-over
 */
void MainWindow::init_settings_popover()
{
  // Toolbar buttons / images
  zoom_out_button.add(zoom_out_image);
  zoom_in_button.add(zoom_in_image);
  brightness_image.set_tooltip_text("Brightness");
  brightness_image.set_margin_start(2);
  brightness_image.set_margin_end(2);
  brightness_image.set_margin_top(1);
  brightness_image.set_margin_bottom(1);
  // Zoom buttons
  auto hboxZoomStyleContext = hbox_setings_zoom.get_style_context();
  hboxZoomStyleContext->add_class("linked");
  zoom_restore_button.set_sensitive(false); // By default restore button disabled
  zoom_restore_button.set_label("100%");
  zoom_out_button.set_tooltip_text("Zoom out");
  zoom_restore_button.set_tooltip_text("Restore zoom");
  zoom_in_button.set_tooltip_text("Zoom in");
  hbox_setings_zoom.set_size_request(-1, 40);
  hbox_setings_zoom.set_margin_bottom(6);
  hbox_setings_zoom.pack_start(zoom_out_button);
  hbox_setings_zoom.pack_start(zoom_restore_button);
  hbox_setings_zoom.pack_end(zoom_in_button);
  // Brightness slider
  brightness_adjustment->set_value(brightness_scale_); // Override with current loaded brightness setting
  scale_settings_brightness.set_adjustment(brightness_adjustment);
  scale_settings_brightness.add_mark(0.5, Gtk::PositionType::POS_BOTTOM, "");
  scale_settings_brightness.set_draw_value(false);
  scale_settings_brightness.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_brightness_changed));
  hbox_setings_brightness.pack_start(brightness_image, false, false);
  hbox_setings_brightness.pack_end(scale_settings_brightness);
  // Settings labels / buttons
  font_label.set_tooltip_text("Font family");
  max_content_width_label.set_tooltip_text("Max content width");
  spacing_label.set_tooltip_text("Text spacing");
  margins_label.set_tooltip_text("Text margins");
  indent_label.set_tooltip_text("Text indentation");
  text_wrapping_label.set_tooltip_text("Text wrapping");
  wrap_none.set_tooltip_text("No wrapping");
  wrap_char.set_tooltip_text("Character wrapping");
  wrap_word.set_tooltip_text("Word wrapping");
  wrap_word_char.set_tooltip_text("Word wrapping (+ character)");
  max_content_width_spin_button.set_adjustment(max_content_width_adjustment);
  spacing_spin_button.set_adjustment(spacing_adjustment);
  spacing_spin_button.set_digits(1);
  margins_spin_button.set_adjustment(margins_adjustment);
  indent_spin_button.set_adjustment(indent_adjustment);
  font_label.set_xalign(1);
  max_content_width_label.set_xalign(1);
  spacing_label.set_xalign(1);
  margins_label.set_xalign(1);
  indent_label.set_xalign(1);
  text_wrapping_label.set_xalign(1);
  theme_label.set_xalign(1);
  reader_view_label.set_xalign(1);
  font_label.get_style_context()->add_class("dim-label");
  max_content_width_label.get_style_context()->add_class("dim-label");
  spacing_label.get_style_context()->add_class("dim-label");
  margins_label.get_style_context()->add_class("dim-label");
  indent_label.get_style_context()->add_class("dim-label");
  text_wrapping_label.get_style_context()->add_class("dim-label");
  theme_label.get_style_context()->add_class("dim-label");
  reader_view_label.get_style_context()->add_class("dim-label");
  // Dark theme switch
  theme_switch.set_active(use_dark_theme_); // Override with current dark theme preference
  // Reader view switch
  reader_view_switch.set_active(is_reader_view_enabled_);
  // Settings grid
  settings_grid.set_margin_start(6);
  settings_grid.set_margin_top(6);
  settings_grid.set_margin_bottom(6);
  settings_grid.set_row_spacing(10);
  settings_grid.set_column_spacing(10);
  settings_grid.attach(font_label, 0, 0, 1);
  settings_grid.attach(font_button, 1, 0, 2);
  settings_grid.attach(max_content_width_label, 0, 1, 1);
  settings_grid.attach(max_content_width_spin_button, 1, 1, 2);
  settings_grid.attach(spacing_label, 0, 2, 1);
  settings_grid.attach(spacing_spin_button, 1, 2, 2);
  settings_grid.attach(margins_label, 0, 3, 1);
  settings_grid.attach(margins_spin_button, 1, 3, 2);
  settings_grid.attach(indent_label, 0, 4, 1);
  settings_grid.attach(indent_spin_button, 1, 4, 2);
  settings_grid.attach(text_wrapping_label, 0, 5, 1);
  settings_grid.attach(wrap_none, 1, 5, 1);
  settings_grid.attach(wrap_char, 2, 5, 1);
  settings_grid.attach(wrap_word, 1, 6, 1);
  settings_grid.attach(wrap_word_char, 2, 6, 1);
  settings_grid.attach(theme_label, 0, 7, 1);
  settings_grid.attach(theme_switch, 1, 7, 2);
  settings_grid.attach(reader_view_label, 0, 8, 1);
  settings_grid.attach(reader_view_switch, 1, 8, 2);
  // Icon theme (+ submenu)
  icon_theme_button.set_label("Icon Theme");
  icon_theme_button.property_menu_name() = "icon-theme";
  about_button.set_label("About LibreWeb");
  Gtk::Label* iconThemeButtonlabel = dynamic_cast<Gtk::Label*>(icon_theme_button.get_child());
  iconThemeButtonlabel->set_xalign(0.0);
  Gtk::Label* aboutButtonLabel = dynamic_cast<Gtk::Label*>(about_button.get_child());
  iconThemeButtonlabel->set_xalign(0.0);
  aboutButtonLabel->set_xalign(0.0);
  // Add Settings vbox to popover menu
  vbox_settings.set_margin_start(10);
  vbox_settings.set_margin_end(10);
  vbox_settings.set_margin_top(10);
  vbox_settings.set_margin_bottom(10);
  vbox_settings.set_spacing(8);
  vbox_settings.add(hbox_setings_zoom);
  vbox_settings.add(hbox_setings_brightness);
  vbox_settings.add(separator5);
  vbox_settings.add(settings_grid);
  vbox_settings.add(separator6);
  vbox_settings.add(icon_theme_button);
  vbox_settings.add(separator7);
  vbox_settings.pack_end(about_button, false, false);
  settings_popover.set_position(Gtk::POS_BOTTOM);
  settings_popover.set_size_request(200, 300);
  settings_popover.set_margin_end(2);
  settings_popover.add(vbox_settings);
  // Add Theme vbox to popover menu
  icon_theme_back_button.set_label("Icon Theme");
  icon_theme_back_button.property_menu_name() = "main";
  icon_theme_back_button.property_inverted() = true;
  // List of themes in list box
  Gtk::Label* iconTheme1 = Gtk::manage(new Gtk::Label("Flat theme"));
  Gtk::ListBoxRow* row1 = Gtk::manage(new Gtk::ListBoxRow());
  row1->add(*iconTheme1);
  row1->set_data("value", &icon_theme_flat_[0]);
  Gtk::Label* iconTheme2 = Gtk::manage(new Gtk::Label("Filled theme"));
  Gtk::ListBoxRow* row2 = Gtk::manage(new Gtk::ListBoxRow());
  row2->add(*iconTheme2);
  row2->set_data("value", &icon_theme_filled_[0]);
  Gtk::Label* iconTheme3 = Gtk::manage(new Gtk::Label("Gtk default theme"));
  Gtk::ListBoxRow* row3 = Gtk::manage(new Gtk::ListBoxRow());
  row3->add(*iconTheme3);
  row3->set_data("value", &icon_theme_none_[0]);
  icon_theme_list_box.add(*row1);
  icon_theme_list_box.add(*row2);
  icon_theme_list_box.add(*row3);
  // Select the correct row by default
  if (use_current_gtk_icon_theme_)
    icon_theme_list_box.select_row(*row3);
  else if (current_icon_theme_ == "flat")
    icon_theme_list_box.select_row(*row1);
  else if (current_icon_theme_ == "filled")
    icon_theme_list_box.select_row(*row2);
  else
    icon_theme_list_box.select_row(*row1); // flat is fallback
  icon_theme_list_scrolled_window.property_height_request() = 200;
  icon_theme_list_scrolled_window.add(icon_theme_list_box);
  icon_theme_label.get_style_context()->add_class("dim-label");
  vbox_icon_theme.add(icon_theme_back_button);
  vbox_icon_theme.add(separator8);
  vbox_icon_theme.add(icon_theme_label);
  vbox_icon_theme.add(icon_theme_list_scrolled_window);
  settings_popover.add(vbox_icon_theme);
  settings_popover.child_property_submenu(vbox_icon_theme) = "icon-theme";
  settings_popover.show_all_children();
}

/**
 * \brief Init all signals and connect them to functions
 */
void MainWindow::init_signals()
{
  // Window signals
  signal_delete_event().connect(sigc::mem_fun(this, &MainWindow::delete_window));
  draw_primary.signal_size_allocate().connect(sigc::mem_fun(this, &MainWindow::on_size_alloc));

  // Table of contents
  close_toc_window_button.signal_clicked().connect(sigc::mem_fun(vbox_toc, &Gtk::Widget::hide));
  toc_tree_view.signal_row_activated().connect(sigc::mem_fun(this, &MainWindow::on_toc_row_activated));
  // Menu & toolbar signals
  menu.new_doc.connect(sigc::mem_fun(this, &MainWindow::new_doc));                       /*!< Menu item for new document */
  menu.open.connect(sigc::mem_fun(this, &MainWindow::open));                             /*!< Menu item for opening existing document */
  menu.open_edit.connect(sigc::mem_fun(this, &MainWindow::open_and_edit));               /*!< Menu item for opening & editing existing document */
  menu.edit.connect(sigc::mem_fun(this, &MainWindow::edit));                             /*!< Menu item for editing current open document */
  menu.save.connect(sigc::mem_fun(this, &MainWindow::save));                             /*!< Menu item for save document */
  menu.save_as.connect(sigc::mem_fun(this, &MainWindow::save_as));                       /*!< Menu item for save document as */
  menu.publish.connect(sigc::mem_fun(this, &MainWindow::publish));                       /*!< Menu item for publishing */
  menu.quit.connect(sigc::mem_fun(this, &MainWindow::close));                            /*!< close main window and therefore closes the app */
  menu.undo.connect(sigc::mem_fun(draw_primary, &Draw::undo));                           /*!< Menu item for undo text */
  menu.redo.connect(sigc::mem_fun(draw_primary, &Draw::redo));                           /*!< Menu item for redo text */
  menu.cut.connect(sigc::mem_fun(this, &MainWindow::cut));                               /*!< Menu item for cut text */
  menu.copy.connect(sigc::mem_fun(this, &MainWindow::copy));                             /*!< Menu item for copy text */
  menu.paste.connect(sigc::mem_fun(this, &MainWindow::paste));                           /*!< Menu item for paste text */
  menu.del.connect(sigc::mem_fun(this, &MainWindow::del));                               /*!< Menu item for deleting selected text */
  menu.select_all.connect(sigc::mem_fun(this, &MainWindow::selectAll));                  /*!< Menu item for selecting all text */
  menu.find.connect(sigc::bind(sigc::mem_fun(this, &MainWindow::show_search), false));   /*!< Menu item for finding text */
  menu.replace.connect(sigc::bind(sigc::mem_fun(this, &MainWindow::show_search), true)); /*!< Menu item for replacing text */
  menu.back.connect(sigc::mem_fun(this, &MainWindow::back));                             /*!< Menu item for previous page */
  menu.forward.connect(sigc::mem_fun(this, &MainWindow::forward));                       /*!< Menu item for next page */
  menu.reload.connect(sigc::mem_fun(this, &MainWindow::refresh_request));                /*!< Menu item for reloading the page */
  menu.home.connect(sigc::mem_fun(this, &MainWindow::go_home));                          /*!< Menu item for home page */
  menu.toc.connect(sigc::mem_fun(this, &MainWindow::show_toc));                          /*!< Menu item for table of contents */
  menu.source_code.connect(sigc::mem_fun(this, &MainWindow::show_source_code_dialog));   /*!< Source code dialog */
  source_code_dialog.signal_response().connect(sigc::mem_fun(source_code_dialog, &SourceCodeDialog::hide_dialog)); /*!< Close source code dialog */
  menu.about.connect(sigc::mem_fun(about, &About::show_about));                                                    /*!< Display about dialog */
  draw_primary.source_code.connect(sigc::mem_fun(this, &MainWindow::show_source_code_dialog));                     /*!< Open source code dialog */
  about.signal_response().connect(sigc::mem_fun(about, &About::hide_about));                                       /*!< Close about dialog */
  address_bar.signal_activate().connect(sigc::mem_fun(this, &MainWindow::address_bar_activate)); /*!< User pressed enter the address bar */
  open_toc_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::show_toc));          /*!< Button for showing Table of Contents */
  back_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::back));                  /*!< Button for previous page */
  forward_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::forward));            /*!< Button for next page */
  refresh_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::refresh_request));    /*!< Button for reloading the page */
  home_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::go_home));               /*!< Button for home page */
  search_entry.signal_activate().connect(sigc::mem_fun(this, &MainWindow::on_search));           /*!< Execute the text search */
  search_replace_entry.signal_activate().connect(sigc::mem_fun(this, &MainWindow::on_replace));  /*!< Execute the text replace */
  // Editor toolbar buttons
  open_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::open_and_edit));
  save_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::save));
  publish_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::publish));
  cut_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::cut));
  copy_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::copy));
  paste_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::paste));
  undo_button.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::undo));
  redo_button.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::redo));
  headings_combo_box.signal_changed().connect(sigc::mem_fun(this, &MainWindow::get_heading));
  bold_button.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_bold));
  italic_button.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_italic));
  strikethrough_button.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_strikethrough));
  super_button.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_super));
  sub_button.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_sub));
  link_button.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::insert_link));
  image_button.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::insert_image));
  emoji_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::insert_emoji));
  quote_button.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_quote));
  code_button.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_code));
  bullet_list_button.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::insert_bullet_list));
  numbered_list_button.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::insert_numbered_list));
  highlight_button.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_highlight));
  // Status pop-over buttons
  copy_id_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::copy_client_id));
  copy_public_key_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::copy_client_public_key));
  // Settings pop-over buttons
  zoom_out_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::on_zoom_out));
  zoom_restore_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::on_zoom_restore));
  zoom_in_button.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::on_zoom_in));
  font_button.signal_font_set().connect(sigc::mem_fun(this, &MainWindow::on_font_set));
  max_content_width_spin_button.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_max_content_width_changed));
  spacing_spin_button.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_spacing_changed));
  margins_spin_button.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_margins_changed));
  indent_spin_button.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_indent_changed));
  wrap_none.signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &MainWindow::on_wrap_toggled), Gtk::WrapMode::WRAP_NONE));
  wrap_char.signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &MainWindow::on_wrap_toggled), Gtk::WrapMode::WRAP_CHAR));
  wrap_word.signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &MainWindow::on_wrap_toggled), Gtk::WrapMode::WRAP_WORD));
  wrap_word_char.signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &MainWindow::on_wrap_toggled), Gtk::WrapMode::WRAP_WORD_CHAR));
  theme_switch.property_active().signal_changed().connect(sigc::mem_fun(this, &MainWindow::on_theme_changed));
  reader_view_switch.property_active().signal_changed().connect(sigc::mem_fun(this, &MainWindow::on_reader_view_changed));
  icon_theme_list_box.signal_row_activated().connect(sigc::mem_fun(this, &MainWindow::on_icon_theme_activated));
  about_button.signal_clicked().connect(sigc::mem_fun(about, &About::show_about));
}

void MainWindow::init_mac_os()
{
#if defined(__APPLE__)
  {
    osx_app = reinterpret_cast<GtkosxApplication*>(g_object_new(GTKOSX_TYPE_APPLICATION, NULL));
    // TODO: Should I implement those terminate signals. Since I disabled quartz accelerators
    MainWindow* mainWindow = this;
    g_signal_connect(osx_app, "NSApplicationWillTerminate", G_CALLBACK(osx_will_quit_cb), mainWindow);
    // TODO: Open file callback?
    // g_signal_connect (osx_app, "NSApplicationOpenFile", G_CALLBACK (osx_open_file_cb), mainWindow);
    menu.hide();
    GtkWidget* menubar = reinterpret_cast<GtkWidget*>(menu.gobj());
    gtkosx_application_set_menu_bar(osx_app, GTK_MENU_SHELL(menubar));
    // Use GTK accelerators
    gtkosx_application_set_use_quartz_accelerators(osx_app, FALSE);
    gtkosx_application_ready(osx_app);
  }
#endif
}

/**
 * \brief Called when Window is closed/exited
 */
bool MainWindow::delete_window(GdkEventAny* any_event __attribute__((unused)))
{
  if (settings)
  {
    // Save the schema settings
    settings->set_int("width", get_width());
    settings->set_int("height", get_height());
    settings->set_boolean("maximized", is_maximized());
    if (paned_root.get_position() > 0)
      settings->set_int("position-divider-toc", paned_root.get_position());
    // Only store a divider value bigger than zero,
    // because the secondary draw window is hidden by default, resulting into a zero value.
    if (paned_draw.get_position() > 0)
      settings->set_int("position-divider-draw", paned_draw.get_position());
    // Fullscreen will be available with gtkmm-4.0
    // settings->set_boolean("fullscreen", is_fullscreen());
    settings->set_string("font-family", font_family_);
    settings->set_int("font-size", current_font_size_);
    settings->set_int("max-content-width", content_max_width_);
    settings->set_double("spacing", font_spacing_);
    settings->set_int("margins", content_margin_);
    settings->set_int("indent", indent_);
    settings->set_enum("wrap-mode", wrap_mode_);
    settings->set_string("icon-theme", current_icon_theme_);
    settings->set_boolean("icon-gtk-theme", use_current_gtk_icon_theme_);
    settings->set_double("brightness", brightness_scale_);
    settings->set_boolean("dark-theme", use_dark_theme_);
    settings->set_boolean("reader-view", is_reader_view_enabled_);
  }
  return false;
}

/**
 * \brief Cut/copy/paste/delete/select all keybindings
 */
void MainWindow::cut()
{
  if (draw_primary.has_focus())
  {
    draw_primary.cut();
  }
  else if (draw_secondary.has_focus())
  {
    draw_secondary.cut();
  }
  else if (address_bar.has_focus())
  {
    address_bar.cut_clipboard();
  }
  else if (search_entry.has_focus())
  {
    search_entry.cut_clipboard();
  }
  else if (search_replace_entry.has_focus())
  {
    search_replace_entry.cut_clipboard();
  }
}

void MainWindow::copy()
{
  if (draw_primary.has_focus())
  {
    draw_primary.copy();
  }
  else if (draw_secondary.has_focus())
  {
    draw_secondary.copy();
  }
  else if (address_bar.has_focus())
  {
    address_bar.copy_clipboard();
  }
  else if (search_entry.has_focus())
  {
    search_entry.copy_clipboard();
  }
  else if (search_replace_entry.has_focus())
  {
    search_replace_entry.copy_clipboard();
  }
}

void MainWindow::paste()
{
  if (draw_primary.has_focus())
  {
    draw_primary.paste();
  }
  else if (draw_secondary.has_focus())
  {
    draw_secondary.paste();
  }
  else if (address_bar.has_focus())
  {
    address_bar.paste_clipboard();
  }
  else if (search_entry.has_focus())
  {
    search_entry.paste_clipboard();
  }
  else if (search_replace_entry.has_focus())
  {
    search_replace_entry.paste_clipboard();
  }
}

void MainWindow::del()
{
  if (draw_primary.has_focus())
  {
    draw_primary.del();
  }
  else if (draw_secondary.has_focus())
  {
    draw_secondary.del();
  }
  else if (address_bar.has_focus())
  {
    int start, end;
    if (address_bar.get_selection_bounds(start, end))
    {
      address_bar.delete_text(start, end);
    }
    else
    {
      ++end;
      address_bar.delete_text(start, end);
    }
  }
  else if (search_entry.has_focus())
  {
    int start, end;
    if (search_entry.get_selection_bounds(start, end))
    {
      search_entry.delete_text(start, end);
    }
    else
    {
      ++end;
      search_entry.delete_text(start, end);
    }
  }
  else if (search_replace_entry.has_focus())
  {
    int start, end;
    if (search_replace_entry.get_selection_bounds(start, end))
    {
      search_replace_entry.delete_text(start, end);
    }
    else
    {
      ++end;
      search_replace_entry.delete_text(start, end);
    }
  }
}

void MainWindow::selectAll()
{
  if (draw_primary.has_focus())
  {
    draw_primary.select_all();
  }
  else if (draw_secondary.has_focus())
  {
    draw_secondary.select_all();
  }
  else if (address_bar.has_focus())
  {
    address_bar.select_region(0, -1);
  }
  else if (search_entry.has_focus())
  {
    search_entry.select_region(0, -1);
  }
  else if (search_replace_entry.has_focus())
  {
    search_replace_entry.select_region(0, -1);
  }
}

/**
 * \brief Triggers when the textview widget changes size
 */
void MainWindow::on_size_alloc(__attribute__((unused)) Gdk::Rectangle& allocation)
{
  if (!is_editor_enabled())
    update_margins();
}

/**
 * \brief Triggered when user clicked on the column in ToC
 */
void MainWindow::on_toc_row_activated(const Gtk::TreeModel::Path& path, __attribute__((unused)) Gtk::TreeViewColumn* column)
{
  const auto iter = toc_tree_model->get_iter(path);
  if (iter)
  {
    const auto row = *iter;
    if (row[toc_columns.col_valid])
    {
      Gtk::TextIter textIter = row[toc_columns.col_iter];
      // Scroll to to mark iterator
      if (is_editor_enabled())
        draw_secondary.scroll_to(textIter);
      else
        draw_primary.scroll_to(textIter);
    }
  }
}

/**
 * \brief Trigger when user selected 'new document' from menu item
 */
void MainWindow::new_doc()
{
  // Clear content & path
  middleware_.reset_content_and_path();
  // Enable editing mode
  enable_edit();
  // Change address bar
  address_bar.set_text("file://unsaved");
  // Set new title
  set_title("Untitled * - " + app_name_);
}

/**
 * \brief Triggered when user selected 'open...' from menu item / toolbar
 */
void MainWindow::open()
{
  auto dialog = new Gtk::FileChooserDialog("Open", Gtk::FILE_CHOOSER_ACTION_OPEN);
  dialog->set_transient_for(*this);
  dialog->set_modal(true);
  dialog->signal_response().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::on_open_dialog_response), dialog));
  dialog->add_button("_Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
  dialog->add_button("_Open", Gtk::ResponseType::RESPONSE_OK);
  // Add filters, so that only certain file types can be selected:
#ifdef __linux__
  auto filterMarkdown = Gtk::FileFilter::create();
  filterMarkdown->set_name("All Markdown files");
  filterMarkdown->add_mime_type("text/markdown");
  dialog->add_filter(filterMarkdown);
#endif
  auto filterMarkdownExt = Gtk::FileFilter::create();
  filterMarkdownExt->set_name("All Markdown files extension (*.md)");
  filterMarkdownExt->add_pattern("*.md");
  dialog->add_filter(filterMarkdownExt);
  auto filterTextFiles = Gtk::FileFilter::create();
  filterTextFiles->set_name("All text files");
  filterTextFiles->add_mime_type("text/plain");
  dialog->add_filter(filterTextFiles);
  auto filterAny = Gtk::FileFilter::create();
  filterAny->set_name("Any files");
  filterAny->add_pattern("*");
  dialog->add_filter(filterAny);
  dialog->show(); // Finally, show the open dialog
}

/**
 * \brief Triggered when user selected 'open & edit...' from menu item
 */
void MainWindow::open_and_edit()
{
  auto dialog = new Gtk::FileChooserDialog("Open & Edit", Gtk::FILE_CHOOSER_ACTION_OPEN);
  dialog->set_transient_for(*this);
  dialog->set_modal(true);
  dialog->signal_response().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::on_open_edit_dialog_response), dialog));
  dialog->add_button("_Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
  dialog->add_button("_Open", Gtk::ResponseType::RESPONSE_OK);
  // Add filters, so that only certain file types can be selected:
#ifdef __linux__
  auto filterMarkdown = Gtk::FileFilter::create();
  filterMarkdown->set_name("All Markdown files");
  filterMarkdown->add_mime_type("text/markdown");
  dialog->add_filter(filterMarkdown);
#endif
  auto filterMarkdownExt = Gtk::FileFilter::create();
  filterMarkdownExt->set_name("All Markdown files extension (*.md)");
  filterMarkdownExt->add_pattern("*.md");
  dialog->add_filter(filterMarkdownExt);
  auto filterTextFiles = Gtk::FileFilter::create();
  filterTextFiles->set_name("All text files");
  filterTextFiles->add_mime_type("text/plain");
  dialog->add_filter(filterTextFiles);
  auto filterAny = Gtk::FileFilter::create();
  filterAny->set_name("Any files");
  filterAny->add_pattern("*");
  dialog->add_filter(filterAny);
  dialog->show(); // Finally, show the open & edit dialog
}

/**
 * \brief Signal response when 'open' dialog is closed
 */
void MainWindow::on_open_dialog_response(int response_id, Gtk::FileChooserDialog* dialog)
{
  switch (response_id)
  {
  case Gtk::ResponseType::RESPONSE_OK:
  {
    auto filePath = dialog->get_file()->get_path();
    // Open file, set address bar & disable editor if needed
    middleware_.do_request("file://" + filePath);
    break;
  }
  case Gtk::ResponseType::RESPONSE_CANCEL:
  {
    break;
  }
  default:
  {
    std::cerr << "WARN: Unexpected button clicked." << std::endl;
    break;
  }
  }
  delete dialog;
}

/**
 * \brief Signal response when 'open & edit' dialog is closed
 */
void MainWindow::on_open_edit_dialog_response(int response_id, Gtk::FileChooserDialog* dialog)
{
  switch (response_id)
  {
  case Gtk::ResponseType::RESPONSE_OK:
  {
    // Enable editor if needed
    if (!is_editor_enabled())
      enable_edit();

    auto filePath = dialog->get_file()->get_path();
    std::string path = "file://" + filePath;
    // Open file and set address bar, but do not parse the content or the disable editor
    middleware_.do_request(path, true, false, false, false);
    // Set current file path for the 'save' feature
    current_file_saved_path_ = filePath;
    break;
  }
  case Gtk::ResponseType::RESPONSE_CANCEL:
  {
    break;
  }
  default:
  {
    std::cerr << "WARN: Unexpected button clicked." << std::endl;
    break;
  }
  }
  delete dialog;
}

/**
 * \brief Triggered when user selected 'edit' from menu item
 */
void MainWindow::edit()
{
  if (!is_editor_enabled())
    enable_edit();

  draw_primary.set_text(middleware_.get_content());
  // Set title
  set_title("Untitled * - " + app_name_);
}

/**
 * \brief Triggered when user selected 'save' from menu item / toolbar
 */
void MainWindow::save()
{
  if (current_file_saved_path_.empty())
  {
    save_as();
  }
  else
  {
    if (is_editor_enabled())
    {
      try
      {
        middleware_.do_write(current_file_saved_path_);
      }
      catch (std::ios_base::failure& error)
      {
        std::cerr << "ERROR: Could not write file: " << current_file_saved_path_ << ". Message: " << error.what() << ".\nError code: " << error.code()
                  << std::endl;
      }
    }
    else
    {
      std::cerr << "ERROR: Saving while \"file saved path\" is filled and editor is disabled should not happen!?" << std::endl;
    }
  }
}

/**
 * \brief Triggered when 'save as..' menu item is selected or the user saves the file for the first time via 'save'
 */
void MainWindow::save_as()
{
  auto dialog = new Gtk::FileChooserDialog("Save", Gtk::FILE_CHOOSER_ACTION_SAVE);
  dialog->set_transient_for(*this);
  dialog->set_modal(true);
  dialog->set_do_overwrite_confirmation(true);
  dialog->signal_response().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::on_save_as_dialog_response), dialog));
  dialog->add_button("_Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
  dialog->add_button("_Save", Gtk::ResponseType::RESPONSE_OK);
  // Add filters, so that only certain file types can be selected:
  auto filterMarkdownExt = Gtk::FileFilter::create();
  filterMarkdownExt->set_name("All Markdown files");
  filterMarkdownExt->add_pattern("*.md");
  dialog->add_filter(filterMarkdownExt);
  auto filterTextFiles = Gtk::FileFilter::create();
  filterTextFiles->set_name("All text files");
  filterTextFiles->add_mime_type("text/plain");
  dialog->add_filter(filterTextFiles);
  auto filterAny = Gtk::FileFilter::create();
  filterAny->set_name("Any files");
  filterAny->add_pattern("*");
  dialog->add_filter(filterAny);
  // If user is saving as an existing file, set the current uri path
  if (!current_file_saved_path_.empty())
  {
    try
    {
      dialog->set_uri(Glib::filename_to_uri(current_file_saved_path_));
    }
    catch (Glib::Error& error)
    {
      std::cerr << "ERROR: Incorrect filename most likely. Message: " << error.what() << ". Error Code: " << error.code() << std::endl;
    }
  }
  dialog->show(); // Finally, show save as dialog
}

/**
 * \brief Signal response when 'save as' dialog is closed
 */
void MainWindow::on_save_as_dialog_response(int response_id, Gtk::FileChooserDialog* dialog)
{
  switch (response_id)
  {
  case Gtk::ResponseType::RESPONSE_OK:
  {
    auto filePath = dialog->get_file()->get_path();
    if (!filePath.ends_with(".md"))
      filePath.append(".md");

    // Save current content to file path
    try
    {
      middleware_.do_write(filePath, is_editor_enabled()); // Only update address & title, when editor mode is enabled
      // Only if editor mode is enabled
      if (is_editor_enabled())
      {
        // Set/update the current file saved path variable (used for the 'save' feature)
        current_file_saved_path_ = filePath;
      }
    }
    catch (std::ios_base::failure& error)
    {
      std::cerr << "ERROR: Could not write file: " << filePath << ". Message: " << error.what() << ".\nError code: " << error.code() << std::endl;
    }
    break;
  }
  case Gtk::ResponseType::RESPONSE_CANCEL:
  {
    break;
  }
  default:
  {
    std::cerr << "ERROR: Unexpected button clicked." << std::endl;
    break;
  }
  }
  delete dialog;
}

/**
 * \brief Triggered when user selected the 'Publish...' menu item or publish button in the toolbar
 */
void MainWindow::publish()
{
  int result = Gtk::RESPONSE_YES; // By default continue
  if (middleware_.get_content().empty())
  {
    Gtk::MessageDialog dialog(*this, "Are you sure you want to publish <b>empty</b> content?", true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
    dialog.set_title("Are you sure?");
    dialog.set_default_response(Gtk::RESPONSE_NO);
    result = dialog.run();
  }

  // Continue ...
  if (result == Gtk::RESPONSE_YES)
  {
    std::string path = "new_file.md";
    // Retrieve filename from saved file (if present)
    if (!current_file_saved_path_.empty())
    {
      path = current_file_saved_path_;
    }
    else
    {
      // TODO: path is not defined yet. however, this may change anyway once we try to build more complex
      // websites, needing to use directory structures.
    }

    try
    {
      // Add content to IPFS
      std::string cid = middleware_.do_add(path);
      if (cid.empty())
      {
        throw std::runtime_error("CID hash is empty.");
      }
      // Show dialog
      content_published_dialog.reset(new Gtk::MessageDialog(*this, "File is successfully added to IPFS!"));
      content_published_dialog->set_secondary_text("The content is now available on the decentralized web, via:");
      // Add custom label
      Gtk::Label* label = Gtk::manage(new Gtk::Label("ipfs://" + cid));
      label->set_selectable(true);
      Gtk::Box* box = content_published_dialog->get_content_area();
      box->pack_end(*label);

      content_published_dialog->set_modal(true);
      // content_published_dialog->set_hide_on_close(true); available in gtk-4.0
      content_published_dialog->signal_response().connect(sigc::hide(sigc::mem_fun(*content_published_dialog, &Gtk::Widget::hide)));
      content_published_dialog->show_all();
    }
    catch (const std::runtime_error& error)
    {
      content_published_dialog.reset(new Gtk::MessageDialog(*this, "File could not be added to IPFS", false, Gtk::MESSAGE_ERROR));
      content_published_dialog->set_secondary_text("Error message: " + std::string(error.what()));
      content_published_dialog->set_modal(true);
      // content_published_dialog->set_hide_on_close(true); available in gtk-4.0
      content_published_dialog->signal_response().connect(sigc::hide(sigc::mem_fun(*content_published_dialog, &Gtk::Widget::hide)));
      content_published_dialog->show();
    }
  }
}

/**
 * \brief Show homepage
 */
void MainWindow::go_home()
{
  middleware_.do_request("about:home", true, false, true);
}

/**
 * \brief Show/hide table of contents
 */
void MainWindow::show_toc()
{
  if (vbox_toc.is_visible())
    vbox_toc.hide();
  else
    vbox_toc.show();
}

/**
 * \brief Copy the IPFS Client ID to clipboard
 */
void MainWindow::copy_client_id()
{
  if (!middleware_.get_ipfs_client_id().empty())
  {
    get_clipboard("CLIPBOARD")->set_text(middleware_.get_ipfs_client_id());
    show_notification("Copied to clipboard", "Your client ID is now copied to your clipboard.");
  }
  else
  {
    std::cerr << "WARNING: IPFS client ID has not been set yet. Skip clipboard action." << std::endl;
  }
}

/**
 * \brief Copy IPFS Client public key to clipboard
 */
void MainWindow::copy_client_public_key()
{
  if (!middleware_.get_ipfs_client_public_key().empty())
  {
    get_clipboard("CLIPBOARD")->set_text(middleware_.get_ipfs_client_public_key());
    show_notification("Copied to clipboard", "Your client public key is now copied to your clipboard.");
  }
  else
  {
    std::cerr << "WARNING: IPFS client public key has not been set yet. Skip clipboard action." << std::endl;
  }
}

/**
 * \brief Trigger when pressed enter in the search entry
 */
void MainWindow::on_search()
{
  // Forward search, find and select
  std::string text = search_entry.get_text();
  auto buffer = draw_primary.get_buffer();
  Gtk::TextBuffer::iterator iter = buffer->get_iter_at_mark(buffer->get_mark("insert"));
  Gtk::TextBuffer::iterator start, end;
  bool matchCase = search_match_case.get_active();
  Gtk::TextSearchFlags flags = Gtk::TextSearchFlags::TEXT_SEARCH_TEXT_ONLY;
  if (!matchCase)
  {
    flags |= Gtk::TextSearchFlags::TEXT_SEARCH_CASE_INSENSITIVE;
  }
  if (iter.forward_search(text, flags, start, end))
  {
    buffer->select_range(end, start);
    draw_primary.scroll_to(start);
  }
  else
  {
    buffer->place_cursor(buffer->begin());
    // Try another search directly from the top
    Gtk::TextBuffer::iterator secondIter = buffer->get_iter_at_mark(buffer->get_mark("insert"));
    if (secondIter.forward_search(text, flags, start, end))
    {
      buffer->select_range(end, start);
      draw_primary.scroll_to(start);
    }
  }
}

/**
 * \brief Trigger when user pressed enter in the replace entry
 */
void MainWindow::on_replace()
{
  if (draw_primary.get_editable())
  {
    auto buffer = draw_primary.get_buffer();
    Gtk::TextBuffer::iterator startIter = buffer->get_iter_at_mark(buffer->get_mark("insert"));
    Gtk::TextBuffer::iterator endIter = buffer->get_iter_at_mark(buffer->get_mark("selection_bound"));
    if (startIter != endIter)
    {
      // replace
      std::string replace = search_replace_entry.get_text();
      buffer->begin_user_action();
      buffer->erase(startIter, endIter);
      buffer->insert_at_cursor(replace);
      buffer->end_user_action();
    }
    on_search();
  }
}

/**
 * \brief Triggers when pressed enter in the address bar
 */
void MainWindow::address_bar_activate()
{
  middleware_.do_request(address_bar.get_text(), false);
  // When user actually entered the address bar, focus on the primary draw
  draw_primary.grab_focus();
}

/**
 * \brief Triggers when user tries to search or replace text
 */
void MainWindow::show_search(bool replace)
{
  if (search_popover.is_visible() && search_replace_entry.is_visible())
  {
    if (replace)
    {
      search_popover.hide();
      address_bar.grab_focus();
      search_replace_entry.hide();
    }
    else
    {
      search_entry.grab_focus();
      search_replace_entry.hide();
    }
  }
  else if (search_popover.is_visible())
  {
    if (replace)
    {
      search_replace_entry.show();
    }
    else
    {
      search_popover.hide();
      address_bar.grab_focus();
      search_replace_entry.hide();
    }
  }
  else
  {
    search_popover.show();
    search_entry.grab_focus();
    if (replace)
    {
      search_replace_entry.show();
    }
    else
    {
      search_replace_entry.hide();
    }
  }
}

void MainWindow::back()
{
  if (current_history_index_ > 0)
  {
    current_history_index_--;
    middleware_.do_request(history_.at(current_history_index_), true, true);
  }
}

void MainWindow::forward()
{
  if (current_history_index_ < history_.size() - 1)
  {
    current_history_index_++;
    middleware_.do_request(history_.at(current_history_index_), true, true);
  }
}

/**
 * \brief Fill-in table of contents and show
 */
void MainWindow::set_table_of_contents(std::vector<Glib::RefPtr<Gtk::TextMark>> headings)
{
  Gtk::TreeRow heading1Row, heading2Row, heading3Row, heading4Row, heading5Row;
  int previousLevel = 1; // Default heading 1
  for (const Glib::RefPtr<Gtk::TextMark>& headerMark : headings)
  {
    Glib::ustring heading = static_cast<char*>(headerMark->get_data("name"));
    auto level = reinterpret_cast<std::intptr_t>(headerMark->get_data("level"));
    switch (level)
    {
    case 1:
    {
      heading1Row = *(toc_tree_model->append());
      heading1Row[toc_columns.col_iter] = headerMark->get_iter();
      heading1Row[toc_columns.col_level] = level;
      heading1Row[toc_columns.col_heading] = heading;
      heading1Row[toc_columns.col_valid] = true;
      // Reset
      if (previousLevel > 1)
      {
        heading2Row = Gtk::TreeRow();
        heading3Row = Gtk::TreeRow();
        heading4Row = Gtk::TreeRow();
        heading5Row = Gtk::TreeRow();
      }
      break;
    }
    case 2:
    {
      if (heading1Row->get_model_gobject() == nullptr)
      {
        // Add missing heading as top-level
        heading1Row = *(toc_tree_model->append());
        heading1Row[toc_columns.col_level] = 1;
        heading1Row[toc_columns.col_heading] = "-Missing heading-";
        heading1Row[toc_columns.col_valid] = false;
      }
      heading2Row = *(toc_tree_model->append(heading1Row.children()));
      heading2Row[toc_columns.col_iter] = headerMark->get_iter();
      heading2Row[toc_columns.col_level] = level;
      heading2Row[toc_columns.col_heading] = heading;
      heading2Row[toc_columns.col_valid] = true;
      // Reset
      if (previousLevel > 2)
      {
        heading3Row = Gtk::TreeRow();
        heading4Row = Gtk::TreeRow();
        heading5Row = Gtk::TreeRow();
      }
      break;
    }
    case 3:
    {
      if (heading2Row->get_model_gobject() == nullptr)
      {
        // Add missing heading as top-level
        heading2Row = *(toc_tree_model->append(heading1Row.children()));
        heading2Row[toc_columns.col_level] = 2;
        heading2Row[toc_columns.col_heading] = "-Missing heading-";
        heading2Row[toc_columns.col_valid] = false;
      }
      heading3Row = *(toc_tree_model->append(heading2Row.children()));
      heading3Row[toc_columns.col_iter] = headerMark->get_iter();
      heading3Row[toc_columns.col_level] = level;
      heading3Row[toc_columns.col_heading] = heading;
      heading3Row[toc_columns.col_valid] = true;
      // Reset
      if (previousLevel > 3)
      {
        heading4Row = Gtk::TreeRow();
        heading5Row = Gtk::TreeRow();
      }
      break;
    }
    case 4:
    {
      if (heading3Row->get_model_gobject() == nullptr)
      {
        // Add missing heading as top-level
        heading3Row = *(toc_tree_model->append(heading2Row.children()));
        heading3Row[toc_columns.col_level] = 3;
        heading3Row[toc_columns.col_heading] = "-Missing heading-";
        heading3Row[toc_columns.col_valid] = false;
      }
      heading4Row = *(toc_tree_model->append(heading3Row.children()));
      heading4Row[toc_columns.col_iter] = headerMark->get_iter();
      heading4Row[toc_columns.col_level] = level;
      heading4Row[toc_columns.col_heading] = heading;
      heading4Row[toc_columns.col_valid] = true;
      // Reset
      if (previousLevel > 4)
      {
        heading5Row = Gtk::TreeRow();
      }
      break;
    }
    case 5:
    {
      if (heading4Row->get_model_gobject() == nullptr)
      {
        // Add missing heading as top-level
        heading4Row = *(toc_tree_model->append(heading3Row.children()));
        heading4Row[toc_columns.col_level] = 4;
        heading4Row[toc_columns.col_heading] = "-Missing heading-";
        heading4Row[toc_columns.col_valid] = false;
      }
      heading5Row = *(toc_tree_model->append(heading4Row.children()));
      heading5Row[toc_columns.col_iter] = headerMark->get_iter();
      heading5Row[toc_columns.col_level] = level;
      heading5Row[toc_columns.col_heading] = heading;
      heading5Row[toc_columns.col_valid] = true;
      break;
    }
    case 6:
    {
      if (heading5Row->get_model_gobject() == nullptr)
      {
        // Add missing heading as top-level
        heading5Row = *(toc_tree_model->append(heading4Row.children()));
        heading5Row[toc_columns.col_level] = 5;
        heading5Row[toc_columns.col_heading] = "- Missing heading -";
        heading5Row[toc_columns.col_valid] = false;
      }
      auto heading6Row = *(toc_tree_model->append(heading5Row.children()));
      heading6Row[toc_columns.col_iter] = headerMark->get_iter();
      heading6Row[toc_columns.col_level] = level;
      heading6Row[toc_columns.col_heading] = heading;
      heading6Row[toc_columns.col_valid] = true;
      break;
    }
    default:
      std::cerr << "ERROR: Out of range heading level detected." << std::endl;
      break;
    }
    previousLevel = level;
  }
  toc_tree_view.columns_autosize();
  toc_tree_view.expand_all();
}

/**
 * \brief Determing if browser is installed to the installation directory at runtime
 * \return true if the current running process is installed (to the installed prefix path)
 */
bool MainWindow::is_installed()
{
  char* path = NULL;
  int length;
  length = wai_getExecutablePath(NULL, 0, NULL);
  if (length > 0)
  {
    path = static_cast<char*>(malloc(length + 1));
    if (!path)
    {
      std::cerr << "ERROR: Couldn't create executable path." << std::endl;
    }
    else
    {
      bool is_browser_installed = true;
      wai_getExecutablePath(path, length, NULL);
      path[length] = '\0';
#if defined(_WIN32)
      // Does the executable path starts with "C:\Program"?
      const char* windowsPrefix = "C:\\Program";
      is_browser_installed = (strncmp(path, windowsPrefix, strlen(windowsPrefix)) == 0);
#elif defined(_APPLE_)
      // Does the executable path contains "Applications"?
      const char* macOsNeedle = "Applications";
      is_browser_installed = (strstr(path, macOsNeedle) != NULL);
#elif defined(__linux__)
      // Does the executable path starts with "/usr/local"?
      is_browser_installed = (strncmp(path, INSTALL_PREFIX, strlen(INSTALL_PREFIX)) == 0);
#endif
      free(path);
      return is_browser_installed;
    }
  }
  return true; // fallback; assume always installed
}

/**
 * \brief Enable editor mode. Allowing to create or edit existing documents
 */
void MainWindow::enable_edit()
{
  // Inform the Draw class that we are creating a new document,
  // will apply change some textview setting changes
  draw_primary.new_document();
  // Show editor toolbars
  hbox_standard_editor_toolbar.show();
  hbox_formatting_editor_toolbar.show();
  // Enable monospace in editor
  draw_primary.set_monospace(true);
  // Apply some settings from primary to secondary window
  draw_secondary.set_indent(indent_);
  draw_secondary.set_wrap_mode(wrap_mode_);
  draw_secondary.set_left_margin(content_margin_);
  draw_secondary.set_right_margin(content_margin_);
  // Determine position of divider between the primary and secondary windows
  int currentWidth = get_width();
  int maxWidth = currentWidth - 40;
  // Recalculate the position divider if it's too big,
  // or position_divider_draw_ is still on default value
  if ((paned_draw.get_position() >= maxWidth) || position_divider_draw_ == -1)
  {
    int proposedPosition = position_divider_draw_; // Try to first use the gsettings
    if ((proposedPosition == -1) || (proposedPosition >= maxWidth))
    {
      proposedPosition = static_cast<int>(currentWidth / 2.0);
    }
    paned_draw.set_position(proposedPosition);
  }
  // Enabled secondary text view (on the right)
  scrolled_window_secondary.show();
  // Disable "view source" menu item
  draw_primary.set_view_source_menu_item(false);
  // Connect changed signal
  text_changed_signal_handler_ = draw_primary.get_buffer()->signal_changed().connect(sigc::mem_fun(this, &MainWindow::editor_changed_text));
  // Enable publish menu item
  menu.set_publish_menu_sensitive(true);
  // Disable edit menu item (you are already editing)
  menu.set_edit_menu_sensitive(false);
  // Just to be sure, disable the spinning animation
  refresh_icon.get_style_context()->remove_class("spinning");
}

/**
 * \brief Disable editor mode
 */
void MainWindow::disable_edit()
{
  if (is_editor_enabled())
  {
    hbox_standard_editor_toolbar.hide();
    hbox_formatting_editor_toolbar.hide();
    scrolled_window_secondary.hide();
    // Disconnect text changed signal
    text_changed_signal_handler_.disconnect();
    // Disable monospace
    draw_primary.set_monospace(false);
    // Re-apply settings on primary window
    draw_primary.set_indent(indent_);
    draw_primary.set_wrap_mode(wrap_mode_);
    // Show "view source" menu item again
    draw_primary.set_view_source_menu_item(true);
    draw_secondary.clear();
    // Disable publish menu item
    menu.set_publish_menu_sensitive(false);
    // Enable edit menu item
    menu.set_edit_menu_sensitive(true);
    // Empty current file saved path
    current_file_saved_path_ = "";
  }
}

/**
 * \brief Check if editor is enabled
 * \return true if enabled, otherwise false
 */
bool MainWindow::is_editor_enabled()
{
  return hbox_standard_editor_toolbar.is_visible();
}

/**
 * \brief Retrieve image path from icon theme location
 * \param icon_name Icon name (.png is added default)
 * \param typeof_icon Type of the icon is the sub-folder within the icons directory (eg. "editor", "arrows" or "basic")
 * \return full path of the icon PNG image
 */
std::string MainWindow::get_icon_image_from_theme(const std::string& icon_name, const std::string& typeof_icon)
{
  // Use data directory first, used when LibreWeb is installed (Linux or Windows)
  for (std::string data_dir : Glib::get_system_data_dirs())
  {
    std::vector<std::string> path_builder{data_dir, "libreweb", "images", "icons", current_icon_theme_, typeof_icon, icon_name + ".png"};
    std::string file_path = Glib::build_path(G_DIR_SEPARATOR_S, path_builder);
    if (Glib::file_test(file_path, Glib::FileTest::FILE_TEST_IS_REGULAR))
    {
      return file_path;
    }
  }

  // Try local path if the images are not (yet) installed
  // When working directory is in the build/bin folder (relative path)
  std::vector<std::string> path_builder{"..", "..", "images", "icons", current_icon_theme_, typeof_icon, icon_name + ".png"};
  std::string file_path = Glib::build_path(G_DIR_SEPARATOR_S, path_builder);
  if (Glib::file_test(file_path, Glib::FileTest::FILE_TEST_IS_REGULAR))
  {
    return file_path;
  }
  else
  {
    return "";
  }
}

/**
 * \brief Calculate & update margins on primary draw
 */
void MainWindow::update_margins()
{
  if (is_editor_enabled())
  {
    draw_secondary.set_left_margin(content_margin_);
    draw_secondary.set_right_margin(content_margin_);
  }
  else
  {
    if (is_reader_view_enabled_)
    {
      int width = draw_primary.get_width();
      if (width > (content_max_width_ + (2 * content_margin_)))
      {
        // Calculate margins on the fly
        int margin = (width - content_max_width_) / 2;
        draw_primary.set_left_margin(margin);
        draw_primary.set_right_margin(margin);
      }
      else
      {
        draw_primary.set_left_margin(content_margin_);
        draw_primary.set_right_margin(content_margin_);
      }
    }
    else
    {
      draw_primary.set_left_margin(content_margin_);
      draw_primary.set_right_margin(content_margin_);
    }
  }
}

/**
 * \brief Update the CSS provider data
 */
void MainWindow::update_css()
{
  std::string colorCss;
  double darknessScale = (1.0 - brightness_scale_);
  std::ostringstream darknessDoubleStream;
  darknessDoubleStream << darknessScale;
  std::string darknessStr = darknessDoubleStream.str();

  // If it's getting to dark, let's change the font color to white
  if (darknessScale >= 0.7)
  {
    double colorDouble = ((((1.0 - darknessScale) - 0.5) * (20.0 - 255.0)) / (1.0 - 0.5)) + 255.0;
    std::ostringstream colorStream;
    colorStream << colorDouble;
    std::string colorStr = colorStream.str();
    colorCss = "color: rgba(" + colorStr + ", " + colorStr + ", " + colorStr + ", " + darknessStr + ");";
  }

  std::stringstream streamFontSpacing;
  streamFontSpacing << std::fixed << std::setprecision(1) << font_spacing_;
  std::string letterSpacing = streamFontSpacing.str();

  try
  {
    draw_css_provider->load_from_data("textview { "
                                      "font-family: \"" +
                                      font_family_ +
                                      "\";"
                                      "font-size: " +
                                      std::to_string(current_font_size_) + "pt; }" + "textview text { " + colorCss +
                                      "background-color: rgba(0, 0, 0," + darknessStr +
                                      ");"
                                      "letter-spacing: " +
                                      letterSpacing + "px; }");
  }
  catch (const Gtk::CssProviderError& error)
  {
    std::cerr << "ERROR: Could not apply CSS format, error: " << error.what() << std::endl;
  }
}

/**
 * \brief Show Gio notification
 * \param title Title of the notification
 * \param message The message displayed along with the notification
 */
void MainWindow::show_notification(const Glib::ustring& title, const Glib::ustring& message)
{
  // TODO: Report GLib-CRITICAL upstream to GTK (this is not my issue)
  auto notification = Gio::Notification::create(title);
  auto icon = Gio::ThemedIcon::create("dialog-information");
  notification->set_body(message);
  notification->set_icon(icon);
  get_application()->send_notification(notification);
}

void MainWindow::editor_changed_text()
{
  // TODO: Just execute the code below in a signal_idle call?
  // So it will never block the GUI thread. Or is this already running in another context

  // Clear table of contents (ToC)
  toc_tree_model->clear();
  // Retrieve text from editor and parse the markdown contents
  middleware_.set_content(draw_primary.get_text());
  cmark_node* doc = middleware_.parse_content();
  /* // Can be enabled to show the markdown format in terminal:
  std::string md = Parser::render_markdown(doc);
  std::cout << "Markdown:\n" << md << std::endl;*/
  // Show the document as a preview on the right side text-view panel
  draw_secondary.set_document(doc);
  set_table_of_contents(draw_secondary.get_headings());
}

/**
 * \brief Show source code dialog window with the current content
 */
void MainWindow::show_source_code_dialog()
{
  source_code_dialog.set_text(middleware_.get_content());
  source_code_dialog.run();
}

/**
 * \brief Retrieve selected heading from combobox.
 * Send to main Draw class
 */
void MainWindow::get_heading()
{
  std::string active = headings_combo_box.get_active_id();
  headings_combo_box.set_active(0); // Reset
  if (active != "")
  {
    std::string::size_type sz;
    try
    {
      int headingLevel = std::stoi(active, &sz, 10);
      draw_primary.make_heading(headingLevel);
    }
    catch (const std::invalid_argument&)
    {
      // ignore
      std::cerr << "ERROR: heading combobox id is invalid (not a number)." << std::endl;
    }
    catch (const std::out_of_range&)
    {
      // ignore
    }
  }
}

void MainWindow::insert_emoji()
{
  // Note: The "insert-emoji" signal is not exposed in Gtkmm library (at least not in gtk3)
  g_signal_emit_by_name(draw_primary.gobj(), "insert-emoji");
}

void MainWindow::on_zoom_out()
{
  current_font_size_ -= 1;
  update_css();
  zoom_restore_button.set_sensitive(current_font_size_ != default_font_size_);
}

void MainWindow::on_zoom_restore()
{
  current_font_size_ = default_font_size_; // reset
  update_css();
  zoom_restore_button.set_sensitive(false);
}

void MainWindow::on_zoom_in()
{
  current_font_size_ += 1;
  update_css();
  zoom_restore_button.set_sensitive(current_font_size_ != default_font_size_);
}

void MainWindow::on_font_set()
{
  Pango::FontDescription fontDesc = Pango::FontDescription(font_button.get_font_name());
  font_family_ = fontDesc.get_family();
  current_font_size_ = default_font_size_ = (fontDesc.get_size_is_absolute()) ? fontDesc.get_size() : fontDesc.get_size() / PANGO_SCALE;
  update_css();
}

void MainWindow::on_max_content_width_changed()
{
  content_max_width_ = max_content_width_spin_button.get_value_as_int();
  if (!is_editor_enabled())
    update_margins();
}

void MainWindow::on_spacing_changed()
{
  font_spacing_ = spacing_spin_button.get_value(); // Letter-spacing
  update_css();
}

void MainWindow::on_margins_changed()
{
  content_margin_ = margins_spin_button.get_value_as_int();
  update_margins();
}

void MainWindow::on_indent_changed()
{
  indent_ = indent_spin_button.get_value_as_int();
  if (is_editor_enabled())
    draw_secondary.set_indent(indent_);
  else
    draw_primary.set_indent(indent_);
}

void MainWindow::on_wrap_toggled(Gtk::WrapMode mode)
{
  wrap_mode_ = mode;
  if (is_editor_enabled())
    draw_secondary.set_wrap_mode(wrap_mode_);
  else
    draw_primary.set_wrap_mode(wrap_mode_);
}

void MainWindow::on_brightness_changed()
{
  brightness_scale_ = scale_settings_brightness.get_value();
  update_css();
}

void MainWindow::on_theme_changed()
{
  // Switch between dark or light theme preference
  use_dark_theme_ = theme_switch.get_active();
  set_theme();
}

void MainWindow::on_reader_view_changed()
{
  is_reader_view_enabled_ = reader_view_switch.get_active();
  if (!is_editor_enabled())
    update_margins();
}

void MainWindow::on_icon_theme_activated(Gtk::ListBoxRow* row)
{
  std::string themeName = static_cast<char*>(row->get_data("value"));
  if (themeName != "none")
  {
    current_icon_theme_ = themeName;
    use_current_gtk_icon_theme_ = false;
  }
  else
  {
    use_current_gtk_icon_theme_ = true;
  }
  // Reload icons
  load_icons();
  // Trigger IPFS status icon
  update_status_popover_and_icon();
}
