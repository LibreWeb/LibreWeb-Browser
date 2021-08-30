#include "mainwindow.h"

#include "project_config.h"
#include "md-parser.h"
#include "menu.h"
#include "file.h"
#include <gtkmm/menuitem.h>
#include <gtkmm/image.h>
#include <gtkmm/listboxrow.h>
#include <giomm/file.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <glibmm/main.h>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cmark-gfm.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

MainWindow::MainWindow(const std::string &timeout)
    : m_accelGroup(Gtk::AccelGroup::create()),
      m_settings(),
      m_brightnessAdjustment(Gtk::Adjustment::create(1.0, 0.5, 1.5, 0.05, 0.1)),
      m_spacingAdjustment(Gtk::Adjustment::create(0, -10, 10, 1, 2)),
      m_marginsAdjustment(Gtk::Adjustment::create(20, 0, 1000, 10, 20)),
      m_indentAdjustment(Gtk::Adjustment::create(0, 0, 1000, 5, 10)),
      m_drawCSSProvider(Gtk::CssProvider::create()),
      m_menu(m_accelGroup),
      m_draw_main(*this),
      m_draw_secondary(*this),
      m_about(*this),
      m_vbox(Gtk::ORIENTATION_VERTICAL, 0),
      m_vboxStatus(Gtk::ORIENTATION_VERTICAL),
      m_vboxSettings(Gtk::ORIENTATION_VERTICAL),
      m_vboxIconTheme(Gtk::ORIENTATION_VERTICAL),
      m_searchMatchCase("Match _Case", true),
      m_copyIDButton("Copy your ID"),
      m_copyPublicKeyButton("Copy Public Key"),
      m_fontLabel("Font"),
      m_spacingLabel("Spacing"),
      m_marginsLabel("Margins"),
      m_indentLabel("Indent"),
      m_iconThemeLabel("Active theme"),
      m_appName("LibreWeb Browser"),
      m_iconTheme("flat"),             // default is flat theme
      m_useCurrentGTKIconTheme(false), // Use our built-in icon theme or the GTK icons
      m_iconSize(18),
      m_fontSpacing(0),
      m_requestThread(nullptr),
      currentHistoryIndex(0),
      m_waitPageVisible(false),
      ipfsHost("localhost"),
      ipfsPort(5001),
      ipfsTimeout(timeout),
      ipfs(ipfsHost, ipfsPort, ipfsTimeout) // Create IPFS object
{
    set_title(m_appName);
    set_default_size(1000, 800);
    set_position(Gtk::WIN_POS_CENTER);
    add_accel_group(m_accelGroup);

    loadStoredSettings();
    loadIcons();
    initButtons();
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

    // Browser text main drawing area
    m_scrolledWindowMain.add(m_draw_main);
    m_scrolledWindowMain.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    // Secondary drawing area
    m_draw_secondary.setViewSourceMenuItem(false);
    m_scrolledWindowSecondary.add(m_draw_secondary);
    m_scrolledWindowSecondary.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    // Bottom Search bar
    m_search.connect_entry(m_searchEntry);
    m_searchReplace.connect_entry(m_searchReplaceEntry);
    m_exitBottomIcon.set_from_icon_name("window-close-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_BUTTON));
    m_exitBottomButton.set_relief(Gtk::RELIEF_NONE);
    m_exitBottomButton.add(m_exitBottomIcon);
    m_exitBottomButton.signal_clicked().connect(sigc::mem_fun(m_hboxBottom, &Gtk::Box::hide));
    m_searchEntry.set_size_request(250, -1);
    m_searchReplaceEntry.set_size_request(250, -1);
    m_searchEntry.set_margin_top(3);
    m_searchEntry.set_margin_bottom(3);
    m_searchReplaceEntry.set_margin_top(3);
    m_searchReplaceEntry.set_margin_bottom(3);
    m_searchMatchCase.set_margin_top(3);
    m_searchMatchCase.set_margin_bottom(3);
    m_hboxBottom.set_margin_start(6);
    m_hboxBottom.set_spacing(8);
    m_hboxBottom.pack_start(m_exitBottomButton, false, false);
    m_hboxBottom.pack_start(m_searchEntry, false, false);
    m_hboxBottom.pack_start(m_searchReplaceEntry, false, false);
    m_hboxBottom.pack_start(m_searchMatchCase, false, false);

    m_paned.pack1(m_scrolledWindowMain, true, false);
    m_paned.pack2(m_scrolledWindowSecondary, true, true);

    m_vbox.pack_start(m_menu, false, false, 0);
    m_vbox.pack_start(m_hboxBrowserToolbar, false, false, 6);
    m_vbox.pack_start(m_hboxStandardEditorToolbar, false, false, 6);
    m_vbox.pack_start(m_hboxFormattingEditorToolbar, false, false, 6);
    m_vbox.pack_start(m_paned, true, true, 0);
    m_vbox.pack_end(m_hboxBottom, false, true, 6);

    add(m_vbox);
    show_all_children();

    // Hide by default the bottom box + replace entry, editor box & secondary text view
    m_hboxBottom.hide();
    m_searchReplaceEntry.hide();
    m_hboxStandardEditorToolbar.hide();
    m_hboxFormattingEditorToolbar.hide();
    m_scrolledWindowSecondary.hide();

    // Grap focus to input field by default
    m_addressBar.grab_focus();

    // First time manually trigger the status update once,
    // timer will do the updates later
    this->update_connection_status();

// Show homepage if debugging is disabled
#ifdef NDEBUG
    go_home();
#else
    std::cout << "INFO: Running as Debug mode, opening test.md." << std::endl;
    // Load test file when developing
    doRequest("file://../../test.md");
#endif
}

/**
 * Load stored settings from GSettings scheme file
 */
void MainWindow::loadStoredSettings()
{
    // Change schema directory when browser is not installed
    if (!this->isInstalled())
    {
        std::string schemaDir = std::string(BINARY_DIR) + "/gsettings";
        std::cout << "INFO: Use settings from: " << schemaDir << std::endl;
        Glib::setenv("GSETTINGS_SCHEMA_DIR", schemaDir);
    }
    // Load schema settings file
    m_settings = Gio::Settings::create("org.libreweb.browser");
    // Apply global settings
    set_default_size(m_settings->get_int("width"), m_settings->get_int("height"));
    if (m_settings->get_boolean("maximized"))
        this->maximize();
    
    this->m_fontFamily = m_settings->get_string("font-family");
    this->m_currentFontSize = this->m_defaultFontSize = m_settings->get_int("font-size");
    this->m_fontButton.set_font_name(this->m_fontFamily + " " + std::to_string(this->m_currentFontSize));

    m_fontSpacing = m_settings->get_int("spacing");
    int margins = m_settings->get_int("margins");
    int indent = m_settings->get_int("indent");
    this->m_spacingAdjustment->set_value(m_fontSpacing);
    this->m_marginsAdjustment->set_value(margins);
    this->m_indentAdjustment->set_value(indent);
    this->m_draw_main.set_left_margin(margins);
    this->m_draw_main.set_right_margin(margins);
    this->m_draw_main.set_indent(indent);

    this->m_iconTheme = m_settings->get_string("icon-theme");
    this->m_useCurrentGTKIconTheme = m_settings->get_boolean("icon-gtk-theme");
}

/**
 * Load all icon images from theme/disk. Or reload them.
 * TODO: Reloading images, does this cause any memory leaks or?
 */
void MainWindow::loadIcons()
{
    try
    {
        // Editor buttons
        m_openIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("open_folder", "folders"), m_iconSize, m_iconSize));
        m_saveIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("floppy_disk", "basic"), m_iconSize, m_iconSize));
        m_publishIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("upload", "basic"), m_iconSize, m_iconSize));
        m_cutIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("cut", "editor"), m_iconSize, m_iconSize));
        m_copyIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("copy", "editor"), m_iconSize, m_iconSize));
        m_pasteIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("clipboard", "editor"), m_iconSize, m_iconSize));
        m_undoIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("undo", "editor"), m_iconSize, m_iconSize));
        m_redoIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("redo", "editor"), m_iconSize, m_iconSize));
        m_boldIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("bold", "editor"), m_iconSize, m_iconSize));
        m_italicIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("italic", "editor"), m_iconSize, m_iconSize));
        m_strikethroughIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("strikethrough", "editor"), m_iconSize, m_iconSize));
        m_superIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("superscript", "editor"), m_iconSize, m_iconSize));
        m_subIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("subscript", "editor"), m_iconSize, m_iconSize));
        m_linkIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("link", "editor"), m_iconSize, m_iconSize));
        m_imageIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("shapes", "editor"), m_iconSize, m_iconSize));
        m_emojiIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("smile", "smiley"), m_iconSize, m_iconSize));
        m_quoteIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("quote", "editor"), m_iconSize, m_iconSize));
        m_codeIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("code", "editor"), m_iconSize, m_iconSize));
        m_bulletListIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("bullet_list", "editor"), m_iconSize, m_iconSize));
        m_numberedListIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("number_list", "editor"), m_iconSize, m_iconSize));
        m_hightlightIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("highlighter", "editor"), m_iconSize, m_iconSize));

        if (m_useCurrentGTKIconTheme)
        {
            // Toolbox buttons
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
        else
        {
            // Toolbox buttons
            m_backIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("right_arrow_1", "arrows"), m_iconSize, m_iconSize)->flip());
            m_forwardIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("right_arrow_1", "arrows"), m_iconSize, m_iconSize));
            m_refreshIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("reload_centered", "arrows"), m_iconSize * 1.13, m_iconSize));
            m_homeIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("home", "basic"), m_iconSize, m_iconSize));
            m_searchIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("search", "basic"), m_iconSize, m_iconSize));
            m_settingsIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("menu", "basic"), m_iconSize, m_iconSize));

            // Settings pop-over buttons
            m_zoomOutImage.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("zoom_out", "basic"), m_iconSize, m_iconSize));
            m_zoomInImage.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("zoom_in", "basic"), m_iconSize, m_iconSize));
            m_brightnessImage.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("brightness", "basic"), m_iconSize, m_iconSize));
        }
    }
    catch (const Glib::FileError &error)
    {
        std::cerr << "ERROR: Icon could not be loaded: " << error.what() << ".\nContinue nevertheless..." << std::endl;
    }

    // Load toolbar status icon image separately
    loadStatusIcon();
}

/**
 * Dedicated function for loading the status icon in the toolbar.
 * Because it will require some additional logic. It also returns the number of IPFS peers connected to.
 * 
 * @param reload Reloading pix buffer from disk again (only needed when the theme has changed)
 * @return Number of IPFS peers
 */
std::size_t MainWindow::loadStatusIcon(bool reload)
{
    std::size_t nrPeers = ipfs.getNrPeers();
    try
    {
        if (m_useCurrentGTKIconTheme)
        {
            if (nrPeers > 0) {
                m_statusIcon.set_from_icon_name("network-wired-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
            } else {
                m_statusIcon.set_from_icon_name("network-wired-disconnected-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
            }
        } else { 
            if (reload) {
                m_statusOfflineIcon = Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("network_disconnected", "network"), m_iconSize, m_iconSize);
                m_statusOnlineIcon = Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("network_connected", "network"), m_iconSize, m_iconSize);
            }
            if (nrPeers > 0) {
                m_statusIcon.set(m_statusOnlineIcon);
            } else {
                m_statusIcon.set(m_statusOfflineIcon);
            }
        }
    }
    catch (const Glib::FileError &error)
    {
        std::cerr << "ERROR: Status icon could not be loaded: " << error.what() << ".\nContinue nevertheless..." << std::endl;
    }
    return nrPeers;
}

/**
 * Init all buttons / comboboxes from the toolbars
 */
void MainWindow::initButtons()
{
    // Editor buttons
    m_openButton.set_tooltip_text("Open document (Ctrl+O)");
    m_openButton.add(m_openIcon);
    m_openButton.set_relief(Gtk::RELIEF_NONE);
    m_openButton.set_can_focus(false);
    m_saveButton.set_tooltip_text("Save document (Ctrl+S)");
    m_saveButton.add(m_saveIcon);
    m_saveButton.set_relief(Gtk::RELIEF_NONE);
    m_saveButton.set_can_focus(false);
    m_publishButton.set_tooltip_text("Publish document... (Ctrl+P)");
    m_publishButton.add(m_publishIcon);
    m_publishButton.set_relief(Gtk::RELIEF_NONE);
    m_publishButton.set_can_focus(false);
    m_cutButton.set_tooltip_text("Cut (Ctrl+X)");
    m_cutButton.add(m_cutIcon);
    m_cutButton.set_relief(Gtk::RELIEF_NONE);
    m_cutButton.set_can_focus(false);
    m_copyButton.set_tooltip_text("Copy (Ctrl+C)");
    m_copyButton.add(m_copyIcon);
    m_copyButton.set_relief(Gtk::RELIEF_NONE);
    m_copyButton.set_can_focus(false);
    m_pasteButton.set_tooltip_text("Paste (Ctrl+V)");
    m_pasteButton.add(m_pasteIcon);
    m_pasteButton.set_relief(Gtk::RELIEF_NONE);
    m_pasteButton.set_can_focus(false);
    m_undoButton.set_tooltip_text("Undo text (Ctrl+Z)");
    m_undoButton.add(m_undoIcon);
    m_undoButton.set_relief(Gtk::RELIEF_NONE);
    m_undoButton.set_can_focus(false);
    m_redoButton.set_tooltip_text("Redo text (Ctrl+Y)");
    m_redoButton.add(m_redoIcon);
    m_redoButton.set_relief(Gtk::RELIEF_NONE);
    m_redoButton.set_can_focus(false);
    m_boldButton.set_tooltip_text("Add bold text");
    m_boldButton.add(m_boldIcon);
    m_boldButton.set_relief(Gtk::RELIEF_NONE);
    m_boldButton.set_can_focus(false);
    m_italicButton.set_tooltip_text("Add italic text");
    m_italicButton.add(m_italicIcon);
    m_italicButton.set_relief(Gtk::RELIEF_NONE);
    m_italicButton.set_can_focus(false);
    m_strikethroughButton.set_tooltip_text("Add strikethrough text");
    m_strikethroughButton.add(m_strikethroughIcon);
    m_strikethroughButton.set_relief(Gtk::RELIEF_NONE);
    m_strikethroughButton.set_can_focus(false);
    m_superButton.set_tooltip_text("Add superscript text");
    m_superButton.add(m_superIcon);
    m_superButton.set_relief(Gtk::RELIEF_NONE);
    m_superButton.set_can_focus(false);
    m_subButton.set_tooltip_text("Add subscript text");
    m_subButton.add(m_subIcon);
    m_subButton.set_relief(Gtk::RELIEF_NONE);
    m_subButton.set_can_focus(false);
    m_linkButton.set_tooltip_text("Add a link");
    m_linkButton.add(m_linkIcon);
    m_linkButton.set_relief(Gtk::RELIEF_NONE);
    m_linkButton.set_can_focus(false);
    m_imageButton.set_tooltip_text("Add an image");
    m_imageButton.add(m_imageIcon);
    m_imageButton.set_relief(Gtk::RELIEF_NONE);
    m_imageButton.set_can_focus(false);
    m_emojiButton.set_tooltip_text("Insert emoji");
    m_emojiButton.add(m_emojiIcon);
    m_emojiButton.set_relief(Gtk::RELIEF_NONE);
    m_emojiButton.set_can_focus(false);
    m_quoteButton.set_tooltip_text("Insert a quote");
    m_quoteButton.add(m_quoteIcon);
    m_quoteButton.set_relief(Gtk::RELIEF_NONE);
    m_quoteButton.set_can_focus(false);
    m_codeButton.set_tooltip_text("Insert code");
    m_codeButton.add(m_codeIcon);
    m_codeButton.set_relief(Gtk::RELIEF_NONE);
    m_codeButton.set_can_focus(false);
    m_bulletListButton.set_tooltip_text("Add a bullet list");
    m_bulletListButton.add(m_bulletListIcon);
    m_bulletListButton.set_relief(Gtk::RELIEF_NONE);
    m_bulletListButton.set_can_focus(false);
    m_numberedListButton.set_tooltip_text("Add a numbered list");
    m_numberedListButton.add(m_numberedListIcon);
    m_numberedListButton.set_relief(Gtk::RELIEF_NONE);
    m_numberedListButton.set_can_focus(false);
    m_highlightButton.set_tooltip_text("Add highlight text");
    m_highlightButton.add(m_hightlightIcon);
    m_highlightButton.set_relief(Gtk::RELIEF_NONE);
    m_highlightButton.set_can_focus(false);

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
    auto styleBack = m_backButton.get_style_context();
    styleBack->add_class("circular");
    auto styleForward = m_forwardButton.get_style_context();
    styleForward->add_class("circular");
    auto styleRefresh = m_refreshButton.get_style_context();
    styleRefresh->add_class("circular");
    m_statusButton.set_popover(m_statusPopover);
    m_settingsButton.set_popover(m_settingsPopover);
    m_backButton.set_relief(Gtk::RELIEF_NONE);
    m_forwardButton.set_relief(Gtk::RELIEF_NONE);
    m_refreshButton.set_relief(Gtk::RELIEF_NONE);
    m_homeButton.set_relief(Gtk::RELIEF_NONE);
    m_searchButton.set_relief(Gtk::RELIEF_NONE);
    m_statusButton.set_relief(Gtk::RELIEF_NONE);
    m_settingsButton.set_relief(Gtk::RELIEF_NONE);

    // Toolbar buttons
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
    std::string spinningCSS = "@keyframes spin {  to { -gtk-icon-transform: rotate(1turn); }} .spinning { animation-name: spin;  animation-duration: 1s;  animation-timing-function: linear;  animation-iteration-count: infinite;}";
    if (!cssProvider->load_from_data(spinningCSS))
    {
        std::cerr << "ERROR: CSS data parsing went wrong." << std::endl;
    }
    auto refreshIconStyle = m_refreshIcon.get_style_context();
    refreshIconStyle->add_provider_for_screen(screen, cssProvider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // Add tooltips to the toolbar buttons
    m_backButton.set_tooltip_text("Go back one page (Alt+Left arrow)");
    m_forwardButton.set_tooltip_text("Go forward one page (Alt+Right arrow)");
    m_refreshButton.set_tooltip_text("Reload current page (Ctrl+R)");
    m_homeButton.set_tooltip_text("Home page (Alt+Home)");
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
    m_backButton.set_margin_left(6);
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
 * Init the IPFS status pop-over
 */
void MainWindow::initStatusPopover()
{
    m_statusLabel.set_text("Network is still starting..."); // fallback text
    m_copyIDButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::copy_client_id));
    m_copyIDButton.set_margin_start(6);
    m_copyIDButton.set_margin_end(6);
    m_copyPublicKeyButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::copy_client_public_key));
    m_copyPublicKeyButton.set_margin_start(6);
    m_copyPublicKeyButton.set_margin_end(6);

    m_vboxStatus.pack_start(m_statusLabel, Gtk::PACK_EXPAND_WIDGET, 4);
    m_vboxStatus.pack_end(m_copyPublicKeyButton, Gtk::PACK_EXPAND_WIDGET, 4);
    m_vboxStatus.pack_end(m_copyIDButton, Gtk::PACK_EXPAND_WIDGET, 4);
    m_statusPopover.set_position(Gtk::POS_BOTTOM);
    m_statusPopover.set_size_request(240, 120);
    m_statusPopover.set_margin_end(2);
    m_statusPopover.add(m_vboxStatus);
    m_statusPopover.show_all_children();
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
    m_scaleSettingsBrightness.set_adjustment(m_brightnessAdjustment);
    m_scaleSettingsBrightness.add_mark(1.0, Gtk::PositionType::POS_BOTTOM, "");
    m_scaleSettingsBrightness.set_draw_value(false);
    m_hboxSetingsBrightness.pack_start(m_brightnessImage, false, false);
    m_hboxSetingsBrightness.pack_end(m_scaleSettingsBrightness);

    // Spin buttons
    m_spacingSpinButton.set_adjustment(m_spacingAdjustment);
    m_marginsSpinButton.set_adjustment(m_marginsAdjustment);
    m_indentSpinButton.set_adjustment(m_indentAdjustment);
    m_fontLabel.set_xalign(1);
    m_spacingLabel.set_xalign(1);
    m_marginsLabel.set_xalign(1);
    m_indentLabel.set_xalign(1);
    auto fontLabelContext = m_fontLabel.get_style_context();
    fontLabelContext->add_class("dim-label");
    auto spacingLabelContext = m_spacingLabel.get_style_context();
    spacingLabelContext->add_class("dim-label");
    auto marginsLabelContext = m_marginsLabel.get_style_context();
    marginsLabelContext->add_class("dim-label");
    auto indentLabelContext = m_indentLabel.get_style_context();
    indentLabelContext->add_class("dim-label");
    m_gridSetings.set_margin_start(6);
    m_gridSetings.set_margin_top(6);
    m_gridSetings.set_margin_bottom(6);
    m_gridSetings.set_row_spacing(10);
    m_gridSetings.set_column_spacing(10);
    m_gridSetings.attach(m_fontLabel, 0, 0);
    m_gridSetings.attach(m_fontButton, 1, 0);
    m_gridSetings.attach(m_spacingLabel, 0, 1);
    m_gridSetings.attach(m_spacingSpinButton, 1, 1);
    m_gridSetings.attach(m_marginsLabel, 0, 2);
    m_gridSetings.attach(m_marginsSpinButton, 1, 2);
    m_gridSetings.attach(m_indentLabel, 0, 3);
    m_gridSetings.attach(m_indentSpinButton, 1, 3);

    // Icon theme (+ submenu)
    m_iconThemeButton.set_label("Icon Theme");
    m_iconThemeButton.property_menu_name() = "icon-theme";
    m_aboutButton.set_label("About LibreWeb");
    Gtk::Label *iconThemeButtonlabel = dynamic_cast<Gtk::Label *>(m_iconThemeButton.get_child());
    iconThemeButtonlabel->set_xalign(0.0);
    Gtk::Label *aboutButtonLabel = dynamic_cast<Gtk::Label *>(m_aboutButton.get_child());
    iconThemeButtonlabel->set_xalign(0.0);
    aboutButtonLabel->set_xalign(0.0);

    // Submenu: back button
    m_iconThemeBackButton.set_label("Icon Theme");
    m_iconThemeBackButton.property_menu_name() = "main";
    m_iconThemeBackButton.property_inverted() = true;
    // List box
    Gtk::Label *iconTheme1 = Gtk::manage(new Gtk::Label("Flat theme"));
    Gtk::ListBoxRow *row1 = Gtk::manage(new Gtk::ListBoxRow());
    row1->add(*iconTheme1);
    row1->set_data("value", (void *)"flat");
    Gtk::Label *iconTheme2 = Gtk::manage(new Gtk::Label("Filled theme"));
    Gtk::ListBoxRow *row2 = Gtk::manage(new Gtk::ListBoxRow());
    row2->add(*iconTheme2);
    row2->set_data("value", (void *)"filled");
    Gtk::Label *iconTheme3 = Gtk::manage(new Gtk::Label("Gtk default theme"));
    Gtk::ListBoxRow *row3 = Gtk::manage(new Gtk::ListBoxRow());
    row3->add(*iconTheme3);
    row3->set_data("value", (void *)"none");
    m_iconThemeListBox.add(*row1);
    m_iconThemeListBox.add(*row2);
    m_iconThemeListBox.add(*row3);
    m_iconThemeListBox.select_row(*row1); // TODO: Select the correct theme on loading
    m_iconThemeListScrolledWindow.property_height_request() = 200;
    m_iconThemeListScrolledWindow.add(m_iconThemeListBox);
    m_iconThemeLabel.set_xalign(0.0);
    m_iconThemeLabel.get_style_context()->add_class("dim-label");
    m_vboxIconTheme.add(m_iconThemeBackButton);
    m_vboxIconTheme.add(m_separator8);
    m_vboxIconTheme.add(m_iconThemeLabel);
    m_vboxIconTheme.add(m_iconThemeListScrolledWindow);
    // TODO: Some strange sliding animation happens the first time I collapse the settings popover
    m_settingsPopover.add(m_vboxIconTheme);
    
    m_settingsPopover.child_property_submenu(m_vboxIconTheme) = "icon-theme";

    // Add all to vbox / pop-over
    m_vboxSettings.set_margin_start(10);
    m_vboxSettings.set_margin_end(10);
    m_vboxSettings.set_margin_top(10);
    m_vboxSettings.set_margin_bottom(10);
    m_vboxSettings.set_spacing(8);
    m_vboxSettings.add(m_hboxSetingsZoom);
    // m_vboxSettings.add(m_hboxSetingsBrightness); // TODO
    m_vboxSettings.add(m_separator5);
    m_vboxSettings.add(m_gridSetings);
    m_vboxSettings.add(m_separator6);
    m_vboxSettings.add(m_iconThemeButton);
    m_vboxSettings.add(m_separator7);
    m_vboxSettings.pack_end(m_aboutButton, false, false);
    m_settingsPopover.set_position(Gtk::POS_BOTTOM);
    m_settingsPopover.set_size_request(200, 300);
    m_settingsPopover.set_margin_end(2);
    m_settingsPopover.add(m_vboxSettings);
    m_settingsPopover.show_all_children();
}

/**
 * Init all signals and connect them to functions
 */
void MainWindow::initSignals()
{
    // Timeouts
    this->statusTimerHandler = Glib::signal_timeout().connect(sigc::mem_fun(this, &MainWindow::update_connection_status), 3000);

    // Window signals
    this->signal_delete_event().connect(sigc::mem_fun(this, &MainWindow::delete_window));

    // Menu & toolbar signals
    m_menu.new_doc.connect(sigc::mem_fun(this, &MainWindow::new_doc));                                               /*!< Menu item for new document */
    m_menu.open.connect(sigc::mem_fun(this, &MainWindow::open));                                                     /*!< Menu item for opening existing document */
    m_menu.open_edit.connect(sigc::mem_fun(this, &MainWindow::open_and_edit));                                       /*!< Menu item for opening & editing existing document */
    m_menu.edit.connect(sigc::mem_fun(this, &MainWindow::edit));                                                     /*!< Menu item for editing current open document */
    m_menu.save.connect(sigc::mem_fun(this, &MainWindow::save));                                                     /*!< Menu item for save document */
    m_menu.save_as.connect(sigc::mem_fun(this, &MainWindow::save_as));                                               /*!< Menu item for save document as */
    m_menu.publish.connect(sigc::mem_fun(this, &MainWindow::publish));                                               /*!< Menu item for publishing */
    m_menu.quit.connect(sigc::mem_fun(this, &MainWindow::close));                                                    /*!< close main window and therefor closes the app */
    m_menu.undo.connect(sigc::mem_fun(m_draw_main, &Draw::undo));                                                    /*!< Menu item for undo text */
    m_menu.redo.connect(sigc::mem_fun(m_draw_main, &Draw::redo));                                                    /*!< Menu item for redo text */
    m_menu.cut.connect(sigc::mem_fun(this, &MainWindow::cut));                                                       /*!< Menu item for cut text */
    m_menu.copy.connect(sigc::mem_fun(this, &MainWindow::copy));                                                     /*!< Menu item for copy text */
    m_menu.paste.connect(sigc::mem_fun(this, &MainWindow::paste));                                                   /*!< Menu item for paste text */
    m_menu.del.connect(sigc::mem_fun(this, &MainWindow::del));                                                       /*!< Menu item for deleting selected text */
    m_menu.select_all.connect(sigc::mem_fun(this, &MainWindow::selectAll));                                          /*!< Menu item for selecting all text */
    m_menu.find.connect(sigc::bind(sigc::mem_fun(this, &MainWindow::show_search), false));                           /*!< Menu item for finding text */
    m_menu.replace.connect(sigc::bind(sigc::mem_fun(this, &MainWindow::show_search), true));                         /*!< Menu item for replacing text */
    m_menu.back.connect(sigc::mem_fun(this, &MainWindow::back));                                                     /*!< Menu item for previous page */
    m_menu.forward.connect(sigc::mem_fun(this, &MainWindow::forward));                                               /*!< Menu item for next page */
    m_menu.reload.connect(sigc::mem_fun(this, &MainWindow::refresh));                                                /*!< Menu item for reloading the page */
    m_menu.home.connect(sigc::mem_fun(this, &MainWindow::go_home));                                                  /*!< Menu item for home page */
    m_menu.source_code.connect(sigc::mem_fun(this, &MainWindow::show_source_code_dialog));                           /*!< Source code dialog */
    m_sourceCodeDialog.signal_response().connect(sigc::mem_fun(m_sourceCodeDialog, &SourceCodeDialog::hide_dialog)); /*!< Close source code dialog */
    m_menu.about.connect(sigc::mem_fun(m_about, &About::show_about));                                                /*!< Display about dialog */
    m_draw_main.source_code.connect(sigc::mem_fun(this, &MainWindow::show_source_code_dialog));                      /*!< Open source code dialog */
    m_about.signal_response().connect(sigc::mem_fun(m_about, &About::hide_about));                                   /*!< Close about dialog */
    m_addressBar.signal_activate().connect(sigc::mem_fun(this, &MainWindow::address_bar_activate));                  /*!< User pressed enter the address bar */
    m_backButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::back));                                   /*!< Button for previous page */
    m_forwardButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::forward));                             /*!< Button for next page */
    m_refreshButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::refresh));                             /*!< Button for reloading the page */
    m_homeButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::go_home));                                /*!< Button for home page */
    m_searchButton.signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &MainWindow::show_search), false));       /*!< Button for finding text */
    m_searchEntry.signal_activate().connect(sigc::mem_fun(this, &MainWindow::on_search));                            /*!< Execute the text search */
    m_searchReplaceEntry.signal_activate().connect(sigc::mem_fun(this, &MainWindow::on_replace));                    /*!< Execute the text replace */

    // Editor buttons
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

    // Settings pop-over buttons
    m_zoomOutButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::on_zoom_out));
    m_zoomRestoreButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::on_zoom_restore));
    m_zoomInButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::on_zoom_in));
    m_fontButton.signal_font_set().connect(sigc::mem_fun(this, &MainWindow::on_font_set));
    m_spacingSpinButton.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_spacing_changed));
    m_marginsSpinButton.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_margins_changed));
    m_indentSpinButton.signal_value_changed().connect(sigc::mem_fun(this, &MainWindow::on_indent_changed));
    m_iconThemeListBox.signal_row_activated().connect(sigc::mem_fun(this, &MainWindow::on_icon_theme_activated));
    m_aboutButton.signal_clicked().connect(sigc::mem_fun(m_about, &About::show_about));
}

/**
 * Fetch document from disk or IPFS, using threading
 * \param path File path that needs to be opened (either from disk or IPFS network)
 * \param isSetAddressBar If true change update the address bar with the file path (default: true)
 * \param isHistoryRequest Set to true if this is an history request call: back/forward (default: false)
 * \param isDisableEditor If true the editor will be disabled if needed (default: true)
 * \param isParseContext If true the content received will be parsed and displayed as markdown syntax (default: true), set to false if you want to editor the content
 */
void MainWindow::doRequest(const std::string &path, bool isSetAddressBar, bool isHistoryRequest, bool isDisableEditor, bool isParseContent)
{
    if (m_requestThread)
    {
        if (m_requestThread->joinable())
        {
            pthread_cancel(m_requestThread->native_handle());
            m_requestThread->join();
            delete m_requestThread;
            m_requestThread = nullptr;
        }
    }

    if (m_requestThread == nullptr)
    {
        // Show spinning icon
        m_refreshIcon.get_style_context()->add_class("spinning");
        // Start thread
        m_requestThread = new std::thread(&MainWindow::processRequest, this, path, isParseContent);
        this->postDoRequest(path, isSetAddressBar, isHistoryRequest, isDisableEditor);
    }
}

/**
 * \brief Called when Window is closed/exited
 */
bool MainWindow::delete_window(GdkEventAny *any_event __attribute__((unused)))
{
    // Save the schema settings
    m_settings->set_int("width", this->get_width());
    m_settings->set_int("height", this->get_height());
    m_settings->set_boolean("maximized", this->is_maximized());
    // Only store a divider value bigger than zero,
    // because the secondary draw window is hidden by default, resulting into a zero value.
    if (this->m_paned.get_position() > 0)
        m_settings->set_int("position-divider", this->m_paned.get_position());

    // Fullscreen will be availible with gtkmm-4.0
    //m_settings->set_boolean("fullscreen", this->is_fullscreen());

    m_settings->set_string("font-family", this->m_fontFamily);
    m_settings->set_int("font-size", this->m_currentFontSize);
    m_settings->set_int("spacing", this->m_spacingSpinButton.get_value_as_int());
    m_settings->set_int("margins", this->m_marginsSpinButton.get_value_as_int());
    m_settings->set_int("indent", this->m_indentSpinButton.get_value_as_int());
    m_settings->set_string("icon-theme", this->m_iconTheme);
    m_settings->set_boolean("icon-gtk-theme", this->m_useCurrentGTKIconTheme);    
    return false;
}

/**
 * \brief Timeout slot: Update the IPFS connection status every x seconds
 */
bool MainWindow::update_connection_status()
{
    // Try to set the client ID & Public key and version
    if (this->clientID.empty())
        this->clientID = ipfs.getClientID();
    if (this->clientPublicKey.empty())
        this->clientPublicKey = ipfs.getClientPublicKey();
    if (this->ipfsVersion.empty())
        this->ipfsVersion = ipfs.getVersion();

    std::size_t nrPeers = loadStatusIcon(false); // No reloading of the image required
    if (nrPeers > 0)
    {
        // Auto-refresh page if needed (when 'Please wait' page is shown)
        if (m_waitPageVisible)
            this->refresh();

        std::map<std::string, float> rates = ipfs.getBandwidthRates();
        char buf[32];
        std::string in = std::string(buf, std::snprintf(buf, sizeof buf, "%.1f", rates.at("in") / 1000.0));
        std::string out = std::string(buf, std::snprintf(buf, sizeof buf, "%.1f", rates.at("out") / 1000.0));

        std::map<std::string, std::variant<int, std::string>> repoStats = ipfs.getRepoStats();
        int totalRepoSize = std::get<int>(repoStats.at("total_size"));
        std::string repoPath = std::get<std::string>(repoStats.at("path"));

        // And also update text
        m_statusLabel.set_text("IPFS Network Stats:\n\nConnected peers: " + std::to_string(nrPeers) +
                               "\nRate in:    " + in + " kB/s" +
                               "\nRate out: " + out + " kB/s" +
                               "\n\nTotal repo size: " + std::to_string(totalRepoSize) + " MB" +
                               "\nRepo path: " + repoPath +
                               "\n\nIPFS version: " + this->ipfsVersion);
    }
    else
    {
        m_statusLabel.set_text("IPFS Status: Connecting...");
    }

    // Keep going (never disconnect the timer)
    return true;
}

/***
 * Cut/copy/paste/delete/select all keybindings
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
 * \brief Trigger when user selected 'new document' from menu item
 */
void MainWindow::new_doc()
{
    // Clear content & requestPath
    this->currentContent = "";
    this->requestPath = "";

    // Enable editing mode
    this->enableEdit();
    // Change address bar
    this->m_addressBar.set_text("file://unsaved");
    // Set new title
    this->set_title("Untitled * - " + m_appName);
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
    auto filter_markdown = Gtk::FileFilter::create();
    filter_markdown->set_name("Markdown files (.md)");
    filter_markdown->add_mime_type("text/markdown");
    dialog->add_filter(filter_markdown);

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Any files");
    filter_any->add_pattern("*");
    dialog->add_filter(filter_any);

    dialog->show();
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
    auto filter_markdown = Gtk::FileFilter::create();
    filter_markdown->set_name("Markdown files (.md)");
    filter_markdown->add_mime_type("text/markdown");
    dialog->add_filter(filter_markdown);

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Any files");
    filter_any->add_pattern("*");
    dialog->add_filter(filter_any);

    dialog->show();
}

/**
 * \brief Signal response when 'open' dialog is closed
 */
void MainWindow::on_open_dialog_response(int response_id, Gtk::FileChooserDialog *dialog)
{
    switch (response_id)
    {
    case Gtk::ResponseType::RESPONSE_OK:
    {
        auto filePath = dialog->get_file()->get_path();
        // Open file, set address bar & disable editor if needed
        doRequest("file://" + filePath);

        // Set new title
        std::string fileName = File::getFilename(filePath);
        this->set_title(fileName + " - " + m_appName);
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
void MainWindow::on_open_edit_dialog_response(int response_id, Gtk::FileChooserDialog *dialog)
{
    switch (response_id)
    {
    case Gtk::ResponseType::RESPONSE_OK:
    {
        // Enable editor if needed
        if (!this->isEditorEnabled())
            this->enableEdit();

        auto filePath = dialog->get_file()->get_path();
        std::string path = "file://" + filePath;
        // Open file and set address bar, but do not parse the content or the disable editor
        doRequest(path, true, false, false, false);

        // Change address bar
        this->m_addressBar.set_text(path);
        // Set new title
        std::string fileName = File::getFilename(filePath);
        this->set_title(fileName + " - " + m_appName);
        // Set current file path for saving feature
        this->currentFileSavedPath = filePath;
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
    if (!this->isEditorEnabled())
        this->enableEdit();

    m_draw_main.setText(this->currentContent);
    // Set title
    this->set_title("Untitled * - " + m_appName);
}

/**
 * \brief Triggered when user selected 'save' from menu item / toolbar
 */
void MainWindow::save()
{
    if (currentFileSavedPath.empty())
    {
        this->save_as();
    }
    else
    {
        if (this->isEditorEnabled())
        {
            try
            {
                File::write(currentFileSavedPath, this->currentContent);
            }
            catch (std::ios_base::failure &error)
            {
                std::cerr << "ERROR: Could not write file: " << currentFileSavedPath << ". Message: " << error.what() << ".\nError code: " << error.code() << std::endl;
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
    auto filter_markdown = Gtk::FileFilter::create();
    filter_markdown->set_name("Markdown files (.md)");
    filter_markdown->add_mime_type("text/markdown");
    dialog->add_filter(filter_markdown);

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Any files");
    filter_any->add_pattern("*");
    dialog->add_filter(filter_any);

    // If user is saving as an existing file, set the current uri path
    if (!this->currentFileSavedPath.empty())
    {
        try
        {
            dialog->set_uri(Glib::filename_to_uri(currentFileSavedPath));
        }
        catch (Glib::Error &error)
        {
            std::cerr << "ERROR: Incorrect filename most likely. Message: " << error.what() << ". Error Code: " << error.code() << std::endl;
        }
    }
    dialog->show();
}

/**
 * \brief Signal response when 'save as' dialog is closed
 */
void MainWindow::on_save_as_dialog_response(int response_id, Gtk::FileChooserDialog *dialog)
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
            File::write(filePath, this->currentContent);
            // Only if editor mode is enabled
            if (this->isEditorEnabled())
            {
                // Set/update the current file saved path variable (used for the 'save' feature)
                this->currentFileSavedPath = filePath;
                // And also update the address bar with the current file path
                this->m_addressBar.set_text("file://" + filePath);
                // Set title
                std::string fileName = File::getFilename(filePath);
                this->set_title(fileName + " - " + m_appName);
            }
        }
        catch (std::ios_base::failure &error)
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
    if (this->currentContent.empty())
    {
        Gtk::MessageDialog dialog(*this, "Are you sure you want to publish <b>empty</b> content?", true,
                                  Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
        dialog.set_title("Are you sure?");
        dialog.set_default_response(Gtk::RESPONSE_NO);
        result = dialog.run();
    }

    // Continue ...
    if (result == Gtk::RESPONSE_YES)
    {
        std::string path = "new_file.md";
        // Retrieve filename from saved file (if present)
        if (!currentFileSavedPath.empty())
        {
            path = currentFileSavedPath;
        }
        else
        {
            // TODO: path is not defined yet. however, this may change anyway once we try to build more complex websites,
            // needing to use directory structures.
        }
        // TODO:  should we run this within a seperate thread? Looks fine until now without threading.

        // Add content to IPFS!
        try
        {
            std::string cid = ipfs.add(path, this->currentContent);
            if (!cid.empty())
            {
                // Show dialog
                m_contentPublishedDialog.reset(new Gtk::MessageDialog(*this, "File is successfully added to IPFS!"));
                m_contentPublishedDialog->set_secondary_text("The content is now available on the decentralized web, via:");
                // Add custom label
                Gtk::Label *label = Gtk::manage(new Gtk::Label("ipfs://" + cid));
                label->set_selectable(true);
                Gtk::Box *box = m_contentPublishedDialog->get_content_area();
                box->pack_end(*label);

                m_contentPublishedDialog->set_modal(true);
                // m_contentPublishedDialog->set_hide_on_close(true); available in gtk-4.0
                m_contentPublishedDialog->signal_response().connect(
                    sigc::hide(sigc::mem_fun(*m_contentPublishedDialog, &Gtk::Widget::hide)));
                m_contentPublishedDialog->show_all();
            }
            else
            {
                m_contentPublishedDialog.reset(new Gtk::MessageDialog(*this, "File failed to be added added to IPFS", false,
                                                                      Gtk::MESSAGE_ERROR));
                m_contentPublishedDialog->set_modal(true);
                // m_contentPublishedDialog->set_hide_on_close(true); available in gtk-4.0
                m_contentPublishedDialog->signal_response().connect(
                    sigc::hide(sigc::mem_fun(*m_contentPublishedDialog, &Gtk::Widget::hide)));
                m_contentPublishedDialog->show();
            }
        }
        catch (const std::runtime_error &error)
        {
            m_contentPublishedDialog.reset(new Gtk::MessageDialog(*this, "File could not be added to IPFS", false,
                                                                  Gtk::MESSAGE_ERROR));
            m_contentPublishedDialog->set_secondary_text("Error message: " + std::string(error.what()));
            m_contentPublishedDialog->set_modal(true);
            // m_contentPublishedDialog->set_hide_on_close(true); available in gtk-4.0
            m_contentPublishedDialog->signal_response().connect(
                sigc::hide(sigc::mem_fun(*m_contentPublishedDialog, &Gtk::Widget::hide)));
            m_contentPublishedDialog->show();
        }
    }
}

/**
 * \brief Post-processing request actions
 * \param path File path (on disk or IPFS) that needs to be processed
 * \param isSetAddressBar If true change update the address bar with the file path
 * \param isHistoryRequest Set to true if this is an history request call: back/forward
 * \param isDisableEditor If true the editor will be disabled if needed
 */
void MainWindow::postDoRequest(const std::string &path, bool isSetAddressBar, bool isHistoryRequest, bool isDisableEditor)
{
    if (isSetAddressBar)
        m_addressBar.set_text(path);

    if (isDisableEditor && isEditorEnabled())
        this->disableEdit();

    // Do not insert history back/forward calls into the history (again)
    if (!isHistoryRequest)
    {
        if (history.size() == 0)
        {
            history.push_back(path);
            currentHistoryIndex = history.size() - 1;
        }
        else if (history.size() > 0 && !path.empty() && (history.back().compare(path) != 0))
        {
            history.push_back(path);
            currentHistoryIndex = history.size() - 1;
        }
    }

    // Enable back/forward buttons when possible
    m_backButton.set_sensitive(currentHistoryIndex > 0);
    m_menu.setBackMenuSensitive(currentHistoryIndex > 0);
    m_forwardButton.set_sensitive(currentHistoryIndex < history.size() - 1);
    m_menu.setForwardMenuSensitive(currentHistoryIndex < history.size() - 1);
}

/**
 * \brief Show homepage
 */
void MainWindow::go_home()
{
    doRequest("about:home", true, false, true);
}

/**
 * \brief Copy the IPFS Client ID to clipboard
 */
void MainWindow::copy_client_id()
{
    if (!this->clientID.empty())
        get_clipboard("CLIPBOARD")->set_text(this->clientID);
    else
        std::cerr << "WARNING: IPFS client ID has not been set yet. Skip clipboard action." << std::endl;
}

void MainWindow::copy_client_public_key()
{
    if (!this->clientPublicKey.empty())
        get_clipboard("CLIPBOARD")->set_text(this->clientPublicKey);
    else
        std::cerr << "WARNING: IPFS client public key has not been set yet. Skip clipboard action." << std::endl;
}

/**
 * Trigger when pressed enter in the search entry
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
 * Trigger when user pressed enter in the replace entry
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
        this->on_search();
    }
}

/**
 * Triggers when pressed enter in the address bar
 */
void MainWindow::address_bar_activate()
{
    doRequest(m_addressBar.get_text(), false);
    // When user actually entered the address bar, focus on the main draw
    m_draw_main.grab_focus();
}

/**
 * Triggers when user tries to search or replace text
 */
void MainWindow::show_search(bool replace)
{
    if (m_hboxBottom.is_visible() && m_searchReplaceEntry.is_visible())
    {
        if (replace)
        {
            m_hboxBottom.hide();
            m_addressBar.grab_focus();
            m_searchReplaceEntry.hide();
        }
        else
        {
            m_searchReplaceEntry.hide();
        }
    }
    else if (m_hboxBottom.is_visible())
    {
        if (replace)
        {
            m_searchReplaceEntry.show();
        }
        else
        {
            m_hboxBottom.hide();
            m_addressBar.grab_focus();
            m_searchReplaceEntry.hide();
        }
    }
    else
    {
        m_hboxBottom.show();
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
    if (currentHistoryIndex > 0)
    {
        currentHistoryIndex--;
        doRequest(history.at(currentHistoryIndex), true, true);
    }
}

void MainWindow::forward()
{
    if (currentHistoryIndex < history.size() - 1)
    {
        currentHistoryIndex++;
        doRequest(history.at(currentHistoryIndex), true, true);
    }
}

void MainWindow::refresh()
{
    // Only allow refresh if editor is disabled (doesn't make sense otherwise to refresh)
    if (!this->isEditorEnabled())
        doRequest("", false, false, false); /*!< Reload existing file, don't need to update the address bar, don't disable the editor */
}

/**
 * \brief Determing if browser is installed from current binary path, at runtime
 * \return true if the current running process is installed (to the installed prefix path)
 */
bool MainWindow::isInstalled()
{
    char pathbuf[1024];
    memset(pathbuf, 0, sizeof(pathbuf));
    if (readlink("/proc/self/exe", pathbuf, sizeof(pathbuf) - 1) > 0)
    {
        // If current binary path starts with the install prefix, it's installed
        return (strncmp(pathbuf, INSTALL_PREFIX, strlen(INSTALL_PREFIX)) == 0);
    }
    else
    {
        return true; // fallback; always installed
    }
}

void MainWindow::enableEdit()
{
    // Inform the Draw class that we are creating a new document
    this->m_draw_main.newDocument();
    // Show editor toolbars
    this->m_hboxStandardEditorToolbar.show();
    this->m_hboxFormattingEditorToolbar.show();
    // Determine position of divider between the primary and secondary windows
    int location = 0;
    int positionSettings = m_settings->get_int("position-divider");
    int currentWidth, _ = 0;
    this->get_size(currentWidth, _);
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
    this->m_paned.set_position(location);

    // Enabled secondary text view (on the right)
    this->m_scrolledWindowSecondary.show();
    // Disable "view source" menu item
    this->m_draw_main.setViewSourceMenuItem(false);
    // Connect changed signal
    this->textChangedSignalHandler = m_draw_main.get_buffer().get()->signal_changed().connect(sigc::mem_fun(this, &MainWindow::editor_changed_text));
    // Enable publish menu item
    this->m_menu.setPublishMenuSensitive(true);
    // Disable edit menu item (you are already editing)
    this->m_menu.setEditMenuSensitive(false);
    // Just to be sure, disable the spinning animation
    this->m_refreshIcon.get_style_context()->remove_class("spinning");
}

void MainWindow::disableEdit()
{
    if (this->isEditorEnabled())
    {
        this->m_hboxStandardEditorToolbar.hide();
        this->m_hboxFormattingEditorToolbar.hide();
        this->m_scrolledWindowSecondary.hide();
        // Disconnect text changed signal
        this->textChangedSignalHandler.disconnect();
        // Show "view source" menu item again
        this->m_draw_main.setViewSourceMenuItem(true);
        this->m_draw_secondary.clearText();
        // Disable publish menu item
        this->m_menu.setPublishMenuSensitive(false);
        // Enable edit menu item
        this->m_menu.setEditMenuSensitive(true);
        // Empty current file saved path
        this->currentFileSavedPath = "";
        // Restore title
        set_title(m_appName);
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
 * \brief Get the file from disk or IPFS network, from the provided path,
 * parse the content, and display the document
 * \param path File path that needs to be fetched (from disk or IPFS network)
 * \param isParseContent Set to true if you want to parse and display the content as markdown syntax (from disk or IPFS network), 
 * set to false if you want to edit the content
 */
void MainWindow::processRequest(const std::string &path, bool isParseContent)
{
    // Reset private variables
    this->currentContent = "";
    this->m_waitPageVisible = false;

    // Do not update the requestPath when path is empty,
    // this is used for refreshing the page
    if (!path.empty())
    {
        requestPath = path;
    }

    if (requestPath.empty())
    {
        std::cerr << "Info: Empty request path." << std::endl;
    }
    // Handle homepage
    else if (requestPath.compare("about:home") == 0)
    {
        m_draw_main.showStartPage();
        m_refreshIcon.get_style_context()->remove_class("spinning");
    }
    // Handle disk or IPFS file paths
    else
    {
        // Check if CID
        if (requestPath.rfind("ipfs://", 0) == 0)
        {
            finalRequestPath = requestPath;
            finalRequestPath.erase(0, 7);
            fetchFromIPFS(isParseContent);
        }
        else if ((requestPath.length() == 46) && (requestPath.rfind("Qm", 0) == 0))
        {
            // CIDv0
            finalRequestPath = requestPath;
            fetchFromIPFS(isParseContent);
        }
        else if (requestPath.rfind("file://", 0) == 0)
        {
            finalRequestPath = requestPath;
            finalRequestPath.erase(0, 7);
            openFromDisk(isParseContent);
        }
        else
        {
            // IPFS as fallback / CIDv1
            finalRequestPath = requestPath;
            fetchFromIPFS(isParseContent);
        }
    }
}

/**
 * \brief Helper method for processRequest(), display markdown file from IPFS network. 
 * Runs in a seperate thread.
 * \param isParseContent Set to true if you want to parse and display the content as markdown syntax (from disk or IPFS network), 
 * set to false if you want to edit the content
 */
void MainWindow::fetchFromIPFS(bool isParseContent)
{
    try
    {
        this->currentContent = ipfs.fetch(finalRequestPath);
        if (isParseContent)
        {
            cmark_node *doc = Parser::parseContent(this->currentContent);
            m_draw_main.processDocument(doc);
            cmark_node_free(doc);
        }
        else
        {
            // directly set the plain content
            m_draw_main.setText(this->currentContent);
        }
    }
    catch (const std::runtime_error &error)
    {
        std::string errorMessage = std::string(error.what());
        std::cerr << "ERROR: IPFS request failed, with message: " << errorMessage << std::endl;
        if (errorMessage.starts_with("HTTP request failed with status code"))
        {
            // Remove text until ':\n'
            errorMessage.erase(0, errorMessage.find(':') + 2);
            auto content = nlohmann::json::parse(errorMessage);
            std::string message = content.value("Message", "");
            if (message.starts_with("context deadline exceeded"))
            {
                message += ". Time-out is set to: " + this->ipfsTimeout;
            }
            m_draw_main.showMessage("🎂 We're having trouble finding this site.", "Message: " + message + ".\n\nYou could try to reload or increase the time-out.");
        }
        else if (errorMessage.starts_with("Couldn't connect to server: Failed to connect to localhost"))
        {
            m_draw_main.showMessage("⌛ Please wait...", "IPFS daemon is still spinnng-up, page will automatically refresh...");
            m_waitPageVisible = true; // Please wait page is shown (auto-refresh when network is up)
        }
        else
        {
            m_draw_main.showMessage("❌ Something went wrong", "Error message: " + std::string(error.what()));
        }
    }
    // Stop spinning
    m_refreshIcon.get_style_context()->remove_class("spinning");
}

/**
 * \brief Helper method for processRequest(), display markdown file from disk.
 * Runs in a seperate thread.
 * \param isParseContent Set to true if you want to parse and display the content as markdown syntax (from disk or IPFS network), 
 * set to false if you want to edit the content
 */
void MainWindow::openFromDisk(bool isParseContent)
{
    try
    {
        this->currentContent = File::read(finalRequestPath);
        if (isParseContent)
        {
            cmark_node *doc = Parser::parseContent(this->currentContent);
            m_draw_main.processDocument(doc);
            cmark_node_free(doc);
        }
        else
        {
            // directly set the plain content
            m_draw_main.setText(this->currentContent);
        }
    }
    catch (const std::ios_base::failure &error)
    {
        std::cerr << "ERROR: Could not read file: " << finalRequestPath << ". Message: " << error.what() << ".\nError code: " << error.code() << std::endl;
        m_draw_main.showMessage("🎂 Could not read file", "Message: " + std::string(error.what()));
    }
    catch (const std::runtime_error &error)
    {
        std::cerr << "ERROR: File request failed, with message: " << error.what() << std::endl;
        m_draw_main.showMessage("🎂 File not found", "Message: " + std::string(error.what()));
    }
    // Stop spinning
    m_refreshIcon.get_style_context()->remove_class("spinning");
}

/**
 * Retrieve image path from icon theme location
 * @param iconName Icon name (.svg is added default)
 * @param typeofIcon Type of the icon is the sub-folder within the icons directory (eg. "editor", "arrows" or "basic")
 * @return full path of the icon SVG image
 */
std::string MainWindow::getIconImageFromTheme(const std::string &iconName, const std::string &typeofIcon)
{
    // Try absolute path first
    for (std::string data_dir : Glib::get_system_data_dirs())
    {
        std::vector<std::string> path_builder{data_dir, "libreweb-browser", "images", "icons", m_iconTheme, typeofIcon, iconName + ".svg"};
        std::string file_path = Glib::build_path(G_DIR_SEPARATOR_S, path_builder);
        if (Glib::file_test(file_path, Glib::FileTest::FILE_TEST_IS_REGULAR))
        {
            return file_path;
        }
    }

    // Try local path if the images are not installed (yet)
    // When working directory is in the build/bin folder (relative path)
    std::string file_path = Glib::build_filename("../../images/icons", m_iconTheme, typeofIcon, iconName + ".svg");
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
 * Update the CSS provider data on the main draw text view
 */
void MainWindow::updateCSS()
{
    m_drawCSSProvider->load_from_data("textview { "
                                      "font-family: \"" +  m_fontFamily + "\";"
                                      "font-size: " + std::to_string(m_currentFontSize) + "pt;"
                                      "letter-spacing: " + std::to_string(m_fontSpacing) + "px;"
                                      "}");
}

void MainWindow::editor_changed_text()
{
    // Retrieve text from text editor
    this->currentContent = m_draw_main.getText();
    // Parse the markdown contents
    cmark_node *doc = Parser::parseContent(this->currentContent);
    /* Can be enabled to show the markdown format in terminal:
    std::string md = Parser::renderMarkdown(doc);
    std::cout << "Markdown:\n" << md << std::endl;*/

    // Show the document as a preview on the right side text-view panel
    m_draw_secondary.processDocument(doc);
    cmark_node_free(doc);
}

/**
 * Show source code dialog window with the current content
 */
void MainWindow::show_source_code_dialog()
{
    m_sourceCodeDialog.setText(this->currentContent);
    m_sourceCodeDialog.run();
}

/**
 * Retrieve selected heading from combobox.
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
        catch (const std::invalid_argument &)
        {
            // ignore
            std::cerr << "ERROR: heading combobox id is invalid (not a number)." << std::endl;
        }
        catch (const std::out_of_range &)
        {
            // ignore
        }
    }
}

void MainWindow::insert_emoji()
{
    // Note: The "insert-emoji" signal is not exposed in Gtkmm library (at least not in gtk3)
    g_signal_emit_by_name(this->m_draw_main.gobj(), "insert-emoji");
}

void MainWindow::on_zoom_out()
{
    m_currentFontSize -= 2;
    updateCSS();
    m_zoomRestoreButton.set_sensitive(m_currentFontSize != m_defaultFontSize);
}

void MainWindow::on_zoom_restore()
{
    m_currentFontSize = m_defaultFontSize; // reset
    updateCSS();
    m_zoomRestoreButton.set_sensitive(false);
}

void MainWindow::on_zoom_in()
{
    m_currentFontSize += 2;
    updateCSS();
    m_zoomRestoreButton.set_sensitive(m_currentFontSize != m_defaultFontSize);
}

void MainWindow::on_font_set()
{
    Pango::FontDescription fontDesc = Pango::FontDescription(m_fontButton.get_font_name());
    m_fontFamily = fontDesc.get_family();
    m_currentFontSize = m_defaultFontSize = (fontDesc.get_size_is_absolute()) ? fontDesc.get_size() : fontDesc.get_size() / PANGO_SCALE;
    updateCSS();
}

void MainWindow::on_spacing_changed()
{
    m_fontSpacing = m_spacingSpinButton.get_value_as_int(); // Letter spacing
    updateCSS();
}

void MainWindow::on_margins_changed()
{
    this->m_draw_main.set_left_margin(m_marginsSpinButton.get_value_as_int());
    this->m_draw_main.set_right_margin(m_marginsSpinButton.get_value_as_int());
}

void MainWindow::on_indent_changed()
{
    this->m_draw_main.set_indent(m_indentSpinButton.get_value_as_int());
}

void MainWindow::on_icon_theme_activated(Gtk::ListBoxRow *row)
{
    std::string themeName = static_cast<char *>(row->get_data("value"));
    if (themeName != "none") {
        this->m_iconTheme = themeName;
        this->m_useCurrentGTKIconTheme = false;
    } else {
        this->m_useCurrentGTKIconTheme = true;
    }
    // Reload images
    this->loadIcons();
}