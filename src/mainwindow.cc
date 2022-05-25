#include "mainwindow.h"

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
      vboxMain(Gtk::ORIENTATION_VERTICAL, 0),
      vboxToc(Gtk::ORIENTATION_VERTICAL),
      vboxSearch(Gtk::ORIENTATION_VERTICAL),
      vboxStatus(Gtk::ORIENTATION_VERTICAL),
      vboxSettings(Gtk::ORIENTATION_VERTICAL),
      vboxIconTheme(Gtk::ORIENTATION_VERTICAL),
      searchMatchCase("Match _Case", true),
      wrapNone(wrappingGroup, "None"),
      wrapChar(wrappingGroup, "Char"),
      wrapWord(wrappingGroup, "Word"),
      wrapWordChar(wrappingGroup, "Word+Char"),
      closeTocWindowButton("Close table of contents"),
      openTocButton("Show table of contents (Ctrl+Shift+T)", true),
      backButton("Go back one page (Alt+Left arrow)", true),
      forwardButton("Go forward one page (Alt+Right arrow)", true),
      refreshButton("Reload current page (Ctrl+R)", true),
      homeButton("Home page (Alt+Home)", true),
      openButton("Open document (Ctrl+O)"),
      saveButton("Save document (Ctrl+S)"),
      publishButton("Publish document... (Ctrl+P)"),
      cutButton("Cut (Ctrl+X)"),
      copyButton("Copy (Ctrl+C)"),
      pasteButton("Paste (Ctrl+V)"),
      undoButton("Undo text (Ctrl+Z)"),
      redoButton("Redo text (Ctrl+Y)"),
      boldButton("Add bold text"),
      italicButton("Add italic text"),
      strikethroughButton("Add strikethrough text"),
      superButton("Add superscript text"),
      subButton("Add subscript text"),
      linkButton("Add a link"),
      imageButton("Add an image"),
      emojiButton("Insert emoji"),
      quoteButton("Insert a quote"),
      codeButton("Insert code"),
      bulletListButton("Add a bullet list"),
      numberedListButton("Add a numbered list"),
      highlightButton("Add highlight text"),
      tableOfContentsLabel("Table of Contents"),
      networkHeadingLabel("IPFS Network"),
      networkRateHeadingLabel("Network rate"),
      connectivityLabel("Status:"),
      peersLabel("Connected peers:"),
      repoSizeLabel("Repo size:"),
      repoPathLabel("Repo path:"),
      ipfsVersionLabel("IPFS version:"),
      networkIncomingLabel("Incoming"),
      networkOutcomingLabel("Outcoming"),
      networkKiloBytesLabel("Kilobytes/s"),
      fontLabel("Font"),
      maxContentWidthLabel("Content width"),
      spacingLabel("Spacing"),
      marginsLabel("Margins"),
      indentLabel("Indent"),
      textWrappingLabel("Wrapping"),
      themeLabel("Dark Theme"),
      readerViewLabel("Reader View"),
      iconThemeLabel("Active Theme"),
      // Private members
      middleware_(*this, timeout),
      app_name_("LibreWeb Browser"),
      use_current_gtk_icon_theme_(false), // Use LibreWeb icon theme or the GTK icons
      icon_theme_("flat"),                // Default is flat built-in theme
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
  scrolledWindowPrimary.add(draw_primary);
  scrolledWindowPrimary.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  // Secondary drawing area
  draw_secondary.set_view_source_menu_item(false);
  scrolledWindowSecondary.add(draw_secondary);
  scrolledWindowSecondary.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  panedDraw.pack1(scrolledWindowPrimary, true, false);
  panedDraw.pack2(scrolledWindowSecondary, true, true);
  // Left the vbox for the table of contents,
  // right the drawing paned windows (primary/secondary).
  panedRoot.pack1(vboxToc, true, false);
  panedRoot.pack2(panedDraw, true, false);
  // Main virtual box
  vboxMain.pack_start(menu, false, false, 0);
  vboxMain.pack_start(hboxBrowserToolbar, false, false, 6);
  vboxMain.pack_start(hboxStandardEditorToolbar, false, false, 6);
  vboxMain.pack_start(hboxFormattingEditorToolbar, false, false, 6);
  vboxMain.pack_start(panedRoot, true, true, 0);
  add(vboxMain);
  show_all_children();

  // Hide by default the table of contents, secondary textview, replace entry and editor toolbars
  vboxToc.hide();
  scrolledWindowSecondary.hide();
  searchReplaceEntry.hide();
  hboxStandardEditorToolbar.hide();
  hboxFormattingEditorToolbar.hide();

  // Grap focus to input field by default
  addressBar.grab_focus();

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
    addressBar.set_text(path);
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
  backButton.set_sensitive(current_history_index_ > 0);
  menu.set_back_menu_sensitive(current_history_index_ > 0);
  forwardButton.set_sensitive(current_history_index_ < history_.size() - 1);
  menu.set_forward_menu_sensitive(current_history_index_ < history_.size() - 1);

  // Clear table of contents (ToC)
  tocTreeModel->clear();
}

/**
 * \brief Called after file is written to disk.
 */
void MainWindow::post_write(const std::string& path, const std::string& title, bool is_set_address_and_title)
{
  if (is_set_address_and_title)
  {
    addressBar.set_text(path);
    set_title(title + " - " + app_name_);
  }
}

/**
 * \brief Called when request started (from thread).
 */
void MainWindow::started_request()
{
  // Start spinning icon
  refreshIcon.get_style_context()->add_class("spinning");
}

/**
 * \brief Called when request is finished (from thread).
 */
void MainWindow::finished_request()
{
  // Stop spinning icon
  refreshIcon.get_style_context()->remove_class("spinning");
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
      statusIcon.set_from_icon_name("network-wired-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    }
    else
    {
      statusIcon.set(statusOnlineIcon);
    }
  }
  else
  {
    networkStatus = "Disconnected";
    if (use_current_gtk_icon_theme_)
    {
      statusIcon.set_from_icon_name("network-wired-disconnected-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    }
    else
    {
      statusIcon.set(statusOfflineIcon);
    }
  }
  connectivityStatusLabel.set_markup("<b>" + networkStatus + "</b>");
  peersStatusLabel.set_text(std::to_string(nrOfPeers));
  repoSizeStatusLabel.set_text(std::to_string(middleware_.get_ipfs_repo_size()) + " MB");
  repoPathStatusLabel.set_text(middleware_.get_ipfs_repo_path());
  networkIncomingStatusLabel.set_text(middleware_.get_ipfs_incoming_rate());
  networkOutcomingStatusLabel.set_text(middleware_.get_ipfs_outgoing_rate());
  ipfsVersionStatusLabel.set_text(middleware_.get_ipfs_version());
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
    std::vector<std::string> relativePath{".."};
    std::string schemaDir = Glib::build_path(G_DIR_SEPARATOR_S, relativePath);
    std::cout << "INFO: Binary not installed. Try to find the gschema file one directory up (..)." << std::endl;
    Glib::setenv("GSETTINGS_SCHEMA_DIR", schemaDir);
  }

  // Load schema settings file
  auto schemaSource = Gio::SettingsSchemaSource::get_default()->lookup("org.libreweb.browser", true);
  if (schemaSource)
  {
    settings = Gio::Settings::create("org.libreweb.browser");
    // Apply global settings
    set_default_size(settings->get_int("width"), settings->get_int("height"));
    if (settings->get_boolean("maximized"))
      maximize();
    position_divider_draw_ = settings->get_int("position-divider-draw");
    panedDraw.set_position(position_divider_draw_);
    font_family_ = settings->get_string("font-family");
    current_font_size_ = default_font_size_ = settings->get_int("font-size");
    fontButton.set_font_name(font_family_ + " " + std::to_string(current_font_size_));

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
    panedRoot.set_position(tocDividerPosition);
    icon_theme_ = settings->get_string("icon-theme");
    use_current_gtk_icon_theme_ = settings->get_boolean("icon-gtk-theme");
    brightness_scale_ = settings->get_double("brightness");
    use_dark_theme_ = settings->get_boolean("dark-theme");
    is_reader_view_enabled_ = settings->get_boolean("reader-view");
    switch (wrap_mode_)
    {
    case Gtk::WRAP_NONE:
      wrapNone.set_active(true);
      break;
    case Gtk::WRAP_CHAR:
      wrapChar.set_active(true);
      break;
    case Gtk::WRAP_WORD:
      wrapWord.set_active(true);
      break;
    case Gtk::WRAP_WORD_CHAR:
      wrapWordChar.set_active(true);
      break;
    default:
      wrapWordChar.set_active(true);
    }
  }
  else
  {
    std::cerr << "ERROR: Gsettings schema file could not be found!" << std::endl;
    // Select default fallback wrap mode
    wrapWordChar.set_active(true);
    // Fallback adjustment controls
    max_content_width_adjustment->set_value(content_max_width_);
    spacing_adjustment->set_value(font_spacing_);
    margins_adjustment->set_value(content_max_width_);
    indent_adjustment->set_value(indent_);
    // Fallback ToC paned divider
    panedRoot.set_position(300);
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
  tocIcon.set_from_icon_name("view-list-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  backIcon.set_from_icon_name("go-previous", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  forwardIcon.set_from_icon_name("go-next", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  refreshIcon.set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  homeIcon.set_from_icon_name("go-home", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  searchIcon.set_from_icon_name("edit-find-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  settingsIcon.set_from_icon_name("open-menu-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  // Settings pop-over buttons
  zoomOutImage.set_from_icon_name("zoom-out-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  zoomInImage.set_from_icon_name("zoom-in-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  brightnessImage.set_from_icon_name("display-brightness-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
}

/**
 * Load all icon images from theme/disk. Or reload them.
 */
void MainWindow::load_icons()
{
  try
  {
    // Editor buttons
    openIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("open_folder", "folders"), icon_size_, icon_size_));
    saveIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("floppy_disk", "basic"), icon_size_, icon_size_));
    publishIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("upload", "basic"), icon_size_, icon_size_));
    cutIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("cut", "editor"), icon_size_, icon_size_));
    copyIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("copy", "editor"), icon_size_, icon_size_));
    pasteIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("clipboard", "editor"), icon_size_, icon_size_));
    undoIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("undo", "editor"), icon_size_, icon_size_));
    redoIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("redo", "editor"), icon_size_, icon_size_));
    boldIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("bold", "editor"), icon_size_, icon_size_));
    italicIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("italic", "editor"), icon_size_, icon_size_));
    strikethroughIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("strikethrough", "editor"), icon_size_, icon_size_));
    superIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("superscript", "editor"), icon_size_, icon_size_));
    subIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("subscript", "editor"), icon_size_, icon_size_));
    linkIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("link", "editor"), icon_size_, icon_size_));
    imageIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("shapes", "editor"), icon_size_, icon_size_));
    emojiIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("smile", "smiley"), icon_size_, icon_size_));
    quoteIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("quote", "editor"), icon_size_, icon_size_));
    codeIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("code", "editor"), icon_size_, icon_size_));
    bulletListIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("bullet_list", "editor"), icon_size_, icon_size_));
    numberedListIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("number_list", "editor"), icon_size_, icon_size_));
    hightlightIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("highlighter", "editor"), icon_size_, icon_size_));

    if (use_current_gtk_icon_theme_)
    {
      set_gtk_icons();
    }
    else
    {
      // Toolbox buttons
      tocIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("square_list", "editor"), icon_size_, icon_size_));
      backIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("right_arrow_1", "arrows"), icon_size_, icon_size_)->flip());
      forwardIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("right_arrow_1", "arrows"), icon_size_, icon_size_));
      refreshIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("reload_centered", "arrows"), icon_size_ * 1.13, icon_size_));
      homeIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("home", "basic"), icon_size_, icon_size_));
      searchIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("search", "basic"), icon_size_, icon_size_));
      settingsIcon.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("menu", "basic"), icon_size_, icon_size_));

      // Settings pop-over buttons
      zoomOutImage.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("zoom_out", "basic"), icon_size_, icon_size_));
      zoomInImage.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("zoom_in", "basic"), icon_size_, icon_size_));
      brightnessImage.set(Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("brightness", "basic"), icon_size_, icon_size_));
      statusOfflineIcon = Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("network_disconnected", "network"), icon_size_, icon_size_);
      statusOnlineIcon = Gdk::Pixbuf::create_from_file(get_icon_image_from_theme("network_connected", "network"), icon_size_, icon_size_);
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
  openButton.add(openIcon);
  saveButton.add(saveIcon);
  publishButton.add(publishIcon);
  cutButton.add(cutIcon);
  copyButton.add(copyIcon);
  pasteButton.add(pasteIcon);
  undoButton.add(undoIcon);
  redoButton.add(redoIcon);
  boldButton.add(boldIcon);
  italicButton.add(italicIcon);
  strikethroughButton.add(strikethroughIcon);
  superButton.add(superIcon);
  subButton.add(subIcon);
  linkButton.add(linkIcon);
  imageButton.add(imageIcon);
  emojiButton.add(emojiIcon);
  quoteButton.add(quoteIcon);
  codeButton.add(codeIcon);
  bulletListButton.add(bulletListIcon);
  numberedListButton.add(numberedListIcon);
  highlightButton.add(hightlightIcon);

  // Disable focus the other buttons as well
  searchMatchCase.set_can_focus(false);
  headingsComboBox.set_can_focus(false);
  headingsComboBox.set_focus_on_click(false);

  // Populate the heading comboboxtext
  headingsComboBox.append("", "Select Heading");
  headingsComboBox.append("1", "Heading 1");
  headingsComboBox.append("2", "Heading 2");
  headingsComboBox.append("3", "Heading 3");
  headingsComboBox.append("4", "Heading 4");
  headingsComboBox.append("5", "Heading 5");
  headingsComboBox.append("6", "Heading 6");
  headingsComboBox.set_active(0);

  // Horizontal bar
  backButton.get_style_context()->add_class("circular");
  forwardButton.get_style_context()->add_class("circular");
  refreshButton.get_style_context()->add_class("circular");
  searchButton.set_popover(searchPopover);
  statusButton.set_popover(statusPopover);
  settingsButton.set_popover(settingsPopover);
  searchButton.set_relief(Gtk::RELIEF_NONE);
  statusButton.set_relief(Gtk::RELIEF_NONE);
  settingsButton.set_relief(Gtk::RELIEF_NONE);

  // Add icons to the toolbar buttons
  openTocButton.add(tocIcon);
  backButton.add(backIcon);
  forwardButton.add(forwardIcon);
  refreshButton.add(refreshIcon);
  homeButton.add(homeIcon);
  searchButton.add(searchIcon);
  statusButton.add(statusIcon);
  settingsButton.add(settingsIcon);

  // Add spinning CSS class to refresh icon
  auto cssProvider = Gtk::CssProvider::create();
  auto screen = Gdk::Screen::get_default();
  std::string spinningCSS = "@keyframes spin {  to { -gtk-icon-transform: rotate(1turn); }} .spinning { animation-name: spin;  "
                            "animation-duration: 1s;  animation-timing-function: linear;  animation-iteration-count: infinite;}";
  if (!cssProvider->load_from_data(spinningCSS))
  {
    std::cerr << "ERROR: CSS data parsing went wrong." << std::endl;
  }
  auto refreshIconStyle = refreshIcon.get_style_context();
  refreshIconStyle->add_provider_for_screen(screen, cssProvider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  // Add tooltips to the toolbar buttons
  searchButton.set_tooltip_text("Find");
  statusButton.set_tooltip_text("IPFS Network Status");
  settingsButton.set_tooltip_text("Settings");
  // Disable back/forward buttons on start-up
  backButton.set_sensitive(false);
  forwardButton.set_sensitive(false);

  /*
   * Adding the buttons to the boxes
   */
  // Browser Toolbar
  openTocButton.set_margin_left(6);
  hboxBrowserToolbar.pack_start(openTocButton, false, false, 0);
  hboxBrowserToolbar.pack_start(backButton, false, false, 0);
  hboxBrowserToolbar.pack_start(forwardButton, false, false, 0);
  hboxBrowserToolbar.pack_start(refreshButton, false, false, 0);
  hboxBrowserToolbar.pack_start(homeButton, false, false, 0);
  hboxBrowserToolbar.pack_start(addressBar, true, true, 4);
  hboxBrowserToolbar.pack_start(searchButton, false, false, 0);
  hboxBrowserToolbar.pack_start(statusButton, false, false, 0);
  hboxBrowserToolbar.pack_start(settingsButton, false, false, 0);

  // Standard editor toolbar
  headingsComboBox.set_margin_left(4);
  hboxStandardEditorToolbar.pack_start(openButton, false, false, 2);
  hboxStandardEditorToolbar.pack_start(saveButton, false, false, 2);
  hboxStandardEditorToolbar.pack_start(publishButton, false, false, 2);
  hboxStandardEditorToolbar.pack_start(separator1, false, false, 0);
  hboxStandardEditorToolbar.pack_start(cutButton, false, false, 2);
  hboxStandardEditorToolbar.pack_start(copyButton, false, false, 2);
  hboxStandardEditorToolbar.pack_start(pasteButton, false, false, 2);
  hboxStandardEditorToolbar.pack_start(separator2, false, false, 0);
  hboxStandardEditorToolbar.pack_start(undoButton, false, false, 2);
  hboxStandardEditorToolbar.pack_start(redoButton, false, false, 2);

  // Formatting toolbar
  headingsComboBox.set_margin_left(4);
  hboxFormattingEditorToolbar.pack_start(headingsComboBox, false, false, 2);
  hboxFormattingEditorToolbar.pack_start(boldButton, false, false, 2);
  hboxFormattingEditorToolbar.pack_start(italicButton, false, false, 2);
  hboxFormattingEditorToolbar.pack_start(strikethroughButton, false, false, 2);
  hboxFormattingEditorToolbar.pack_start(superButton, false, false, 2);
  hboxFormattingEditorToolbar.pack_start(subButton, false, false, 2);
  hboxFormattingEditorToolbar.pack_start(separator3, false, false, 0);
  hboxFormattingEditorToolbar.pack_start(linkButton, false, false, 2);
  hboxFormattingEditorToolbar.pack_start(imageButton, false, false, 2);
  hboxFormattingEditorToolbar.pack_start(emojiButton, false, false, 2);
  hboxFormattingEditorToolbar.pack_start(separator4, false, false, 0);
  hboxFormattingEditorToolbar.pack_start(quoteButton, false, false, 2);
  hboxFormattingEditorToolbar.pack_start(codeButton, false, false, 2);
  hboxFormattingEditorToolbar.pack_start(bulletListButton, false, false, 2);
  hboxFormattingEditorToolbar.pack_start(numberedListButton, false, false, 2);
  hboxFormattingEditorToolbar.pack_start(highlightButton, false, false, 2);
}

/**
 * \brief Prefer dark or light theme
 */
void MainWindow::set_theme()
{
  auto settings = Gtk::Settings::get_default();
  settings->property_gtk_application_prefer_dark_theme().set_value(use_dark_theme_);
}

/**
 * \brief Popover search bar
 */
void MainWindow::init_search_popover()
{
  searchEntry.set_placeholder_text("Find");
  searchReplaceEntry.set_placeholder_text("Replace");
  search.connect_entry(searchEntry);
  searchReplace.connect_entry(searchReplaceEntry);
  searchEntry.set_size_request(250, -1);
  searchReplaceEntry.set_size_request(250, -1);
  vboxSearch.set_margin_left(8);
  vboxSearch.set_margin_right(8);
  vboxSearch.set_spacing(8);
  hboxSearch.set_spacing(8);

  hboxSearch.pack_start(searchEntry, false, false);
  hboxSearch.pack_start(searchMatchCase, false, false);
  vboxSearch.pack_start(hboxSearch, false, false, 4);
  vboxSearch.pack_end(searchReplaceEntry, false, false, 4);
  searchPopover.set_position(Gtk::POS_BOTTOM);
  searchPopover.set_size_request(300, 50);
  searchPopover.add(vboxSearch);
  searchPopover.show_all_children();
}

/**
 * Init the IPFS status pop-over
 */
void MainWindow::init_status_popover()
{
  connectivityLabel.set_xalign(0.0);
  peersLabel.set_xalign(0.0);
  repoSizeLabel.set_xalign(0.0);
  repoPathLabel.set_xalign(0.0);
  ipfsVersionLabel.set_xalign(0.0);
  connectivityStatusLabel.set_xalign(1.0);
  peersStatusLabel.set_xalign(1.0);
  repoSizeStatusLabel.set_xalign(1.0);
  repoPathStatusLabel.set_xalign(1.0);
  ipfsVersionStatusLabel.set_xalign(1.0);
  connectivityLabel.get_style_context()->add_class("dim-label");
  peersLabel.get_style_context()->add_class("dim-label");
  repoSizeLabel.get_style_context()->add_class("dim-label");
  repoPathLabel.get_style_context()->add_class("dim-label");
  ipfsVersionLabel.get_style_context()->add_class("dim-label");
  // Status popover grid
  statusGrid.set_column_homogeneous(true);
  statusGrid.set_margin_start(6);
  statusGrid.set_margin_top(6);
  statusGrid.set_margin_bottom(6);
  statusGrid.set_margin_end(12);
  statusGrid.set_row_spacing(10);
  statusGrid.set_column_spacing(6);
  statusGrid.attach(connectivityLabel, 0, 0);
  statusGrid.attach(connectivityStatusLabel, 1, 0);
  statusGrid.attach(peersLabel, 0, 1);
  statusGrid.attach(peersStatusLabel, 1, 1);
  statusGrid.attach(repoSizeLabel, 0, 2);
  statusGrid.attach(repoSizeStatusLabel, 1, 2);
  statusGrid.attach(repoPathLabel, 0, 3);
  statusGrid.attach(repoPathStatusLabel, 1, 3);
  statusGrid.attach(ipfsVersionLabel, 0, 4);
  statusGrid.attach(ipfsVersionStatusLabel, 1, 4);
  // IPFS Network activity status grid
  networkKiloBytesLabel.get_style_context()->add_class("dim-label");
  activityStatusGrid.set_column_homogeneous(true);
  activityStatusGrid.set_margin_start(6);
  activityStatusGrid.set_margin_top(6);
  activityStatusGrid.set_margin_bottom(6);
  activityStatusGrid.set_margin_end(6);
  activityStatusGrid.set_row_spacing(10);
  activityStatusGrid.set_column_spacing(6);
  activityStatusGrid.attach(networkIncomingLabel, 1, 0);
  activityStatusGrid.attach(networkOutcomingLabel, 2, 0);
  activityStatusGrid.attach(networkKiloBytesLabel, 0, 1);
  activityStatusGrid.attach(networkIncomingStatusLabel, 1, 1);
  activityStatusGrid.attach(networkOutcomingStatusLabel, 2, 1);

  networkHeadingLabel.get_style_context()->add_class("dim-label");
  networkRateHeadingLabel.get_style_context()->add_class("dim-label");
  // Copy ID & public key buttons
  copyIDButton.set_label("Copy your ID");
  copyPublicKeyButton.set_label("Copy Public Key");
  copyIDButton.set_margin_start(6);
  copyIDButton.set_margin_end(6);
  copyPublicKeyButton.set_margin_start(6);
  copyPublicKeyButton.set_margin_end(6);
  // Add all items to status box & status popover
  vboxStatus.set_margin_start(10);
  vboxStatus.set_margin_end(10);
  vboxStatus.set_margin_top(10);
  vboxStatus.set_margin_bottom(10);
  vboxStatus.set_spacing(6);
  vboxStatus.add(networkHeadingLabel);
  vboxStatus.add(statusGrid);
  vboxStatus.add(separator9);
  vboxStatus.add(networkRateHeadingLabel);
  vboxStatus.add(activityStatusGrid);
  vboxStatus.add(separator10);
  vboxStatus.add(copyPublicKeyButton);
  vboxStatus.add(copyIDButton);
  statusPopover.set_position(Gtk::POS_BOTTOM);
  statusPopover.set_size_request(100, 250);
  statusPopover.set_margin_end(2);
  statusPopover.add(vboxStatus);
  statusPopover.show_all_children();
  // Set fallback values for all status fields + status icon
  update_status_popover_and_icon();
}

/**
 * \brief Init table of contents window (left side-panel)
 */
void MainWindow::init_table_of_contents()
{
  closeTocWindowButton.set_image_from_icon_name("window-close-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
  tableOfContentsLabel.set_margin_start(6);
  hboxToc.pack_start(tableOfContentsLabel, false, false);
  hboxToc.pack_end(closeTocWindowButton, false, false);
  tocTreeView.append_column("Level", tocColumns.col_level);
  tocTreeView.append_column("Name", tocColumns.col_heading);
  tocTreeView.set_activate_on_single_click(true);
  tocTreeView.set_headers_visible(false);
  tocTreeView.set_tooltip_column(2);
  scrolledToc.add(tocTreeView);
  scrolledToc.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  tocTreeModel = Gtk::TreeStore::create(tocColumns);
  tocTreeView.set_model(tocTreeModel);
  vboxToc.pack_start(hboxToc, Gtk::PackOptions::PACK_SHRINK);
  vboxToc.pack_end(scrolledToc);
}

/**
 * \brief Init the settings pop-over
 */
void MainWindow::init_settings_popover()
{
  // Toolbar buttons / images
  zoomOutButton.add(zoomOutImage);
  zoomInButton.add(zoomInImage);
  brightnessImage.set_tooltip_text("Brightness");
  brightnessImage.set_margin_start(2);
  brightnessImage.set_margin_end(2);
  brightnessImage.set_margin_top(1);
  brightnessImage.set_margin_bottom(1);
  // Zoom buttons
  auto hboxZoomStyleContext = hboxSetingsZoom.get_style_context();
  hboxZoomStyleContext->add_class("linked");
  zoomRestoreButton.set_sensitive(false); // By default restore button disabled
  zoomRestoreButton.set_label("100%");
  zoomOutButton.set_tooltip_text("Zoom out");
  zoomRestoreButton.set_tooltip_text("Restore zoom");
  zoomInButton.set_tooltip_text("Zoom in");
  hboxSetingsZoom.set_size_request(-1, 40);
  hboxSetingsZoom.set_margin_bottom(6);
  hboxSetingsZoom.pack_start(zoomOutButton);
  hboxSetingsZoom.pack_start(zoomRestoreButton);
  hboxSetingsZoom.pack_end(zoomInButton);
  // Brightness slider
  brightness_adjustment->set_value(brightness_scale_); // Override with current loaded brightness setting
  scaleSettingsBrightness.set_adjustment(brightness_adjustment);
  scaleSettingsBrightness.add_mark(0.5, Gtk::PositionType::POS_BOTTOM, "");
  scaleSettingsBrightness.set_draw_value(false);
  scaleSettingsBrightness.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_brightness_changed));
  hboxSetingsBrightness.pack_start(brightnessImage, false, false);
  hboxSetingsBrightness.pack_end(scaleSettingsBrightness);
  // Settings labels / buttons
  fontLabel.set_tooltip_text("Font familiy");
  maxContentWidthLabel.set_tooltip_text("Max content width");
  spacingLabel.set_tooltip_text("Text spacing");
  marginsLabel.set_tooltip_text("Text margins");
  indentLabel.set_tooltip_text("Text indentation");
  textWrappingLabel.set_tooltip_text("Text wrapping");
  wrapNone.set_tooltip_text("No wrapping");
  wrapChar.set_tooltip_text("Character wrapping");
  wrapWord.set_tooltip_text("Word wrapping");
  wrapWordChar.set_tooltip_text("Word wrapping (+ character)");
  maxContentWidthSpinButton.set_adjustment(max_content_width_adjustment);
  spacingSpinButton.set_adjustment(spacing_adjustment);
  spacingSpinButton.set_digits(1);
  marginsSpinButton.set_adjustment(margins_adjustment);
  indentSpinButton.set_adjustment(indent_adjustment);
  fontLabel.set_xalign(1);
  maxContentWidthLabel.set_xalign(1);
  spacingLabel.set_xalign(1);
  marginsLabel.set_xalign(1);
  indentLabel.set_xalign(1);
  textWrappingLabel.set_xalign(1);
  themeLabel.set_xalign(1);
  readerViewLabel.set_xalign(1);
  fontLabel.get_style_context()->add_class("dim-label");
  maxContentWidthLabel.get_style_context()->add_class("dim-label");
  spacingLabel.get_style_context()->add_class("dim-label");
  marginsLabel.get_style_context()->add_class("dim-label");
  indentLabel.get_style_context()->add_class("dim-label");
  textWrappingLabel.get_style_context()->add_class("dim-label");
  themeLabel.get_style_context()->add_class("dim-label");
  readerViewLabel.get_style_context()->add_class("dim-label");
  // Dark theme switch
  themeSwitch.set_active(use_dark_theme_); // Override with current dark theme preference
  // Reader view switch
  readerViewSwitch.set_active(is_reader_view_enabled_);
  // Settings grid
  settingsGrid.set_margin_start(6);
  settingsGrid.set_margin_top(6);
  settingsGrid.set_margin_bottom(6);
  settingsGrid.set_row_spacing(10);
  settingsGrid.set_column_spacing(10);
  settingsGrid.attach(fontLabel, 0, 0, 1);
  settingsGrid.attach(fontButton, 1, 0, 2);
  settingsGrid.attach(maxContentWidthLabel, 0, 1, 1);
  settingsGrid.attach(maxContentWidthSpinButton, 1, 1, 2);
  settingsGrid.attach(spacingLabel, 0, 2, 1);
  settingsGrid.attach(spacingSpinButton, 1, 2, 2);
  settingsGrid.attach(marginsLabel, 0, 3, 1);
  settingsGrid.attach(marginsSpinButton, 1, 3, 2);
  settingsGrid.attach(indentLabel, 0, 4, 1);
  settingsGrid.attach(indentSpinButton, 1, 4, 2);
  settingsGrid.attach(textWrappingLabel, 0, 5, 1);
  settingsGrid.attach(wrapNone, 1, 5, 1);
  settingsGrid.attach(wrapChar, 2, 5, 1);
  settingsGrid.attach(wrapWord, 1, 6, 1);
  settingsGrid.attach(wrapWordChar, 2, 6, 1);
  settingsGrid.attach(themeLabel, 0, 7, 1);
  settingsGrid.attach(themeSwitch, 1, 7, 2);
  settingsGrid.attach(readerViewLabel, 0, 8, 1);
  settingsGrid.attach(readerViewSwitch, 1, 8, 2);
  // Icon theme (+ submenu)
  iconThemeButton.set_label("Icon Theme");
  iconThemeButton.property_menu_name() = "icon-theme";
  aboutButton.set_label("About LibreWeb");
  Gtk::Label* iconThemeButtonlabel = dynamic_cast<Gtk::Label*>(iconThemeButton.get_child());
  iconThemeButtonlabel->set_xalign(0.0);
  Gtk::Label* aboutButtonLabel = dynamic_cast<Gtk::Label*>(aboutButton.get_child());
  iconThemeButtonlabel->set_xalign(0.0);
  aboutButtonLabel->set_xalign(0.0);
  // Add Settings vbox to popover menu
  vboxSettings.set_margin_start(10);
  vboxSettings.set_margin_end(10);
  vboxSettings.set_margin_top(10);
  vboxSettings.set_margin_bottom(10);
  vboxSettings.set_spacing(8);
  vboxSettings.add(hboxSetingsZoom);
  vboxSettings.add(hboxSetingsBrightness);
  vboxSettings.add(separator5);
  vboxSettings.add(settingsGrid);
  vboxSettings.add(separator6);
  vboxSettings.add(iconThemeButton);
  vboxSettings.add(separator7);
  vboxSettings.pack_end(aboutButton, false, false);
  settingsPopover.set_position(Gtk::POS_BOTTOM);
  settingsPopover.set_size_request(200, 300);
  settingsPopover.set_margin_end(2);
  settingsPopover.add(vboxSettings);
  // Add Theme vbox to popover menu
  iconThemeBackButton.set_label("Icon Theme");
  iconThemeBackButton.property_menu_name() = "main";
  iconThemeBackButton.property_inverted() = true;
  // List of themes in list box
  Gtk::Label* iconTheme1 = Gtk::manage(new Gtk::Label("Flat theme"));
  Gtk::ListBoxRow* row1 = Gtk::manage(new Gtk::ListBoxRow());
  row1->add(*iconTheme1);
  row1->set_data("value", (void*)"flat");
  Gtk::Label* iconTheme2 = Gtk::manage(new Gtk::Label("Filled theme"));
  Gtk::ListBoxRow* row2 = Gtk::manage(new Gtk::ListBoxRow());
  row2->add(*iconTheme2);
  row2->set_data("value", (void*)"filled");
  Gtk::Label* iconTheme3 = Gtk::manage(new Gtk::Label("Gtk default theme"));
  Gtk::ListBoxRow* row3 = Gtk::manage(new Gtk::ListBoxRow());
  row3->add(*iconTheme3);
  row3->set_data("value", (void*)"none");
  iconThemeListBox.add(*row1);
  iconThemeListBox.add(*row2);
  iconThemeListBox.add(*row3);
  // Select the correct row by default
  if (use_current_gtk_icon_theme_)
    iconThemeListBox.select_row(*row3);
  else if (icon_theme_ == "flat")
    iconThemeListBox.select_row(*row1);
  else if (icon_theme_ == "filled")
    iconThemeListBox.select_row(*row2);
  else
    iconThemeListBox.select_row(*row1); // flat is fallback
  iconThemeListScrolledWindow.property_height_request() = 200;
  iconThemeListScrolledWindow.add(iconThemeListBox);
  iconThemeLabel.get_style_context()->add_class("dim-label");
  vboxIconTheme.add(iconThemeBackButton);
  vboxIconTheme.add(separator8);
  vboxIconTheme.add(iconThemeLabel);
  vboxIconTheme.add(iconThemeListScrolledWindow);
  settingsPopover.add(vboxIconTheme);
  settingsPopover.child_property_submenu(vboxIconTheme) = "icon-theme";
  settingsPopover.show_all_children();
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
  closeTocWindowButton.signal_clicked().connect(sigc::mem_fun(vboxToc, &Gtk::Widget::hide));
  tocTreeView.signal_row_activated().connect(sigc::mem_fun(this, &MainWindow::on_toc_row_activated));
  // Menu & toolbar signals
  menu.new_doc.connect(sigc::mem_fun(this, &MainWindow::new_doc));                       /*!< Menu item for new document */
  menu.open.connect(sigc::mem_fun(this, &MainWindow::open));                             /*!< Menu item for opening existing document */
  menu.open_edit.connect(sigc::mem_fun(this, &MainWindow::open_and_edit));               /*!< Menu item for opening & editing existing document */
  menu.edit.connect(sigc::mem_fun(this, &MainWindow::edit));                             /*!< Menu item for editing current open document */
  menu.save.connect(sigc::mem_fun(this, &MainWindow::save));                             /*!< Menu item for save document */
  menu.save_as.connect(sigc::mem_fun(this, &MainWindow::save_as));                       /*!< Menu item for save document as */
  menu.publish.connect(sigc::mem_fun(this, &MainWindow::publish));                       /*!< Menu item for publishing */
  menu.quit.connect(sigc::mem_fun(this, &MainWindow::close));                            /*!< close main window and therefor closes the app */
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
  sourceCodeDialog.signal_response().connect(sigc::mem_fun(sourceCodeDialog, &SourceCodeDialog::hide_dialog)); /*!< Close source code dialog */
  menu.about.connect(sigc::mem_fun(about, &About::show_about));                                                /*!< Display about dialog */
  draw_primary.source_code.connect(sigc::mem_fun(this, &MainWindow::show_source_code_dialog));                 /*!< Open source code dialog */
  about.signal_response().connect(sigc::mem_fun(about, &About::hide_about));                                   /*!< Close about dialog */
  addressBar.signal_activate().connect(sigc::mem_fun(this, &MainWindow::address_bar_activate)); /*!< User pressed enter the address bar */
  openTocButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::show_toc));           /*!< Button for showing Table of Contents */
  backButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::back));                  /*!< Button for previous page */
  forwardButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::forward));            /*!< Button for next page */
  refreshButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::refresh_request));    /*!< Button for reloading the page */
  homeButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::go_home));               /*!< Button for home page */
  searchEntry.signal_activate().connect(sigc::mem_fun(this, &MainWindow::on_search));           /*!< Execute the text search */
  searchReplaceEntry.signal_activate().connect(sigc::mem_fun(this, &MainWindow::on_replace));   /*!< Execute the text replace */
  // Editor toolbar buttons
  openButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::open_and_edit));
  saveButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::save));
  publishButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::publish));
  cutButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::cut));
  copyButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::copy));
  pasteButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::paste));
  undoButton.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::undo));
  redoButton.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::redo));
  headingsComboBox.signal_changed().connect(sigc::mem_fun(this, &MainWindow::get_heading));
  boldButton.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_bold));
  italicButton.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_italic));
  strikethroughButton.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_strikethrough));
  superButton.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_super));
  subButton.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_sub));
  linkButton.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::insert_link));
  imageButton.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::insert_image));
  emojiButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::insert_emoji));
  quoteButton.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_quote));
  codeButton.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_code));
  bulletListButton.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::insert_bullet_list));
  numberedListButton.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::insert_numbered_list));
  highlightButton.signal_clicked().connect(sigc::mem_fun(draw_primary, &Draw::make_highlight));
  // Status pop-over buttons
  copyIDButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::copy_client_id));
  copyPublicKeyButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::copy_client_public_key));
  // Settings pop-over buttons
  zoomOutButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::on_zoom_out));
  zoomRestoreButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::on_zoom_restore));
  zoomInButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::on_zoom_in));
  fontButton.signal_font_set().connect(sigc::mem_fun(this, &MainWindow::on_font_set));
  maxContentWidthSpinButton.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_max_content_width_changed));
  spacingSpinButton.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_spacing_changed));
  marginsSpinButton.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_margins_changed));
  indentSpinButton.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_indent_changed));
  wrapNone.signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &MainWindow::on_wrap_toggled), Gtk::WrapMode::WRAP_NONE));
  wrapChar.signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &MainWindow::on_wrap_toggled), Gtk::WrapMode::WRAP_CHAR));
  wrapWord.signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &MainWindow::on_wrap_toggled), Gtk::WrapMode::WRAP_WORD));
  wrapWordChar.signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &MainWindow::on_wrap_toggled), Gtk::WrapMode::WRAP_WORD_CHAR));
  themeSwitch.property_active().signal_changed().connect(sigc::mem_fun(this, &MainWindow::on_theme_changed));
  readerViewSwitch.property_active().signal_changed().connect(sigc::mem_fun(this, &MainWindow::on_reader_view_changed));
  iconThemeListBox.signal_row_activated().connect(sigc::mem_fun(this, &MainWindow::on_icon_theme_activated));
  aboutButton.signal_clicked().connect(sigc::mem_fun(about, &About::show_about));
}

void MainWindow::init_mac_os()
{
#if defined(__APPLE__)
  {
    osx_app = (GtkosxApplication*)g_object_new(GTKOSX_TYPE_APPLICATION, NULL);
    // TODO: Should I implement those terminate signals. Sinse I disabled quartz accelerators
    MainWindow* mainWindow = this;
    g_signal_connect(osx_app, "NSApplicationWillTerminate", G_CALLBACK(osx_will_quit_cb), mainWindow);
    // TODO: Open file callback?
    // g_signal_connect (osx_app, "NSApplicationOpenFile", G_CALLBACK (osx_open_file_cb), mainWindow);
    menu.hide();
    GtkWidget* menubar = (GtkWidget*)menu.gobj();
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
    if (panedRoot.get_position() > 0)
      settings->set_int("position-divider-toc", panedRoot.get_position());
    // Only store a divider value bigger than zero,
    // because the secondary draw window is hidden by default, resulting into a zero value.
    if (panedDraw.get_position() > 0)
      settings->set_int("position-divider-draw", panedDraw.get_position());
    // Fullscreen will be availible with gtkmm-4.0
    // settings->set_boolean("fullscreen", is_fullscreen());
    settings->set_string("font-family", font_family_);
    settings->set_int("font-size", current_font_size_);
    settings->set_int("max-content-width", content_max_width_);
    settings->set_double("spacing", font_spacing_);
    settings->set_int("margins", content_margin_);
    settings->set_int("indent", indent_);
    settings->set_enum("wrap-mode", wrap_mode_);
    settings->set_string("icon-theme", icon_theme_);
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
  else if (addressBar.has_focus())
  {
    addressBar.cut_clipboard();
  }
  else if (searchEntry.has_focus())
  {
    searchEntry.cut_clipboard();
  }
  else if (searchReplaceEntry.has_focus())
  {
    searchReplaceEntry.cut_clipboard();
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
  else if (addressBar.has_focus())
  {
    addressBar.copy_clipboard();
  }
  else if (searchEntry.has_focus())
  {
    searchEntry.copy_clipboard();
  }
  else if (searchReplaceEntry.has_focus())
  {
    searchReplaceEntry.copy_clipboard();
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
  else if (addressBar.has_focus())
  {
    addressBar.paste_clipboard();
  }
  else if (searchEntry.has_focus())
  {
    searchEntry.paste_clipboard();
  }
  else if (searchReplaceEntry.has_focus())
  {
    searchReplaceEntry.paste_clipboard();
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
  else if (addressBar.has_focus())
  {
    int start, end;
    if (addressBar.get_selection_bounds(start, end))
    {
      addressBar.delete_text(start, end);
    }
    else
    {
      ++end;
      addressBar.delete_text(start, end);
    }
  }
  else if (searchEntry.has_focus())
  {
    int start, end;
    if (searchEntry.get_selection_bounds(start, end))
    {
      searchEntry.delete_text(start, end);
    }
    else
    {
      ++end;
      searchEntry.delete_text(start, end);
    }
  }
  else if (searchReplaceEntry.has_focus())
  {
    int start, end;
    if (searchReplaceEntry.get_selection_bounds(start, end))
    {
      searchReplaceEntry.delete_text(start, end);
    }
    else
    {
      ++end;
      searchReplaceEntry.delete_text(start, end);
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
  else if (addressBar.has_focus())
  {
    addressBar.select_region(0, -1);
  }
  else if (searchEntry.has_focus())
  {
    searchEntry.select_region(0, -1);
  }
  else if (searchReplaceEntry.has_focus())
  {
    searchReplaceEntry.select_region(0, -1);
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
  const auto iter = tocTreeModel->get_iter(path);
  if (iter)
  {
    const auto row = *iter;
    if (row[tocColumns.col_valid])
    {
      Gtk::TextIter textIter = row[tocColumns.col_iter];
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
  addressBar.set_text("file://unsaved");
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
      contentPublishedDialog.reset(new Gtk::MessageDialog(*this, "File is successfully added to IPFS!"));
      contentPublishedDialog->set_secondary_text("The content is now available on the decentralized web, via:");
      // Add custom label
      Gtk::Label* label = Gtk::manage(new Gtk::Label("ipfs://" + cid));
      label->set_selectable(true);
      Gtk::Box* box = contentPublishedDialog->get_content_area();
      box->pack_end(*label);

      contentPublishedDialog->set_modal(true);
      // contentPublishedDialog->set_hide_on_close(true); available in gtk-4.0
      contentPublishedDialog->signal_response().connect(sigc::hide(sigc::mem_fun(*contentPublishedDialog, &Gtk::Widget::hide)));
      contentPublishedDialog->show_all();
    }
    catch (const std::runtime_error& error)
    {
      contentPublishedDialog.reset(new Gtk::MessageDialog(*this, "File could not be added to IPFS", false, Gtk::MESSAGE_ERROR));
      contentPublishedDialog->set_secondary_text("Error message: " + std::string(error.what()));
      contentPublishedDialog->set_modal(true);
      // contentPublishedDialog->set_hide_on_close(true); available in gtk-4.0
      contentPublishedDialog->signal_response().connect(sigc::hide(sigc::mem_fun(*contentPublishedDialog, &Gtk::Widget::hide)));
      contentPublishedDialog->show();
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
  if (vboxToc.is_visible())
    vboxToc.hide();
  else
    vboxToc.show();
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
  std::string text = searchEntry.get_text();
  auto buffer = draw_primary.get_buffer();
  Gtk::TextBuffer::iterator iter = buffer->get_iter_at_mark(buffer->get_mark("insert"));
  Gtk::TextBuffer::iterator start, end;
  bool matchCase = searchMatchCase.get_active();
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
      std::string replace = searchReplaceEntry.get_text();
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
  middleware_.do_request(addressBar.get_text(), false);
  // When user actually entered the address bar, focus on the primary draw
  draw_primary.grab_focus();
}

/**
 * \brief Triggers when user tries to search or replace text
 */
void MainWindow::show_search(bool replace)
{
  if (searchPopover.is_visible() && searchReplaceEntry.is_visible())
  {
    if (replace)
    {
      searchPopover.hide();
      addressBar.grab_focus();
      searchReplaceEntry.hide();
    }
    else
    {
      searchEntry.grab_focus();
      searchReplaceEntry.hide();
    }
  }
  else if (searchPopover.is_visible())
  {
    if (replace)
    {
      searchReplaceEntry.show();
    }
    else
    {
      searchPopover.hide();
      addressBar.grab_focus();
      searchReplaceEntry.hide();
    }
  }
  else
  {
    searchPopover.show();
    searchEntry.grab_focus();
    if (replace)
    {
      searchReplaceEntry.show();
    }
    else
    {
      searchReplaceEntry.hide();
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
      heading1Row = *(tocTreeModel->append());
      heading1Row[tocColumns.col_iter] = headerMark->get_iter();
      heading1Row[tocColumns.col_level] = level;
      heading1Row[tocColumns.col_heading] = heading;
      heading1Row[tocColumns.col_valid] = true;
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
        heading1Row = *(tocTreeModel->append());
        heading1Row[tocColumns.col_level] = 1;
        heading1Row[tocColumns.col_heading] = "-Missing heading-";
        heading1Row[tocColumns.col_valid] = false;
      }
      heading2Row = *(tocTreeModel->append(heading1Row.children()));
      heading2Row[tocColumns.col_iter] = headerMark->get_iter();
      heading2Row[tocColumns.col_level] = level;
      heading2Row[tocColumns.col_heading] = heading;
      heading2Row[tocColumns.col_valid] = true;
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
        heading2Row = *(tocTreeModel->append(heading1Row.children()));
        heading2Row[tocColumns.col_level] = 2;
        heading2Row[tocColumns.col_heading] = "-Missing heading-";
        heading2Row[tocColumns.col_valid] = false;
      }
      heading3Row = *(tocTreeModel->append(heading2Row.children()));
      heading3Row[tocColumns.col_iter] = headerMark->get_iter();
      heading3Row[tocColumns.col_level] = level;
      heading3Row[tocColumns.col_heading] = heading;
      heading3Row[tocColumns.col_valid] = true;
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
        heading3Row = *(tocTreeModel->append(heading2Row.children()));
        heading3Row[tocColumns.col_level] = 3;
        heading3Row[tocColumns.col_heading] = "-Missing heading-";
        heading3Row[tocColumns.col_valid] = false;
      }
      heading4Row = *(tocTreeModel->append(heading3Row.children()));
      heading4Row[tocColumns.col_iter] = headerMark->get_iter();
      heading4Row[tocColumns.col_level] = level;
      heading4Row[tocColumns.col_heading] = heading;
      heading4Row[tocColumns.col_valid] = true;
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
        heading4Row = *(tocTreeModel->append(heading3Row.children()));
        heading4Row[tocColumns.col_level] = 4;
        heading4Row[tocColumns.col_heading] = "-Missing heading-";
        heading4Row[tocColumns.col_valid] = false;
      }
      heading5Row = *(tocTreeModel->append(heading4Row.children()));
      heading5Row[tocColumns.col_iter] = headerMark->get_iter();
      heading5Row[tocColumns.col_level] = level;
      heading5Row[tocColumns.col_heading] = heading;
      heading5Row[tocColumns.col_valid] = true;
      break;
    }
    case 6:
    {
      if (heading5Row->get_model_gobject() == nullptr)
      {
        // Add missing heading as top-level
        heading5Row = *(tocTreeModel->append(heading4Row.children()));
        heading5Row[tocColumns.col_level] = 5;
        heading5Row[tocColumns.col_heading] = "- Missing heading -";
        heading5Row[tocColumns.col_valid] = false;
      }
      auto heading6Row = *(tocTreeModel->append(heading5Row.children()));
      heading6Row[tocColumns.col_iter] = headerMark->get_iter();
      heading6Row[tocColumns.col_level] = level;
      heading6Row[tocColumns.col_heading] = heading;
      heading6Row[tocColumns.col_valid] = true;
      break;
    }
    default:
      std::cerr << "ERROR: Out of range heading level detected." << std::endl;
      break;
    }
    previousLevel = level;
  }
  tocTreeView.columns_autosize();
  tocTreeView.expand_all();
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
    path = (char*)malloc(length + 1);
    if (!path)
    {
      std::cerr << "ERROR: Couldn't create executable path." << std::endl;
    }
    else
    {
      bool is_installed = true;
      wai_getExecutablePath(path, length, NULL);
      path[length] = '\0';
#if defined(_WIN32)
      // Does the executable path starts with "C:\Program"?
      const char* windowsPrefix = "C:\\Program";
      is_installed = (strncmp(path, windowsPrefix, strlen(windowsPrefix)) == 0);
#elif defined(_APPLE_)
      // Does the executable path contains "Applications"?
      const char* macOsNeedle = "Applications";
      is_installed = (strstr(path, macOsNeedle) != NULL);
#elif defined(__linux__)
      // Does the executable path starts with "/usr/local"?
      is_installed = (strncmp(path, INSTALL_PREFIX, strlen(INSTALL_PREFIX)) == 0);
#endif
      free(path);
      return is_installed;
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
  hboxStandardEditorToolbar.show();
  hboxFormattingEditorToolbar.show();
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
  if ((panedDraw.get_position() >= maxWidth) || position_divider_draw_ == -1)
  {
    int proposedPosition = position_divider_draw_; // Try to first use the gsettings
    if ((proposedPosition == -1) || (proposedPosition >= maxWidth))
    {
      proposedPosition = static_cast<int>(currentWidth / 2.0);
    }
    panedDraw.set_position(proposedPosition);
  }
  // Enabled secondary text view (on the right)
  scrolledWindowSecondary.show();
  // Disable "view source" menu item
  draw_primary.set_view_source_menu_item(false);
  // Connect changed signal
  text_changed_signal_handler_ = draw_primary.get_buffer()->signal_changed().connect(sigc::mem_fun(this, &MainWindow::editor_changed_text));
  // Enable publish menu item
  menu.set_publish_menu_sensitive(true);
  // Disable edit menu item (you are already editing)
  menu.set_edit_menu_sensitive(false);
  // Just to be sure, disable the spinning animation
  refreshIcon.get_style_context()->remove_class("spinning");
}

/**
 * \brief Disable editor mode
 */
void MainWindow::disable_edit()
{
  if (is_editor_enabled())
  {
    hboxStandardEditorToolbar.hide();
    hboxFormattingEditorToolbar.hide();
    scrolledWindowSecondary.hide();
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
  return hboxStandardEditorToolbar.is_visible();
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
    std::vector<std::string> path_builder{data_dir, "libreweb", "images", "icons", icon_theme_, typeof_icon, icon_name + ".png"};
    std::string file_path = Glib::build_path(G_DIR_SEPARATOR_S, path_builder);
    if (Glib::file_test(file_path, Glib::FileTest::FILE_TEST_IS_REGULAR))
    {
      return file_path;
    }
  }

  // Try local path if the images are not (yet) installed
  // When working directory is in the build/bin folder (relative path)
  std::vector<std::string> path_builder{"..", "..", "images", "icons", icon_theme_, typeof_icon, icon_name + ".png"};
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
 * \param message The message displayed along with the notificiation
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
  tocTreeModel->clear();
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
  sourceCodeDialog.set_text(middleware_.get_content());
  sourceCodeDialog.run();
}

/**
 * \brief Retrieve selected heading from combobox.
 * Send to main Draw class
 */
void MainWindow::get_heading()
{
  std::string active = headingsComboBox.get_active_id();
  headingsComboBox.set_active(0); // Reset
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
  zoomRestoreButton.set_sensitive(current_font_size_ != default_font_size_);
}

void MainWindow::on_zoom_restore()
{
  current_font_size_ = default_font_size_; // reset
  update_css();
  zoomRestoreButton.set_sensitive(false);
}

void MainWindow::on_zoom_in()
{
  current_font_size_ += 1;
  update_css();
  zoomRestoreButton.set_sensitive(current_font_size_ != default_font_size_);
}

void MainWindow::on_font_set()
{
  Pango::FontDescription fontDesc = Pango::FontDescription(fontButton.get_font_name());
  font_family_ = fontDesc.get_family();
  current_font_size_ = default_font_size_ = (fontDesc.get_size_is_absolute()) ? fontDesc.get_size() : fontDesc.get_size() / PANGO_SCALE;
  update_css();
}

void MainWindow::on_max_content_width_changed()
{
  content_max_width_ = maxContentWidthSpinButton.get_value_as_int();
  if (!is_editor_enabled())
    update_margins();
}

void MainWindow::on_spacing_changed()
{
  font_spacing_ = spacingSpinButton.get_value(); // Letter-spacing
  update_css();
}

void MainWindow::on_margins_changed()
{
  content_margin_ = marginsSpinButton.get_value_as_int();
  update_margins();
}

void MainWindow::on_indent_changed()
{
  indent_ = indentSpinButton.get_value_as_int();
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
  brightness_scale_ = scaleSettingsBrightness.get_value();
  update_css();
}

void MainWindow::on_theme_changed()
{
  // Switch between dark or light theme preference
  use_dark_theme_ = themeSwitch.get_active();
  set_theme();
}

void MainWindow::on_reader_view_changed()
{
  is_reader_view_enabled_ = readerViewSwitch.get_active();
  if (!is_editor_enabled())
    update_margins();
}

void MainWindow::on_icon_theme_activated(Gtk::ListBoxRow* row)
{
  std::string themeName = static_cast<char*>(row->get_data("value"));
  if (themeName != "none")
  {
    icon_theme_ = themeName;
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
