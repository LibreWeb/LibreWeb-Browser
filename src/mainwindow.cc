#include "mainwindow.h"

#include "menu.h"
#include "project_config.h"
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
#include <whereami.h>

MainWindow::MainWindow(const std::string& timeout)
    : m_accelGroup(Gtk::AccelGroup::create()),
      m_settings(),
      m_brightnessAdjustment(Gtk::Adjustment::create(1.0, 0.0, 1.0, 0.05, 0.1)),
      m_spacingAdjustment(Gtk::Adjustment::create(0, -10, 10, 1, 2)),
      m_marginsAdjustment(Gtk::Adjustment::create(20, 0, 1000, 10, 20)),
      m_indentAdjustment(Gtk::Adjustment::create(0, 0, 1000, 5, 10)),
      m_drawCSSProvider(Gtk::CssProvider::create()),
      m_menu(m_accelGroup),
      m_draw_main(middleware_),
      m_draw_secondary(middleware_),
      m_about(*this),
      m_vbox(Gtk::ORIENTATION_VERTICAL, 0),
      m_vboxSearch(Gtk::ORIENTATION_VERTICAL),
      m_vboxStatus(Gtk::ORIENTATION_VERTICAL),
      m_vboxSettings(Gtk::ORIENTATION_VERTICAL),
      m_vboxIconTheme(Gtk::ORIENTATION_VERTICAL),
      m_searchMatchCase("Match _Case", true),
      m_tocButton("Show table of contents (Ctrl+Shift+T)", true),
      m_backButton("Go back one page (Alt+Left arrow)", true),
      m_forwardButton("Go forward one page (Alt+Right arrow)", true),
      m_refreshButton("Reload current page (Ctrl+R)", true),
      m_homeButton("Home page (Alt+Home)", true),
      m_openButton("Open document (Ctrl+O)"),
      m_saveButton("Save document (Ctrl+S)"),
      m_publishButton("Publish document... (Ctrl+P)"),
      m_cutButton("Cut (Ctrl+X)"),
      m_copyButton("Copy (Ctrl+C)"),
      m_pasteButton("Paste (Ctrl+V)"),
      m_undoButton("Undo text (Ctrl+Z)"),
      m_redoButton("Redo text (Ctrl+Y)"),
      m_boldButton("Add bold text"),
      m_italicButton("Add italic text"),
      m_strikethroughButton("Add strikethrough text"),
      m_superButton("Add superscript text"),
      m_subButton("Add subscript text"),
      m_linkButton("Add a link"),
      m_imageButton("Add an image"),
      m_emojiButton("Insert emoji"),
      m_quoteButton("Insert a quote"),
      m_codeButton("Insert code"),
      m_bulletListButton("Add a bullet list"),
      m_numberedListButton("Add a numbered list"),
      m_highlightButton("Add highlight text"),
      m_networkHeadingLabel("IPFS Network"),
      m_networkRateHeadingLabel("Network rate"),
      m_connectivityLabel("Status:"),
      m_peersLabel("Connected peers:"),
      m_repoSizeLabel("Repo size:"),
      m_repoPathLabel("Repo path:"),
      m_ipfsVersionLabel("IPFS version:"),
      m_networkIncomingLabel("Incoming"),
      m_networkOutcomingLabel("Outcoming"),
      m_networkKiloBytesLabel("Kilobytes/s"),
      m_fontLabel("Font"),
      m_spacingLabel("Spacing"),
      m_marginsLabel("Margins"),
      m_indentLabel("Indent"),
      m_themeLabel("Dark Theme"),
      m_iconThemeLabel("Active Theme"),
      // Private members
      middleware_(*this, timeout),
      appName_("LibreWeb Browser"),
      useCurrentGTKIconTheme_(false), // Use LibreWeb icon theme or the GTK icons
      iconTheme_("flat"),             // Default is flat built-in theme
      iconSize_(18),
      fontFamily_("Sans"),
      defaultFontSize_(10),
      currentFontSize_(10),
      fontSpacing_(0),
      brightnessScale_(1.0),
      useDarkTheme_(false),
      currentHistoryIndex_(0)
{
  set_title(appName_);
  set_default_size(1000, 800);
  set_position(Gtk::WIN_POS_CENTER);
  add_accel_group(m_accelGroup);

  loadStoredSettings();
  loadIcons();
  initButtons();
  setTheme();
  initSearchPopover();
  initStatusPopover();
  initSettingsPopover();
  initSignals();

  // Add custom CSS Provider to draw textviews
  auto styleMain = m_draw_main.get_style_context();
  auto styleSecondary = m_draw_secondary.get_style_context();
  styleMain->add_provider(m_drawCSSProvider, GTK_STYLE_PROVIDER_PRIORITY_USER);
  styleSecondary->add_provider(m_drawCSSProvider, GTK_STYLE_PROVIDER_PRIORITY_USER);
  // Load the default font family and font size
  updateCSS();

  // Table of contents
  tocTreeView.append_column("Name", m_tocColumns.m_col_heading);
  tocTreeView.set_activate_on_single_click(true);
  tocTreeView.set_headers_visible(false);
  tocTreeView.set_tooltip_column(1);

  m_scrolledToc.add(tocTreeView);
  m_scrolledToc.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  m_tocTreeModel = Gtk::TreeStore::create(m_tocColumns);
  tocTreeView.set_model(m_tocTreeModel);
  // Browser main drawing area
  m_scrolledWindowMain.add(m_draw_main);
  m_scrolledWindowMain.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  // Secondary drawing area
  m_draw_secondary.setViewSourceMenuItem(false);
  m_scrolledWindowSecondary.add(m_draw_secondary);
  m_scrolledWindowSecondary.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  m_panedDraw.pack1(m_scrolledWindowMain, true, false);
  m_panedDraw.pack2(m_scrolledWindowSecondary, true, true);

  // Add table of contents to horizontal paned
  m_panedRoot.pack1(m_scrolledToc, true, false);
  m_panedRoot.pack2(m_panedDraw, true, false);

  m_vbox.pack_start(m_menu, false, false, 0);
  m_vbox.pack_start(m_hboxBrowserToolbar, false, false, 6);
  m_vbox.pack_start(m_hboxStandardEditorToolbar, false, false, 6);
  m_vbox.pack_start(m_hboxFormattingEditorToolbar, false, false, 6);
  m_vbox.pack_start(m_panedRoot, true, true, 0);
  add(m_vbox);
  show_all_children();

  // Hide by default the replace entry, editor box, ToC & secondary text view
  m_searchReplaceEntry.hide();
  m_hboxStandardEditorToolbar.hide();
  m_hboxFormattingEditorToolbar.hide();
  m_scrolledToc.hide();
  m_scrolledWindowSecondary.hide();

  // Grap focus to input field by default
  m_addressBar.grab_focus();

  // Example of TOC filling
  auto row = *(m_tocTreeModel->append());
  row[m_tocColumns.m_col_id] = 1;
  row[m_tocColumns.m_col_heading] = "Billy Bob";

  auto childrow = *(m_tocTreeModel->append(row.children()));
  childrow[m_tocColumns.m_col_id] = 11;
  childrow[m_tocColumns.m_col_heading] = "Billy Bob Junior";

  tocTreeView.expand_all();

// Show homepage if debugging is disabled
#ifdef NDEBUG
  go_home();
#else
  std::cout << "INFO: Running as Debug mode, opening test.md." << std::endl;
  // Load test file during development
  middleware_.doRequest("file://../../test.md");
#endif
}

/**
 * \brief Called before the requests begins.
 * \param path File path (on disk or IPFS) that needs to be processed.
 * \param title Application title
 * \param isSetAddressBar If true update the address bar with the file path
 * \param isHistoryRequest Set to true if this is an history request call: back/forward
 * \param isDisableEditor If true the editor will be disabled if needed
 */
void MainWindow::preRequest(const std::string& path, const std::string& title, bool isSetAddressBar, bool isHistoryRequest, bool isDisableEditor)
{
  if (isSetAddressBar)
    m_addressBar.set_text(path);
  if (!title.empty())
    set_title(title + " - " + appName_);
  else
    set_title(appName_);
  if (isDisableEditor && isEditorEnabled())
    disableEdit();

  // Do not insert history back/forward calls into the history (again)
  if (!isHistoryRequest && !path.empty())
  {
    if (history_.empty())
    {
      history_.push_back(path);
      currentHistoryIndex_ = history_.size() - 1;
    }
    else if (history_.back().compare(path) != 0)
    {
      history_.push_back(path);
      currentHistoryIndex_ = history_.size() - 1;
    }
  }
  // Enable back/forward buttons when possible
  m_backButton.set_sensitive(currentHistoryIndex_ > 0);
  m_menu.setBackMenuSensitive(currentHistoryIndex_ > 0);
  m_forwardButton.set_sensitive(currentHistoryIndex_ < history_.size() - 1);
  m_menu.setForwardMenuSensitive(currentHistoryIndex_ < history_.size() - 1);
}

/**
 * \brief Called after file is written to disk.
 */
void MainWindow::postWrite(const std::string& path, const std::string& title, bool isSetAddressAndTitle)
{
  if (isSetAddressAndTitle)
  {
    m_addressBar.set_text(path);
    set_title(title + " - " + appName_);
  }
}

/**
 * \brief Called when request started (from thread).
 */
void MainWindow::startedRequest()
{
  // Start spinning icon
  m_refreshIcon.get_style_context()->add_class("spinning");
}

/**
 * \brief Called when request is finished (from thread).
 */
void MainWindow::finishedRequest()
{
  // Stop spinning icon
  m_refreshIcon.get_style_context()->remove_class("spinning");
}

/**
 * \brief Refresh the current page
 */
void MainWindow::refreshRequest()
{
  // Only allow refresh if editor is disabled (doesn't make sense otherwise to refresh)
  if (!isEditorEnabled())
    // Reload existing file, don't need to update the address bar, don't disable the editor
    middleware_.doRequest("", false, false, false);
}

/**
 * \brief Show startpage
 */
void MainWindow::showStartpage()
{
  m_draw_main.showStartPage();
}

/**
 * \brief Set plain text
 * \param content content string
 */
void MainWindow::setText(const Glib::ustring& content)
{
  m_draw_main.setText(content);
}

/**
 * \brief Set markdown document (cmark). cmark_node pointer will be freed automatically.
 * \param rootNode cmark root data struct
 */
void MainWindow::setDocument(cmark_node* rootNode)
{
  m_draw_main.setDocument(rootNode);
}

/**
 * \brief Set message with optionally additional details
 * \param message Message string
 * \param details Details string
 */
void MainWindow::setMessage(const Glib::ustring& message, const Glib::ustring& details)
{
  m_draw_main.setMessage(message, details);
}

/**
 * \brief Update all status fields in status pop-over menu + status icon
 */
void MainWindow::updateStatusPopoverAndIcon()
{
  std::string networkStatus;
  std::size_t nrOfPeers = middleware_.getIPFSNumberOfPeers();
  // Update status icon
  if (nrOfPeers > 0)
  {
    networkStatus = "Connected";
    if (useCurrentGTKIconTheme_)
    {
      m_statusIcon.set_from_icon_name("network-wired-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    }
    else
    {
      m_statusIcon.set(m_statusOnlineIcon);
    }
  }
  else
  {
    networkStatus = "Disconnected";
    if (useCurrentGTKIconTheme_)
    {
      m_statusIcon.set_from_icon_name("network-wired-disconnected-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    }
    else
    {
      m_statusIcon.set(m_statusOfflineIcon);
    }
  }
  m_connectivityStatusLabel.set_markup("<b>" + networkStatus + "</b>");
  m_peersStatusLabel.set_text(std::to_string(nrOfPeers));
  m_repoSizeStatusLabel.set_text(std::to_string(middleware_.getIPFSRepoSize()) + " MB");
  m_repoPathStatusLabel.set_text(middleware_.getIPFSRepoPath());
  m_networkIncomingStatusLabel.set_text(middleware_.getIPFSIncomingRate());
  m_networkOutcomingStatusLabel.set_text(middleware_.getIPFSOutcomingRate());
  m_ipfsVersionStatusLabel.set_text(middleware_.getIPFSVersion());
}

/**
 * Load stored settings from GSettings scheme file
 */
void MainWindow::loadStoredSettings()
{
  // Set additional schema directory, when browser is not yet installed
  if (!isInstalled())
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
    m_settings = Gio::Settings::create("org.libreweb.browser");
    // Apply global settings
    set_default_size(m_settings->get_int("width"), m_settings->get_int("height"));
    if (m_settings->get_boolean("maximized"))
      maximize();

    fontFamily_ = m_settings->get_string("font-family");
    currentFontSize_ = defaultFontSize_ = m_settings->get_int("font-size");
    m_fontButton.set_font_name(fontFamily_ + " " + std::to_string(currentFontSize_));

    fontSpacing_ = m_settings->get_int("spacing");
    int margins = m_settings->get_int("margins");
    int indent = m_settings->get_int("indent");
    m_spacingAdjustment->set_value(fontSpacing_);
    m_marginsAdjustment->set_value(margins);
    m_indentAdjustment->set_value(indent);
    m_draw_main.set_left_margin(margins);
    m_draw_main.set_right_margin(margins);
    m_draw_main.set_indent(indent);
    int tocDividerPosition = m_settings->get_int("position-divider-toc");
    m_panedRoot.set_position(tocDividerPosition);
    iconTheme_ = m_settings->get_string("icon-theme");
    useCurrentGTKIconTheme_ = m_settings->get_boolean("icon-gtk-theme");
    brightnessScale_ = m_settings->get_double("brightness");
    useDarkTheme_ = m_settings->get_boolean("dark-theme");
  }
  else
  {
    std::cerr << "ERROR: Gsettings schema file could not be found!" << std::endl;
    // Adjustment controls
    int margins = 20;
    int indent = 0;
    m_spacingAdjustment->set_value(0);
    m_marginsAdjustment->set_value(margins);
    m_indentAdjustment->set_value(indent);
    // For drawing
    m_draw_main.set_left_margin(margins);
    m_draw_main.set_right_margin(margins);
    m_draw_main.set_indent(indent);
    // ToC paned divider
    m_panedRoot.set_position(300);
  }
}

/**
 * \brief set GTK Icons
 */
void MainWindow::setGTKIcons()
{
  // Toolbox buttons
  m_tocIcon.set_from_icon_name("view-list-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  m_backIcon.set_from_icon_name("go-previous", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  m_forwardIcon.set_from_icon_name("go-next", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  m_refreshIcon.set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  m_homeIcon.set_from_icon_name("go-home", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  m_searchIcon.set_from_icon_name("edit-find-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  m_settingsIcon.set_from_icon_name("open-menu-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  // Settings pop-over buttons
  m_zoomOutImage.set_from_icon_name("zoom-out-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  m_zoomInImage.set_from_icon_name("zoom-in-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  m_brightnessImage.set_from_icon_name("display-brightness-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
}

/**
 * Load all icon images from theme/disk. Or reload them.
 */
void MainWindow::loadIcons()
{
  try
  {
    // Editor buttons
    m_openIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("open_folder", "folders"), iconSize_, iconSize_));
    m_saveIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("floppy_disk", "basic"), iconSize_, iconSize_));
    m_publishIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("upload", "basic"), iconSize_, iconSize_));
    m_cutIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("cut", "editor"), iconSize_, iconSize_));
    m_copyIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("copy", "editor"), iconSize_, iconSize_));
    m_pasteIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("clipboard", "editor"), iconSize_, iconSize_));
    m_undoIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("undo", "editor"), iconSize_, iconSize_));
    m_redoIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("redo", "editor"), iconSize_, iconSize_));
    m_boldIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("bold", "editor"), iconSize_, iconSize_));
    m_italicIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("italic", "editor"), iconSize_, iconSize_));
    m_strikethroughIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("strikethrough", "editor"), iconSize_, iconSize_));
    m_superIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("superscript", "editor"), iconSize_, iconSize_));
    m_subIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("subscript", "editor"), iconSize_, iconSize_));
    m_linkIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("link", "editor"), iconSize_, iconSize_));
    m_imageIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("shapes", "editor"), iconSize_, iconSize_));
    m_emojiIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("smile", "smiley"), iconSize_, iconSize_));
    m_quoteIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("quote", "editor"), iconSize_, iconSize_));
    m_codeIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("code", "editor"), iconSize_, iconSize_));
    m_bulletListIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("bullet_list", "editor"), iconSize_, iconSize_));
    m_numberedListIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("number_list", "editor"), iconSize_, iconSize_));
    m_hightlightIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("highlighter", "editor"), iconSize_, iconSize_));

    if (useCurrentGTKIconTheme_)
    {
      setGTKIcons();
    }
    else
    {
      // Toolbox buttons
      m_tocIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("square_list", "editor"), iconSize_, iconSize_));
      m_backIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("right_arrow_1", "arrows"), iconSize_, iconSize_)->flip());
      m_forwardIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("right_arrow_1", "arrows"), iconSize_, iconSize_));
      m_refreshIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("reload_centered", "arrows"), iconSize_ * 1.13, iconSize_));
      m_homeIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("home", "basic"), iconSize_, iconSize_));
      m_searchIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("search", "basic"), iconSize_, iconSize_));
      m_settingsIcon.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("menu", "basic"), iconSize_, iconSize_));

      // Settings pop-over buttons
      m_zoomOutImage.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("zoom_out", "basic"), iconSize_, iconSize_));
      m_zoomInImage.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("zoom_in", "basic"), iconSize_, iconSize_));
      m_brightnessImage.set(Gdk::Pixbuf::create_from_file(getIconImageFromTheme("brightness", "basic"), iconSize_, iconSize_));
      m_statusOfflineIcon = Gdk::Pixbuf::create_from_file(getIconImageFromTheme("network_disconnected", "network"), iconSize_, iconSize_);
      m_statusOnlineIcon = Gdk::Pixbuf::create_from_file(getIconImageFromTheme("network_connected", "network"), iconSize_, iconSize_);
    }
  }
  catch (const Glib::FileError& error)
  {
    std::cerr << "ERROR: Icon could not be loaded, file error: " << error.what() << ".\nContinue nevertheless, with GTK icons as fallback..."
              << std::endl;
    setGTKIcons();
    useCurrentGTKIconTheme_ = true;
  }
  catch (const Gdk::PixbufError& error)
  {
    std::cerr << "ERROR: Icon could not be loaded, pixbuf error: " << error.what() << ".\nContinue nevertheless, with GTK icons as fallback..."
              << std::endl;
    setGTKIcons();
    useCurrentGTKIconTheme_ = true;
  }
}

/**
 * Init all buttons / comboboxes from the toolbars
 */
void MainWindow::initButtons()
{
  // Add icons to the toolbar editor buttons
  m_openButton.add(m_openIcon);
  m_saveButton.add(m_saveIcon);
  m_publishButton.add(m_publishIcon);
  m_cutButton.add(m_cutIcon);
  m_copyButton.add(m_copyIcon);
  m_pasteButton.add(m_pasteIcon);
  m_undoButton.add(m_undoIcon);
  m_redoButton.add(m_redoIcon);
  m_boldButton.add(m_boldIcon);
  m_italicButton.add(m_italicIcon);
  m_strikethroughButton.add(m_strikethroughIcon);
  m_superButton.add(m_superIcon);
  m_subButton.add(m_subIcon);
  m_linkButton.add(m_linkIcon);
  m_imageButton.add(m_imageIcon);
  m_emojiButton.add(m_emojiIcon);
  m_quoteButton.add(m_quoteIcon);
  m_codeButton.add(m_codeIcon);
  m_bulletListButton.add(m_bulletListIcon);
  m_numberedListButton.add(m_numberedListIcon);
  m_highlightButton.add(m_hightlightIcon);

  // Disable focus the other buttons as well
  m_searchMatchCase.set_can_focus(false);
  m_headingsComboBox.set_can_focus(false);
  m_headingsComboBox.set_focus_on_click(false);

  // Populate the heading comboboxtext
  m_headingsComboBox.append("", "Select Heading");
  m_headingsComboBox.append("1", "Heading 1");
  m_headingsComboBox.append("2", "Heading 2");
  m_headingsComboBox.append("3", "Heading 3");
  m_headingsComboBox.append("4", "Heading 4");
  m_headingsComboBox.append("5", "Heading 5");
  m_headingsComboBox.append("6", "Heading 6");
  m_headingsComboBox.set_active(0);

  // Horizontal bar
  m_backButton.get_style_context()->add_class("circular");
  m_forwardButton.get_style_context()->add_class("circular");
  m_refreshButton.get_style_context()->add_class("circular");
  m_searchButton.set_popover(m_searchPopover);
  m_statusButton.set_popover(m_statusPopover);
  m_settingsButton.set_popover(m_settingsPopover);
  m_searchButton.set_relief(Gtk::RELIEF_NONE);
  m_statusButton.set_relief(Gtk::RELIEF_NONE);
  m_settingsButton.set_relief(Gtk::RELIEF_NONE);

  // Add icons to the toolbar buttons
  m_tocButton.add(m_tocIcon);
  m_backButton.add(m_backIcon);
  m_forwardButton.add(m_forwardIcon);
  m_refreshButton.add(m_refreshIcon);
  m_homeButton.add(m_homeIcon);
  m_searchButton.add(m_searchIcon);
  m_statusButton.add(m_statusIcon);
  m_settingsButton.add(m_settingsIcon);

  // Add spinning CSS class to refresh icon
  auto cssProvider = Gtk::CssProvider::create();
  auto screen = Gdk::Screen::get_default();
  std::string spinningCSS = "@keyframes spin {  to { -gtk-icon-transform: rotate(1turn); }} .spinning { animation-name: spin;  "
                            "animation-duration: 1s;  animation-timing-function: linear;  animation-iteration-count: infinite;}";
  if (!cssProvider->load_from_data(spinningCSS))
  {
    std::cerr << "ERROR: CSS data parsing went wrong." << std::endl;
  }
  auto refreshIconStyle = m_refreshIcon.get_style_context();
  refreshIconStyle->add_provider_for_screen(screen, cssProvider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  // Add tooltips to the toolbar buttons
  m_searchButton.set_tooltip_text("Find");
  m_statusButton.set_tooltip_text("IPFS Network Status");
  m_settingsButton.set_tooltip_text("Settings");
  // Disable back/forward buttons on start-up
  m_backButton.set_sensitive(false);
  m_forwardButton.set_sensitive(false);

  /*
   * Adding the buttons to the boxes
   */
  // Browser Toolbar
  m_tocButton.set_margin_left(6);
  m_hboxBrowserToolbar.pack_start(m_tocButton, false, false, 0);
  m_hboxBrowserToolbar.pack_start(m_backButton, false, false, 0);
  m_hboxBrowserToolbar.pack_start(m_forwardButton, false, false, 0);
  m_hboxBrowserToolbar.pack_start(m_refreshButton, false, false, 0);
  m_hboxBrowserToolbar.pack_start(m_homeButton, false, false, 0);
  m_hboxBrowserToolbar.pack_start(m_addressBar, true, true, 4);
  m_hboxBrowserToolbar.pack_start(m_searchButton, false, false, 0);
  m_hboxBrowserToolbar.pack_start(m_statusButton, false, false, 0);
  m_hboxBrowserToolbar.pack_start(m_settingsButton, false, false, 0);

  // Standard editor toolbar
  m_headingsComboBox.set_margin_left(4);
  m_hboxStandardEditorToolbar.pack_start(m_openButton, false, false, 2);
  m_hboxStandardEditorToolbar.pack_start(m_saveButton, false, false, 2);
  m_hboxStandardEditorToolbar.pack_start(m_publishButton, false, false, 2);
  m_hboxStandardEditorToolbar.pack_start(m_separator1, false, false, 0);
  m_hboxStandardEditorToolbar.pack_start(m_cutButton, false, false, 2);
  m_hboxStandardEditorToolbar.pack_start(m_copyButton, false, false, 2);
  m_hboxStandardEditorToolbar.pack_start(m_pasteButton, false, false, 2);
  m_hboxStandardEditorToolbar.pack_start(m_separator2, false, false, 0);
  m_hboxStandardEditorToolbar.pack_start(m_undoButton, false, false, 2);
  m_hboxStandardEditorToolbar.pack_start(m_redoButton, false, false, 2);

  // Formatting toolbar
  m_headingsComboBox.set_margin_left(4);
  m_hboxFormattingEditorToolbar.pack_start(m_headingsComboBox, false, false, 2);
  m_hboxFormattingEditorToolbar.pack_start(m_boldButton, false, false, 2);
  m_hboxFormattingEditorToolbar.pack_start(m_italicButton, false, false, 2);
  m_hboxFormattingEditorToolbar.pack_start(m_strikethroughButton, false, false, 2);
  m_hboxFormattingEditorToolbar.pack_start(m_superButton, false, false, 2);
  m_hboxFormattingEditorToolbar.pack_start(m_subButton, false, false, 2);
  m_hboxFormattingEditorToolbar.pack_start(m_separator3, false, false, 0);
  m_hboxFormattingEditorToolbar.pack_start(m_linkButton, false, false, 2);
  m_hboxFormattingEditorToolbar.pack_start(m_imageButton, false, false, 2);
  m_hboxFormattingEditorToolbar.pack_start(m_emojiButton, false, false, 2);
  m_hboxFormattingEditorToolbar.pack_start(m_separator4, false, false, 0);
  m_hboxFormattingEditorToolbar.pack_start(m_quoteButton, false, false, 2);
  m_hboxFormattingEditorToolbar.pack_start(m_codeButton, false, false, 2);
  m_hboxFormattingEditorToolbar.pack_start(m_bulletListButton, false, false, 2);
  m_hboxFormattingEditorToolbar.pack_start(m_numberedListButton, false, false, 2);
  m_hboxFormattingEditorToolbar.pack_start(m_highlightButton, false, false, 2);
}

/**
 * \brief Prefer dark or light theme
 */
void MainWindow::setTheme()
{
  auto settings = Gtk::Settings::get_default();
  settings->property_gtk_application_prefer_dark_theme().set_value(useDarkTheme_);
}

/**
 * \brief Popover search bar
 */
void MainWindow::initSearchPopover()
{
  m_searchEntry.set_placeholder_text("Find");
  m_searchReplaceEntry.set_placeholder_text("Replace");
  m_search.connect_entry(m_searchEntry);
  m_searchReplace.connect_entry(m_searchReplaceEntry);
  m_searchEntry.set_size_request(250, -1);
  m_searchReplaceEntry.set_size_request(250, -1);
  m_vboxSearch.set_margin_left(8);
  m_vboxSearch.set_margin_right(8);
  m_vboxSearch.set_spacing(8);
  m_hboxSearch.set_spacing(8);

  m_hboxSearch.pack_start(m_searchEntry, false, false);
  m_hboxSearch.pack_start(m_searchMatchCase, false, false);
  m_vboxSearch.pack_start(m_hboxSearch, false, false, 4);
  m_vboxSearch.pack_end(m_searchReplaceEntry, false, false, 4);
  m_searchPopover.set_position(Gtk::POS_BOTTOM);
  m_searchPopover.set_size_request(300, 50);
  m_searchPopover.add(m_vboxSearch);
  m_searchPopover.show_all_children();
}

/**
 * Init the IPFS status pop-over
 */
void MainWindow::initStatusPopover()
{
  m_connectivityLabel.set_xalign(0.0);
  m_peersLabel.set_xalign(0.0);
  m_repoSizeLabel.set_xalign(0.0);
  m_repoPathLabel.set_xalign(0.0);
  m_ipfsVersionLabel.set_xalign(0.0);
  m_connectivityStatusLabel.set_xalign(1.0);
  m_peersStatusLabel.set_xalign(1.0);
  m_repoSizeStatusLabel.set_xalign(1.0);
  m_repoPathStatusLabel.set_xalign(1.0);
  m_ipfsVersionStatusLabel.set_xalign(1.0);
  m_connectivityLabel.get_style_context()->add_class("dim-label");
  m_peersLabel.get_style_context()->add_class("dim-label");
  m_repoSizeLabel.get_style_context()->add_class("dim-label");
  m_repoPathLabel.get_style_context()->add_class("dim-label");
  m_ipfsVersionLabel.get_style_context()->add_class("dim-label");
  // Status popover grid
  m_statusGrid.set_column_homogeneous(true);
  m_statusGrid.set_margin_start(6);
  m_statusGrid.set_margin_top(6);
  m_statusGrid.set_margin_bottom(6);
  m_statusGrid.set_margin_end(12);
  m_statusGrid.set_row_spacing(10);
  m_statusGrid.set_column_spacing(6);
  m_statusGrid.attach(m_connectivityLabel, 0, 0);
  m_statusGrid.attach(m_connectivityStatusLabel, 1, 0);
  m_statusGrid.attach(m_peersLabel, 0, 1);
  m_statusGrid.attach(m_peersStatusLabel, 1, 1);
  m_statusGrid.attach(m_repoSizeLabel, 0, 2);
  m_statusGrid.attach(m_repoSizeStatusLabel, 1, 2);
  m_statusGrid.attach(m_repoPathLabel, 0, 3);
  m_statusGrid.attach(m_repoPathStatusLabel, 1, 3);
  m_statusGrid.attach(m_ipfsVersionLabel, 0, 4);
  m_statusGrid.attach(m_ipfsVersionStatusLabel, 1, 4);
  // IPFS Network activity status grid
  m_networkKiloBytesLabel.get_style_context()->add_class("dim-label");
  m_activityStatusGrid.set_column_homogeneous(true);
  m_activityStatusGrid.set_margin_start(6);
  m_activityStatusGrid.set_margin_top(6);
  m_activityStatusGrid.set_margin_bottom(6);
  m_activityStatusGrid.set_margin_end(6);
  m_activityStatusGrid.set_row_spacing(10);
  m_activityStatusGrid.set_column_spacing(6);
  m_activityStatusGrid.attach(m_networkIncomingLabel, 1, 0);
  m_activityStatusGrid.attach(m_networkOutcomingLabel, 2, 0);
  m_activityStatusGrid.attach(m_networkKiloBytesLabel, 0, 1);
  m_activityStatusGrid.attach(m_networkIncomingStatusLabel, 1, 1);
  m_activityStatusGrid.attach(m_networkOutcomingStatusLabel, 2, 1);

  m_networkHeadingLabel.get_style_context()->add_class("dim-label");
  m_networkRateHeadingLabel.get_style_context()->add_class("dim-label");
  // Copy ID & public key buttons
  m_copyIDButton.set_label("Copy your ID");
  m_copyPublicKeyButton.set_label("Copy Public Key");
  m_copyIDButton.set_margin_start(6);
  m_copyIDButton.set_margin_end(6);
  m_copyPublicKeyButton.set_margin_start(6);
  m_copyPublicKeyButton.set_margin_end(6);
  // Add all items to status box & status popover
  m_vboxStatus.set_margin_start(10);
  m_vboxStatus.set_margin_end(10);
  m_vboxStatus.set_margin_top(10);
  m_vboxStatus.set_margin_bottom(10);
  m_vboxStatus.set_spacing(6);
  m_vboxStatus.add(m_networkHeadingLabel);
  m_vboxStatus.add(m_statusGrid);
  m_vboxStatus.add(m_separator9);
  m_vboxStatus.add(m_networkRateHeadingLabel);
  m_vboxStatus.add(m_activityStatusGrid);
  m_vboxStatus.add(m_separator10);
  m_vboxStatus.add(m_copyPublicKeyButton);
  m_vboxStatus.add(m_copyIDButton);
  m_statusPopover.set_position(Gtk::POS_BOTTOM);
  m_statusPopover.set_size_request(100, 250);
  m_statusPopover.set_margin_end(2);
  m_statusPopover.add(m_vboxStatus);
  m_statusPopover.show_all_children();
  // Set fallback values for all status fields + status icon
  updateStatusPopoverAndIcon();
}

/**
 * Init the settings pop-over
 */
void MainWindow::initSettingsPopover()
{
  // Toolbar buttons / images
  m_zoomOutButton.add(m_zoomOutImage);
  m_zoomInButton.add(m_zoomInImage);
  m_brightnessImage.set_tooltip_text("Brightness");
  m_brightnessImage.set_margin_start(2);
  m_brightnessImage.set_margin_end(2);
  m_brightnessImage.set_margin_top(1);
  m_brightnessImage.set_margin_bottom(1);
  // Zoom buttons
  auto hboxZoomStyleContext = m_hboxSetingsZoom.get_style_context();
  hboxZoomStyleContext->add_class("linked");
  m_zoomRestoreButton.set_sensitive(false); // By default restore button disabled
  m_zoomRestoreButton.set_label("100%");
  m_zoomOutButton.set_tooltip_text("Zoom out");
  m_zoomRestoreButton.set_tooltip_text("Restore zoom");
  m_zoomInButton.set_tooltip_text("Zoom in");
  m_hboxSetingsZoom.set_size_request(-1, 40);
  m_hboxSetingsZoom.set_margin_bottom(6);
  m_hboxSetingsZoom.pack_start(m_zoomOutButton);
  m_hboxSetingsZoom.pack_start(m_zoomRestoreButton);
  m_hboxSetingsZoom.pack_end(m_zoomInButton);
  // Brightness slider
  m_brightnessAdjustment->set_value(brightnessScale_); // Override with current loaded brightness setting
  m_scaleSettingsBrightness.set_adjustment(m_brightnessAdjustment);
  m_scaleSettingsBrightness.add_mark(0.5, Gtk::PositionType::POS_BOTTOM, "");
  m_scaleSettingsBrightness.set_draw_value(false);
  m_scaleSettingsBrightness.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_brightness_changed));
  m_hboxSetingsBrightness.pack_start(m_brightnessImage, false, false);
  m_hboxSetingsBrightness.pack_end(m_scaleSettingsBrightness);
  // Dark theme switch
  m_themeSwitch.set_active(useDarkTheme_); // Override with current dark theme preference
  // Spin buttons
  m_spacingSpinButton.set_adjustment(m_spacingAdjustment);
  m_marginsSpinButton.set_adjustment(m_marginsAdjustment);
  m_indentSpinButton.set_adjustment(m_indentAdjustment);
  m_fontLabel.set_xalign(1);
  m_spacingLabel.set_xalign(1);
  m_marginsLabel.set_xalign(1);
  m_indentLabel.set_xalign(1);
  m_themeLabel.set_xalign(1);
  m_fontLabel.get_style_context()->add_class("dim-label");
  m_spacingLabel.get_style_context()->add_class("dim-label");
  m_marginsLabel.get_style_context()->add_class("dim-label");
  m_indentLabel.get_style_context()->add_class("dim-label");
  m_themeLabel.get_style_context()->add_class("dim-label");
  m_settingsGrid.set_margin_start(6);
  m_settingsGrid.set_margin_top(6);
  m_settingsGrid.set_margin_bottom(6);
  m_settingsGrid.set_row_spacing(10);
  m_settingsGrid.set_column_spacing(10);
  m_settingsGrid.attach(m_fontLabel, 0, 0);
  m_settingsGrid.attach(m_fontButton, 1, 0);
  m_settingsGrid.attach(m_spacingLabel, 0, 1);
  m_settingsGrid.attach(m_spacingSpinButton, 1, 1);
  m_settingsGrid.attach(m_marginsLabel, 0, 2);
  m_settingsGrid.attach(m_marginsSpinButton, 1, 2);
  m_settingsGrid.attach(m_indentLabel, 0, 3);
  m_settingsGrid.attach(m_indentSpinButton, 1, 3);
  m_settingsGrid.attach(m_themeLabel, 0, 4);
  m_settingsGrid.attach(m_themeSwitch, 1, 4);
  // Icon theme (+ submenu)
  m_iconThemeButton.set_label("Icon Theme");
  m_iconThemeButton.property_menu_name() = "icon-theme";
  m_aboutButton.set_label("About LibreWeb");
  Gtk::Label* iconThemeButtonlabel = dynamic_cast<Gtk::Label*>(m_iconThemeButton.get_child());
  iconThemeButtonlabel->set_xalign(0.0);
  Gtk::Label* aboutButtonLabel = dynamic_cast<Gtk::Label*>(m_aboutButton.get_child());
  iconThemeButtonlabel->set_xalign(0.0);
  aboutButtonLabel->set_xalign(0.0);
  // Add Settings vbox to popover menu
  m_vboxSettings.set_margin_start(10);
  m_vboxSettings.set_margin_end(10);
  m_vboxSettings.set_margin_top(10);
  m_vboxSettings.set_margin_bottom(10);
  m_vboxSettings.set_spacing(8);
  m_vboxSettings.add(m_hboxSetingsZoom);
  m_vboxSettings.add(m_hboxSetingsBrightness);
  m_vboxSettings.add(m_separator5);
  m_vboxSettings.add(m_settingsGrid);
  m_vboxSettings.add(m_separator6);
  m_vboxSettings.add(m_iconThemeButton);
  m_vboxSettings.add(m_separator7);
  m_vboxSettings.pack_end(m_aboutButton, false, false);
  m_settingsPopover.set_position(Gtk::POS_BOTTOM);
  m_settingsPopover.set_size_request(200, 300);
  m_settingsPopover.set_margin_end(2);
  m_settingsPopover.add(m_vboxSettings);
  // Add Theme vbox to popover menu
  m_iconThemeBackButton.set_label("Icon Theme");
  m_iconThemeBackButton.property_menu_name() = "main";
  m_iconThemeBackButton.property_inverted() = true;
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
  m_iconThemeListBox.add(*row1);
  m_iconThemeListBox.add(*row2);
  m_iconThemeListBox.add(*row3);
  m_iconThemeListBox.select_row(*row1); // TODO: Select the correct theme on loading
  m_iconThemeListScrolledWindow.property_height_request() = 200;
  m_iconThemeListScrolledWindow.add(m_iconThemeListBox);
  m_iconThemeLabel.get_style_context()->add_class("dim-label");
  m_vboxIconTheme.add(m_iconThemeBackButton);
  m_vboxIconTheme.add(m_separator8);
  m_vboxIconTheme.add(m_iconThemeLabel);
  m_vboxIconTheme.add(m_iconThemeListScrolledWindow);
  m_settingsPopover.add(m_vboxIconTheme);
  m_settingsPopover.child_property_submenu(m_vboxIconTheme) = "icon-theme";
  m_settingsPopover.show_all_children();
}

/**
 * \brief Init all signals and connect them to functions
 */
void MainWindow::initSignals()
{
  // Window signals
  signal_delete_event().connect(sigc::mem_fun(this, &MainWindow::delete_window));
  // Table of contents
  tocTreeView.signal_row_activated().connect(sigc::mem_fun(this, &MainWindow::on_toc_row_activated));
  // Menu & toolbar signals
  m_menu.new_doc.connect(sigc::mem_fun(this, &MainWindow::new_doc));                       /*!< Menu item for new document */
  m_menu.open.connect(sigc::mem_fun(this, &MainWindow::open));                             /*!< Menu item for opening existing document */
  m_menu.open_edit.connect(sigc::mem_fun(this, &MainWindow::open_and_edit));               /*!< Menu item for opening & editing existing document */
  m_menu.edit.connect(sigc::mem_fun(this, &MainWindow::edit));                             /*!< Menu item for editing current open document */
  m_menu.save.connect(sigc::mem_fun(this, &MainWindow::save));                             /*!< Menu item for save document */
  m_menu.save_as.connect(sigc::mem_fun(this, &MainWindow::save_as));                       /*!< Menu item for save document as */
  m_menu.publish.connect(sigc::mem_fun(this, &MainWindow::publish));                       /*!< Menu item for publishing */
  m_menu.quit.connect(sigc::mem_fun(this, &MainWindow::close));                            /*!< close main window and therefor closes the app */
  m_menu.undo.connect(sigc::mem_fun(m_draw_main, &Draw::undo));                            /*!< Menu item for undo text */
  m_menu.redo.connect(sigc::mem_fun(m_draw_main, &Draw::redo));                            /*!< Menu item for redo text */
  m_menu.cut.connect(sigc::mem_fun(this, &MainWindow::cut));                               /*!< Menu item for cut text */
  m_menu.copy.connect(sigc::mem_fun(this, &MainWindow::copy));                             /*!< Menu item for copy text */
  m_menu.paste.connect(sigc::mem_fun(this, &MainWindow::paste));                           /*!< Menu item for paste text */
  m_menu.del.connect(sigc::mem_fun(this, &MainWindow::del));                               /*!< Menu item for deleting selected text */
  m_menu.select_all.connect(sigc::mem_fun(this, &MainWindow::selectAll));                  /*!< Menu item for selecting all text */
  m_menu.find.connect(sigc::bind(sigc::mem_fun(this, &MainWindow::show_search), false));   /*!< Menu item for finding text */
  m_menu.replace.connect(sigc::bind(sigc::mem_fun(this, &MainWindow::show_search), true)); /*!< Menu item for replacing text */
  m_menu.back.connect(sigc::mem_fun(this, &MainWindow::back));                             /*!< Menu item for previous page */
  m_menu.forward.connect(sigc::mem_fun(this, &MainWindow::forward));                       /*!< Menu item for next page */
  m_menu.reload.connect(sigc::mem_fun(this, &MainWindow::refreshRequest));                 /*!< Menu item for reloading the page */
  m_menu.home.connect(sigc::mem_fun(this, &MainWindow::go_home));                          /*!< Menu item for home page */
  m_menu.toc.connect(sigc::mem_fun(this, &MainWindow::show_toc));                          /*!< Menu item for table of contents */
  m_menu.source_code.connect(sigc::mem_fun(this, &MainWindow::show_source_code_dialog));   /*!< Source code dialog */
  m_sourceCodeDialog.signal_response().connect(sigc::mem_fun(m_sourceCodeDialog, &SourceCodeDialog::hide_dialog)); /*!< Close source code dialog */
  m_menu.about.connect(sigc::mem_fun(m_about, &About::show_about));                                                /*!< Display about dialog */
  m_draw_main.source_code.connect(sigc::mem_fun(this, &MainWindow::show_source_code_dialog));                      /*!< Open source code dialog */
  m_about.signal_response().connect(sigc::mem_fun(m_about, &About::hide_about));                                   /*!< Close about dialog */
  m_addressBar.signal_activate().connect(sigc::mem_fun(this, &MainWindow::address_bar_activate)); /*!< User pressed enter the address bar */
  m_tocButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::show_toc));               /*!< Button for showing Table of Contents */
  m_backButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::back));                  /*!< Button for previous page */
  m_forwardButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::forward));            /*!< Button for next page */
  m_refreshButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::refreshRequest));     /*!< Button for reloading the page */
  m_homeButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::go_home));               /*!< Button for home page */
  m_searchEntry.signal_activate().connect(sigc::mem_fun(this, &MainWindow::on_search));           /*!< Execute the text search */
  m_searchReplaceEntry.signal_activate().connect(sigc::mem_fun(this, &MainWindow::on_replace));   /*!< Execute the text replace */
  // Editor toolbar buttons
  m_openButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::open_and_edit));
  m_saveButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::save));
  m_publishButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::publish));
  m_cutButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::cut));
  m_copyButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::copy));
  m_pasteButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::paste));
  m_undoButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::undo));
  m_redoButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::redo));
  m_headingsComboBox.signal_changed().connect(sigc::mem_fun(this, &MainWindow::get_heading));
  m_boldButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::make_bold));
  m_italicButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::make_italic));
  m_strikethroughButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::make_strikethrough));
  m_superButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::make_super));
  m_subButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::make_sub));
  m_linkButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::insert_link));
  m_imageButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::insert_image));
  m_emojiButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::insert_emoji));
  m_quoteButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::make_quote));
  m_codeButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::make_code));
  m_bulletListButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::insert_bullet_list));
  m_numberedListButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::insert_numbered_list));
  m_highlightButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::make_highlight));
  // Status pop-over buttons
  m_copyIDButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::copy_client_id));
  m_copyPublicKeyButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::copy_client_public_key));
  // Settings pop-over buttons
  m_zoomOutButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::on_zoom_out));
  m_zoomRestoreButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::on_zoom_restore));
  m_zoomInButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::on_zoom_in));
  m_fontButton.signal_font_set().connect(sigc::mem_fun(this, &MainWindow::on_font_set));
  m_spacingSpinButton.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_spacing_changed));
  m_marginsSpinButton.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_margins_changed));
  m_indentSpinButton.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_indent_changed));
  m_themeSwitch.property_active().signal_changed().connect(sigc::mem_fun(this, &MainWindow::on_theme_changed));
  m_iconThemeListBox.signal_row_activated().connect(sigc::mem_fun(this, &MainWindow::on_icon_theme_activated));
  m_aboutButton.signal_clicked().connect(sigc::mem_fun(m_about, &About::show_about));
}

/**
 * \brief Called when Window is closed/exited
 */
bool MainWindow::delete_window(GdkEventAny* any_event __attribute__((unused)))
{
  if (m_settings)
  {
    // Save the schema settings
    m_settings->set_int("width", get_width());
    m_settings->set_int("height", get_height());
    m_settings->set_boolean("maximized", is_maximized());
    if (m_panedRoot.get_position() > 0)
      m_settings->set_int("position-divider-toc", m_panedRoot.get_position());
    // Only store a divider value bigger than zero,
    // because the secondary draw window is hidden by default, resulting into a zero value.
    if (m_panedDraw.get_position() > 0)
      m_settings->set_int("position-divider-draw", m_panedDraw.get_position());
    // Fullscreen will be availible with gtkmm-4.0
    // m_settings->set_boolean("fullscreen", is_fullscreen());
    m_settings->set_string("font-family", fontFamily_);
    m_settings->set_int("font-size", currentFontSize_);
    m_settings->set_int("spacing", m_spacingSpinButton.get_value_as_int());
    m_settings->set_int("margins", m_marginsSpinButton.get_value_as_int());
    m_settings->set_int("indent", m_indentSpinButton.get_value_as_int());
    m_settings->set_string("icon-theme", iconTheme_);
    m_settings->set_boolean("icon-gtk-theme", useCurrentGTKIconTheme_);
    m_settings->set_double("brightness", brightnessScale_);
    m_settings->set_boolean("dark-theme", useDarkTheme_);
  }
  return false;
}

/**
 * \brief Cut/copy/paste/delete/select all keybindings
 */
void MainWindow::cut()
{
  if (m_draw_main.has_focus())
  {
    m_draw_main.cut();
  }
  else if (m_draw_secondary.has_focus())
  {
    m_draw_secondary.cut();
  }
  else if (m_addressBar.has_focus())
  {
    m_addressBar.cut_clipboard();
  }
  else if (m_searchEntry.has_focus())
  {
    m_searchEntry.cut_clipboard();
  }
  else if (m_searchReplaceEntry.has_focus())
  {
    m_searchReplaceEntry.cut_clipboard();
  }
}

void MainWindow::copy()
{
  if (m_draw_main.has_focus())
  {
    m_draw_main.copy();
  }
  else if (m_draw_secondary.has_focus())
  {
    m_draw_secondary.copy();
  }
  else if (m_addressBar.has_focus())
  {
    m_addressBar.copy_clipboard();
  }
  else if (m_searchEntry.has_focus())
  {
    m_searchEntry.copy_clipboard();
  }
  else if (m_searchReplaceEntry.has_focus())
  {
    m_searchReplaceEntry.copy_clipboard();
  }
}

void MainWindow::paste()
{
  if (m_draw_main.has_focus())
  {
    m_draw_main.paste();
  }
  else if (m_draw_secondary.has_focus())
  {
    m_draw_secondary.paste();
  }
  else if (m_addressBar.has_focus())
  {
    m_addressBar.paste_clipboard();
  }
  else if (m_searchEntry.has_focus())
  {
    m_searchEntry.paste_clipboard();
  }
  else if (m_searchReplaceEntry.has_focus())
  {
    m_searchReplaceEntry.paste_clipboard();
  }
}

void MainWindow::del()
{
  if (m_draw_main.has_focus())
  {
    m_draw_main.del();
  }
  else if (m_draw_secondary.has_focus())
  {
    m_draw_secondary.del();
  }
  else if (m_addressBar.has_focus())
  {
    int start, end;
    if (m_addressBar.get_selection_bounds(start, end))
    {
      m_addressBar.delete_text(start, end);
    }
    else
    {
      ++end;
      m_addressBar.delete_text(start, end);
    }
  }
  else if (m_searchEntry.has_focus())
  {
    int start, end;
    if (m_searchEntry.get_selection_bounds(start, end))
    {
      m_searchEntry.delete_text(start, end);
    }
    else
    {
      ++end;
      m_searchEntry.delete_text(start, end);
    }
  }
  else if (m_searchReplaceEntry.has_focus())
  {
    int start, end;
    if (m_searchReplaceEntry.get_selection_bounds(start, end))
    {
      m_searchReplaceEntry.delete_text(start, end);
    }
    else
    {
      ++end;
      m_searchReplaceEntry.delete_text(start, end);
    }
  }
}

void MainWindow::selectAll()
{
  if (m_draw_main.has_focus())
  {
    m_draw_main.selectAll();
  }
  else if (m_draw_secondary.has_focus())
  {
    m_draw_secondary.selectAll();
  }
  else if (m_addressBar.has_focus())
  {
    m_addressBar.select_region(0, -1);
  }
  else if (m_searchEntry.has_focus())
  {
    m_searchEntry.select_region(0, -1);
  }
  else if (m_searchReplaceEntry.has_focus())
  {
    m_searchReplaceEntry.select_region(0, -1);
  }
}

/**
 * \brief Triggered when user clicked on the column in TOC
 */
void MainWindow::on_toc_row_activated(const Gtk::TreeModel::Path& path, __attribute__((unused)) Gtk::TreeViewColumn* column)
{
  const auto iter = m_tocTreeModel->get_iter(path);
  if (iter)
  {
    const auto row = *iter;
    std::cout << "Row activated: ID=" << row[m_tocColumns.m_col_id] << ", Name=" << row[m_tocColumns.m_col_heading] << std::endl;
  }
}

/**
 * \brief Trigger when user selected 'new document' from menu item
 */
void MainWindow::new_doc()
{
  // Clear content & path
  middleware_.resetContentAndPath();
  // Enable editing mode
  enableEdit();
  // Change address bar
  m_addressBar.set_text("file://unsaved");
  // Set new title
  set_title("Untitled * - " + appName_);
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
    middleware_.doRequest("file://" + filePath);
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
    if (!isEditorEnabled())
      enableEdit();

    auto filePath = dialog->get_file()->get_path();
    std::string path = "file://" + filePath;
    // Open file and set address bar, but do not parse the content or the disable editor
    middleware_.doRequest(path, true, false, false, false);
    // Set current file path for the 'save' feature
    currentFileSavedPath_ = filePath;
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
  if (!isEditorEnabled())
    enableEdit();

  m_draw_main.setText(middleware_.getContent());
  // Set title
  set_title("Untitled * - " + appName_);
}

/**
 * \brief Triggered when user selected 'save' from menu item / toolbar
 */
void MainWindow::save()
{
  if (currentFileSavedPath_.empty())
  {
    save_as();
  }
  else
  {
    if (isEditorEnabled())
    {
      try
      {
        middleware_.doWrite(currentFileSavedPath_);
      }
      catch (std::ios_base::failure& error)
      {
        std::cerr << "ERROR: Could not write file: " << currentFileSavedPath_ << ". Message: " << error.what() << ".\nError code: " << error.code()
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
  if (!currentFileSavedPath_.empty())
  {
    try
    {
      dialog->set_uri(Glib::filename_to_uri(currentFileSavedPath_));
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
      middleware_.doWrite(filePath, isEditorEnabled()); // Only update address & title, when editor mode is enabled
      // Only if editor mode is enabled
      if (isEditorEnabled())
      {
        // Set/update the current file saved path variable (used for the 'save' feature)
        currentFileSavedPath_ = filePath;
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
  if (middleware_.getContent().empty())
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
    if (!currentFileSavedPath_.empty())
    {
      path = currentFileSavedPath_;
    }
    else
    {
      // TODO: path is not defined yet. however, this may change anyway once we try to build more complex
      // websites, needing to use directory structures.
    }

    try
    {
      // Add content to IPFS
      std::string cid = middleware_.doAdd(path);
      if (cid.empty())
      {
        throw std::runtime_error("CID hash is empty.");
      }
      // Show dialog
      m_contentPublishedDialog.reset(new Gtk::MessageDialog(*this, "File is successfully added to IPFS!"));
      m_contentPublishedDialog->set_secondary_text("The content is now available on the decentralized web, via:");
      // Add custom label
      Gtk::Label* label = Gtk::manage(new Gtk::Label("ipfs://" + cid));
      label->set_selectable(true);
      Gtk::Box* box = m_contentPublishedDialog->get_content_area();
      box->pack_end(*label);

      m_contentPublishedDialog->set_modal(true);
      // m_contentPublishedDialog->set_hide_on_close(true); available in gtk-4.0
      m_contentPublishedDialog->signal_response().connect(sigc::hide(sigc::mem_fun(*m_contentPublishedDialog, &Gtk::Widget::hide)));
      m_contentPublishedDialog->show_all();
    }
    catch (const std::runtime_error& error)
    {
      m_contentPublishedDialog.reset(new Gtk::MessageDialog(*this, "File could not be added to IPFS", false, Gtk::MESSAGE_ERROR));
      m_contentPublishedDialog->set_secondary_text("Error message: " + std::string(error.what()));
      m_contentPublishedDialog->set_modal(true);
      // m_contentPublishedDialog->set_hide_on_close(true); available in gtk-4.0
      m_contentPublishedDialog->signal_response().connect(sigc::hide(sigc::mem_fun(*m_contentPublishedDialog, &Gtk::Widget::hide)));
      m_contentPublishedDialog->show();
    }
  }
}

/**
 * \brief Show homepage
 */
void MainWindow::go_home()
{
  middleware_.doRequest("about:home", true, false, true);
}

/**
 * \brief Show/hide table of contents
 */
void MainWindow::show_toc()
{
  if (m_scrolledToc.is_visible())
    m_scrolledToc.hide();
  else
    m_scrolledToc.show();
}

/**
 * \brief Copy the IPFS Client ID to clipboard
 */
void MainWindow::copy_client_id()
{
  if (!middleware_.getIPFSClientId().empty())
  {
    get_clipboard("CLIPBOARD")->set_text(middleware_.getIPFSClientId());
    showNotification("Copied to clipboard", "Your client ID is now copied to your clipboard.");
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
  if (!middleware_.getIPFSClientPublicKey().empty())
  {
    get_clipboard("CLIPBOARD")->set_text(middleware_.getIPFSClientPublicKey());
    showNotification("Copied to clipboard", "Your client public key is now copied to your clipboard.");
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
  std::string text = m_searchEntry.get_text();
  auto buffer = m_draw_main.get_buffer();
  Gtk::TextBuffer::iterator iter = buffer->get_iter_at_mark(buffer->get_mark("insert"));
  Gtk::TextBuffer::iterator start, end;
  bool matchCase = m_searchMatchCase.get_active();
  Gtk::TextSearchFlags flags = Gtk::TextSearchFlags::TEXT_SEARCH_TEXT_ONLY;
  if (!matchCase)
  {
    flags |= Gtk::TextSearchFlags::TEXT_SEARCH_CASE_INSENSITIVE;
  }
  if (iter.forward_search(text, flags, start, end))
  {
    buffer->select_range(end, start);
    m_draw_main.scroll_to(start);
  }
  else
  {
    buffer->place_cursor(buffer->begin());
    // Try another search directly from the top
    Gtk::TextBuffer::iterator secondIter = buffer->get_iter_at_mark(buffer->get_mark("insert"));
    if (secondIter.forward_search(text, flags, start, end))
    {
      buffer->select_range(end, start);
      m_draw_main.scroll_to(start);
    }
  }
}

/**
 * \brief Trigger when user pressed enter in the replace entry
 */
void MainWindow::on_replace()
{
  if (m_draw_main.get_editable())
  {
    auto buffer = m_draw_main.get_buffer();
    Gtk::TextBuffer::iterator startIter = buffer->get_iter_at_mark(buffer->get_mark("insert"));
    Gtk::TextBuffer::iterator endIter = buffer->get_iter_at_mark(buffer->get_mark("selection_bound"));
    if (startIter != endIter)
    {
      // replace
      std::string replace = m_searchReplaceEntry.get_text();
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
  middleware_.doRequest(m_addressBar.get_text(), false);
  // When user actually entered the address bar, focus on the main draw
  m_draw_main.grab_focus();
}

/**
 * \brief Triggers when user tries to search or replace text
 */
void MainWindow::show_search(bool replace)
{
  if (m_searchPopover.is_visible() && m_searchReplaceEntry.is_visible())
  {
    if (replace)
    {
      m_searchPopover.hide();
      m_addressBar.grab_focus();
      m_searchReplaceEntry.hide();
    }
    else
    {
      m_searchEntry.grab_focus();
      m_searchReplaceEntry.hide();
    }
  }
  else if (m_searchPopover.is_visible())
  {
    if (replace)
    {
      m_searchReplaceEntry.show();
    }
    else
    {
      m_searchPopover.hide();
      m_addressBar.grab_focus();
      m_searchReplaceEntry.hide();
    }
  }
  else
  {
    m_searchPopover.show();
    m_searchEntry.grab_focus();
    if (replace)
    {
      m_searchReplaceEntry.show();
    }
    else
    {
      m_searchReplaceEntry.hide();
    }
  }
}

void MainWindow::back()
{
  if (currentHistoryIndex_ > 0)
  {
    currentHistoryIndex_--;
    middleware_.doRequest(history_.at(currentHistoryIndex_), true, true);
  }
}

void MainWindow::forward()
{
  if (currentHistoryIndex_ < history_.size() - 1)
  {
    currentHistoryIndex_++;
    middleware_.doRequest(history_.at(currentHistoryIndex_), true, true);
  }
}

/**
 * \brief Determing if browser is installed to the installation directory at runtime
 * \return true if the current running process is installed (to the installed prefix path)
 */
bool MainWindow::isInstalled()
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
      bool isInstalled = true;
      wai_getExecutablePath(path, length, NULL);
      path[length] = '\0';
#ifdef _WIN32
      // Does the executable path starts with C:\Program?
      const char* windowsPrefix = "C:\\Program";
      isInstalled = (strncmp(path, windowsPrefix, strlen(windowsPrefix)) == 0);
#else
      // Does the executable path starts with /usr/local?
      isInstalled = (strncmp(path, INSTALL_PREFIX, strlen(INSTALL_PREFIX)) == 0);
#endif
      free(path);
      return isInstalled;
    }
  }
  return true; // fallback; assume always installed
}

/**
 * \brief Enable editor mode. Allowing to create or edit existing documents.
 */
void MainWindow::enableEdit()
{
  // Inform the Draw class that we are creating a new document
  m_draw_main.newDocument();
  // Show editor toolbars
  m_hboxStandardEditorToolbar.show();
  m_hboxFormattingEditorToolbar.show();
  // Determine position of divider between the primary and secondary windows
  int location = 0;
  int positionSettings = 42;
  if (m_settings)
    positionSettings = m_settings->get_int("position-divider-draw");
  int currentWidth, _ = 0;
  get_size(currentWidth, _);
  // If position from settings is still default (42) or too big,
  // let's calculate the paned divider location
  if ((positionSettings == 42) || (positionSettings >= (currentWidth - 10)))
  {
    location = static_cast<int>(currentWidth / 2.0);
  }
  else
  {
    location = positionSettings;
  }
  m_panedDraw.set_position(location);

  // Disable Table of Contents during edit
  m_scrolledToc.hide();
  // Enabled secondary text view (on the right)
  m_scrolledWindowSecondary.show();
  // Disable "view source" menu item
  m_draw_main.setViewSourceMenuItem(false);
  // Connect changed signal
  textChangedSignalHandler_ = m_draw_main.get_buffer().get()->signal_changed().connect(sigc::mem_fun(this, &MainWindow::editor_changed_text));
  // Enable publish menu item
  m_menu.setPublishMenuSensitive(true);
  // Disable edit menu item (you are already editing)
  m_menu.setEditMenuSensitive(false);
  // Just to be sure, disable the spinning animation
  m_refreshIcon.get_style_context()->remove_class("spinning");
}

/**
 * \brief Disable editor mode.
 */
void MainWindow::disableEdit()
{
  if (isEditorEnabled())
  {
    m_hboxStandardEditorToolbar.hide();
    m_hboxFormattingEditorToolbar.hide();
    m_scrolledWindowSecondary.hide();
    // Disconnect text changed signal
    textChangedSignalHandler_.disconnect();
    // Show "view source" menu item again
    m_draw_main.setViewSourceMenuItem(true);
    m_draw_secondary.clearText();
    // Disable publish menu item
    m_menu.setPublishMenuSensitive(false);
    // Enable edit menu item
    m_menu.setEditMenuSensitive(true);
    // Empty current file saved path
    currentFileSavedPath_ = "";
  }
}

/**
 * \brief Check if editor is enabled
 * \return true if enabled, otherwise false
 */
bool MainWindow::isEditorEnabled()
{
  return m_hboxStandardEditorToolbar.is_visible();
}

/**
 * \brief Retrieve image path from icon theme location
 * \param iconName Icon name (.png is added default)
 * \param typeofIcon Type of the icon is the sub-folder within the icons directory (eg. "editor", "arrows" or "basic")
 * \return full path of the icon PNG image
 */
std::string MainWindow::getIconImageFromTheme(const std::string& iconName, const std::string& typeofIcon)
{
  // Use data directory first, used when LibreWeb is installed (Linux or Windows)
  for (std::string data_dir : Glib::get_system_data_dirs())
  {
    std::vector<std::string> path_builder{data_dir, "libreweb", "images", "icons", iconTheme_, typeofIcon, iconName + ".png"};
    std::string file_path = Glib::build_path(G_DIR_SEPARATOR_S, path_builder);
    if (Glib::file_test(file_path, Glib::FileTest::FILE_TEST_IS_REGULAR))
    {
      return file_path;
    }
  }

  // Try local path if the images are not (yet) installed
  // When working directory is in the build/bin folder (relative path)
  std::vector<std::string> path_builder{"..", "..", "images", "icons", iconTheme_, typeofIcon, iconName + ".png"};
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
 * \brief Update the CSS provider data on the main draw text view
 */
void MainWindow::updateCSS()
{
  std::string colorCss;
  double darknessScale = (1.0 - brightnessScale_);
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

  try
  {
    m_drawCSSProvider->load_from_data("textview { "
                                      "font-family: \"" +
                                      fontFamily_ +
                                      "\";"
                                      "font-size: " +
                                      std::to_string(currentFontSize_) + "pt; }" + "textview text { " + colorCss + "background-color: rgba(0, 0, 0," +
                                      darknessStr +
                                      ");"
                                      "letter-spacing: " +
                                      std::to_string(fontSpacing_) + "px; }");
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
void MainWindow::showNotification(const Glib::ustring& title, const Glib::ustring& message)
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

  // Retrieve text from editor and parse the markdown contents
  middleware_.setContent(m_draw_main.getText());
  cmark_node* doc = middleware_.parseContent();
  /* // Can be enabled to show the markdown format in terminal:
  std::string md = Parser::renderMarkdown(doc);
  std::cout << "Markdown:\n" << md << std::endl;*/
  // Show the document as a preview on the right side text-view panel
  m_draw_secondary.setDocument(doc);
}

/**
 * \brief Show source code dialog window with the current content
 */
void MainWindow::show_source_code_dialog()
{
  m_sourceCodeDialog.setText(middleware_.getContent());
  m_sourceCodeDialog.run();
}

/**
 * \brief Retrieve selected heading from combobox.
 * Send to main Draw class
 */
void MainWindow::get_heading()
{
  std::string active = m_headingsComboBox.get_active_id();
  m_headingsComboBox.set_active(0); // Reset
  if (active != "")
  {
    std::string::size_type sz;
    try
    {
      int headingLevel = std::stoi(active, &sz, 10);
      m_draw_main.make_heading(headingLevel);
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
  g_signal_emit_by_name(m_draw_main.gobj(), "insert-emoji");
}

void MainWindow::on_zoom_out()
{
  currentFontSize_ -= 1;
  updateCSS();
  m_zoomRestoreButton.set_sensitive(currentFontSize_ != defaultFontSize_);
}

void MainWindow::on_zoom_restore()
{
  currentFontSize_ = defaultFontSize_; // reset
  updateCSS();
  m_zoomRestoreButton.set_sensitive(false);
}

void MainWindow::on_zoom_in()
{
  currentFontSize_ += 1;
  updateCSS();
  m_zoomRestoreButton.set_sensitive(currentFontSize_ != defaultFontSize_);
}

void MainWindow::on_font_set()
{
  Pango::FontDescription fontDesc = Pango::FontDescription(m_fontButton.get_font_name());
  fontFamily_ = fontDesc.get_family();
  currentFontSize_ = defaultFontSize_ = (fontDesc.get_size_is_absolute()) ? fontDesc.get_size() : fontDesc.get_size() / PANGO_SCALE;
  updateCSS();
}

void MainWindow::on_spacing_changed()
{
  fontSpacing_ = m_spacingSpinButton.get_value_as_int(); // Letter spacing
  updateCSS();
}

void MainWindow::on_margins_changed()
{
  m_draw_main.set_left_margin(m_marginsSpinButton.get_value_as_int());
  m_draw_main.set_right_margin(m_marginsSpinButton.get_value_as_int());
}

void MainWindow::on_indent_changed()
{
  m_draw_main.set_indent(m_indentSpinButton.get_value_as_int());
}

void MainWindow::on_brightness_changed()
{
  brightnessScale_ = m_scaleSettingsBrightness.get_value();
  updateCSS();
}

void MainWindow::on_theme_changed()
{
  // Switch between dark or light theme preference
  useDarkTheme_ = m_themeSwitch.get_active();
  setTheme();
}

void MainWindow::on_icon_theme_activated(Gtk::ListBoxRow* row)
{
  std::string themeName = static_cast<char*>(row->get_data("value"));
  if (themeName != "none")
  {
    iconTheme_ = themeName;
    useCurrentGTKIconTheme_ = false;
  }
  else
  {
    useCurrentGTKIconTheme_ = true;
  }
  // Reload icons
  loadIcons();
  // Trigger IPFS status icon
  updateStatusPopoverAndIcon();
}
