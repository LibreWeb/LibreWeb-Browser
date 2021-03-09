#include "mainwindow.h"

#include "md-parser.h"
#include "menu.h"
#include <gtkmm/menuitem.h>
#include <gtkmm/image.h>
#include <giomm/file.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cmark-gfm.h>
#include <pthread.h>
#include <iostream>
#include <nlohmann/json.hpp>

MainWindow::MainWindow()
    : accelGroup(Gtk::AccelGroup::create()),
      m_menu(accelGroup),
      m_draw_main(*this),
      m_draw_secondary(*this),
      m_vbox(Gtk::ORIENTATION_VERTICAL, 0),
      m_hboxBrowserToolbar(Gtk::ORIENTATION_HORIZONTAL, 0),
      m_hboxStandardEditorToolbar(Gtk::ORIENTATION_HORIZONTAL, 0),
      m_hboxFormattingEditorToolbar(Gtk::ORIENTATION_HORIZONTAL, 0),
      m_hboxBottom(Gtk::ORIENTATION_HORIZONTAL, 0),
      m_appName("LibreWeb Browser"),
      m_iconTheme("flat"), // filled or flat
      m_requestThread(nullptr),
      requestPath(""),
      finalRequestPath(""),
      currentContent(""),
      currentHistoryIndex(0)
{
    set_title(m_appName);
    set_default_size(1000, 800);
    set_position(Gtk::WIN_POS_CENTER);
    add_accel_group(accelGroup);

    // Connect signals
    m_menu.new_doc.connect(sigc::mem_fun(this, &MainWindow::new_doc));                                               /*!< Menu item for new document */
    m_menu.open.connect(sigc::mem_fun(this, &MainWindow::open));                                                     /*!< Menu item for opening existing document */
    m_menu.save.connect(sigc::mem_fun(this, &MainWindow::save));                                                     /*!< Menu item for save document */
    m_menu.save_as.connect(sigc::mem_fun(this, &MainWindow::save_as));                                               /*!< Menu item for save document as */
    m_menu.publish.connect(sigc::mem_fun(this, &MainWindow::publish));                                               /*!< Menu item for publishing */
    m_menu.quit.connect(sigc::mem_fun(this, &MainWindow::hide));                                                     /*!< hide main window and therefor closes the app */
    m_menu.undo.connect(sigc::mem_fun(m_draw_main, &Draw::undo));                                                    /*!< Menu item for undo text */
    m_menu.redo.connect(sigc::mem_fun(m_draw_main, &Draw::redo));                                                    /*!< Menu item for redo text */
    m_menu.cut.connect(sigc::mem_fun(this, &MainWindow::cut));                                                       /*!< Menu item for cut text */
    m_menu.copy.connect(sigc::mem_fun(this, &MainWindow::copy));                                                     /*!< Menu item for copy text */
    m_menu.paste.connect(sigc::mem_fun(this, &MainWindow::paste));                                                   /*!< Menu item for paste text */
    m_menu.del.connect(sigc::mem_fun(this, &MainWindow::del));                                                       /*!< Menu item for deleting selected text */
    m_menu.select_all.connect(sigc::mem_fun(this, &MainWindow::selectAll));                                          /*!< Menu item for selecting all text */
    m_menu.find.connect(sigc::mem_fun(this, &MainWindow::show_search));                                              /*!< Menu item for finding text */
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
    m_searchEntry.signal_activate().connect(sigc::mem_fun(this, &MainWindow::do_search));                            /*!< Execute the text search */

    m_vbox.pack_start(m_menu, false, false, 0);

    // Editor buttons
    m_openButton.signal_clicked().connect(sigc::mem_fun(this, &MainWindow::open));
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
    m_quoteButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::make_quote));
    m_codeButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::make_code));
    m_bulletListButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::insert_bullet_list));
    m_numberedListButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::insert_numbered_list));
    m_highlightButton.signal_clicked().connect(sigc::mem_fun(m_draw_main, &Draw::make_highlight));

    try
    {
        // Add icons to the editor buttons
        int iconSize = 16;
        m_openIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("open_folder", "folders"), iconSize, iconSize));
        m_openButton.set_tooltip_text("Open document (Ctrl+O)");
        m_openButton.add(m_openIcon);
        m_openButton.set_relief(Gtk::RELIEF_NONE);
        m_saveIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("floppy_disk", "basic"), iconSize, iconSize));
        m_saveButton.set_tooltip_text("Save document (Ctrl+S)");
        m_saveButton.add(m_saveIcon);
        m_saveButton.set_relief(Gtk::RELIEF_NONE);
        m_publishIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("upload", "basic"), iconSize, iconSize));
        m_publishButton.set_tooltip_text("Publish document... (Ctrl+P)");
        m_publishButton.add(m_publishIcon);
        m_publishButton.set_relief(Gtk::RELIEF_NONE);
        m_cutIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("cut", "editor"), iconSize, iconSize));
        m_cutButton.set_tooltip_text("Cut (Ctrl+X)");
        m_cutButton.add(m_cutIcon);
        m_cutButton.set_relief(Gtk::RELIEF_NONE);
        m_copyIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("copy", "editor"), iconSize, iconSize));
        m_copyButton.set_tooltip_text("Copy (Ctrl+C)");
        m_copyButton.add(m_copyIcon);
        m_copyButton.set_relief(Gtk::RELIEF_NONE);
        m_pasteIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("clipboard", "editor"), iconSize, iconSize));
        m_pasteButton.set_tooltip_text("Paste (Ctrl+V)");
        m_pasteButton.add(m_pasteIcon);
        m_pasteButton.set_relief(Gtk::RELIEF_NONE);
        m_undoIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("undo", "editor"), iconSize, iconSize));
        m_undoButton.set_tooltip_text("Undo text (Ctrl+Z)");
        m_undoButton.add(m_undoIcon);
        m_undoButton.set_relief(Gtk::RELIEF_NONE);
        m_redoIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("redo", "editor"), iconSize, iconSize));
        m_redoButton.set_tooltip_text("Redo text (Ctrl+Y)");
        m_redoButton.add(m_redoIcon);
        m_redoButton.set_relief(Gtk::RELIEF_NONE);
        m_boldIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("bold", "editor"), iconSize, iconSize));
        m_boldButton.set_tooltip_text("Add bold text");
        m_boldButton.add(m_boldIcon);
        m_boldButton.set_relief(Gtk::RELIEF_NONE);
        m_italicIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("italic", "editor"), iconSize, iconSize));
        m_italicButton.set_tooltip_text("Add italic text");
        m_italicButton.add(m_italicIcon);
        m_italicButton.set_relief(Gtk::RELIEF_NONE);
        m_strikethroughIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("strikethrough", "editor"), iconSize, iconSize));
        m_strikethroughButton.set_tooltip_text("Add strikethrough text");
        m_strikethroughButton.add(m_strikethroughIcon);
        m_strikethroughButton.set_relief(Gtk::RELIEF_NONE);
        m_superIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("superscript", "editor"), iconSize, iconSize));
        m_superButton.set_tooltip_text("Add superscript text");
        m_superButton.add(m_superIcon);
        m_superButton.set_relief(Gtk::RELIEF_NONE);
        m_subIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("subscript", "editor"), iconSize, iconSize));
        m_subButton.set_tooltip_text("Add subscript text");
        m_subButton.add(m_subIcon);
        m_subButton.set_relief(Gtk::RELIEF_NONE);
        m_linkIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("link", "editor"), iconSize, iconSize));
        m_linkButton.set_tooltip_text("Add a link");
        m_linkButton.add(m_linkIcon);
        m_linkButton.set_relief(Gtk::RELIEF_NONE);
        m_imageIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("shapes", "editor"), iconSize, iconSize));
        m_imageButton.set_tooltip_text("Add a image");
        m_imageButton.add(m_imageIcon);
        m_imageButton.set_relief(Gtk::RELIEF_NONE);
        m_quoteIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("quote", "editor"), iconSize, iconSize));
        m_quoteButton.set_tooltip_text("Insert a quote");
        m_quoteButton.add(m_quoteIcon);
        m_quoteButton.set_relief(Gtk::RELIEF_NONE);
        m_codeIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("code", "editor"), iconSize, iconSize));
        m_codeButton.set_tooltip_text("Insert code");
        m_codeButton.add(m_codeIcon);
        m_codeButton.set_relief(Gtk::RELIEF_NONE);
        m_bulletListIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("bullet_list", "editor"), iconSize, iconSize));
        m_bulletListButton.set_tooltip_text("Add a bullet list");
        m_bulletListButton.add(m_bulletListIcon);
        m_bulletListButton.set_relief(Gtk::RELIEF_NONE);
        m_numberedListIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("number_list", "editor"), iconSize, iconSize));
        m_numberedListButton.set_tooltip_text("Add a numbered list");
        m_numberedListButton.add(m_numberedListIcon);
        m_numberedListButton.set_relief(Gtk::RELIEF_NONE);
        m_hightlightIcon.set(Gdk::Pixbuf::create_from_file(this->getIconImageFromTheme("highlighter", "editor"), iconSize, iconSize));
        m_highlightButton.set_tooltip_text("Add highlight text");
        m_highlightButton.add(m_hightlightIcon);
        m_highlightButton.set_relief(Gtk::RELIEF_NONE);
    }
    catch (const Glib::FileError &e)
    {
        std::cerr << "Error: Icons could not be loaded: " << e.what() << std::endl;
    }

    // Disable focus on editor buttons
    m_openButton.set_can_focus(false);
    m_saveButton.set_can_focus(false);
    m_publishButton.set_can_focus(false);
    m_cutButton.set_can_focus(false);
    m_copyButton.set_can_focus(false);
    m_pasteButton.set_can_focus(false);
    m_undoButton.set_can_focus(false);
    m_redoButton.set_can_focus(false);
    m_headingsComboBox.set_can_focus(false);
    m_headingsComboBox.set_focus_on_click(false);
    m_boldButton.set_can_focus(false);
    m_italicButton.set_can_focus(false);
    m_strikethroughButton.set_can_focus(false);
    m_superButton.set_can_focus(false);
    m_subButton.set_can_focus(false);
    m_linkButton.set_can_focus(false);
    m_imageButton.set_can_focus(false);
    m_quoteButton.set_can_focus(false);
    m_codeButton.set_can_focus(false);
    m_bulletListButton.set_can_focus(false);
    m_numberedListButton.set_can_focus(false);
    m_highlightButton.set_can_focus(false);

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
    m_backButton.set_relief(Gtk::RELIEF_NONE);
    m_forwardButton.set_relief(Gtk::RELIEF_NONE);
    m_refreshButton.set_relief(Gtk::RELIEF_NONE);
    m_homeButton.set_relief(Gtk::RELIEF_NONE);

    // Add icons to the toolbar buttons
    // TODO: Optionally use either the official GTK icons OR 
    //       use our built-in icon-themes for back/forward/refresh and home buttons
    m_backIcon.set_from_icon_name("go-previous", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_backButton.add(m_backIcon);
    m_forwardIcon.set_from_icon_name("go-next", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_forwardButton.add(m_forwardIcon);
    m_refreshIcon.set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_refreshButton.add(m_refreshIcon);
    m_homeIcon.set_from_icon_name("go-home", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    m_homeButton.add(m_homeIcon);

    // Disable back/forward button on start-up
    m_backButton.set_sensitive(false);
    m_forwardButton.set_sensitive(false);

    // Browser Toolbar
    m_backButton.set_margin_left(6);
    m_hboxBrowserToolbar.pack_start(m_backButton, false, false, 0);
    m_hboxBrowserToolbar.pack_start(m_forwardButton, false, false, 0);
    m_hboxBrowserToolbar.pack_start(m_refreshButton, false, false, 0);
    m_hboxBrowserToolbar.pack_start(m_homeButton, false, false, 0);
    m_hboxBrowserToolbar.pack_start(m_addressBar, true, true, 8);
    m_vbox.pack_start(m_hboxBrowserToolbar, false, false, 6);

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
    m_vbox.pack_start(m_hboxStandardEditorToolbar, false, false, 6);

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
    m_hboxFormattingEditorToolbar.pack_start(m_separator4, false, false, 0);
    m_hboxFormattingEditorToolbar.pack_start(m_quoteButton, false, false, 2);
    m_hboxFormattingEditorToolbar.pack_start(m_codeButton, false, false, 2);
    m_hboxFormattingEditorToolbar.pack_start(m_bulletListButton, false, false, 2);
    m_hboxFormattingEditorToolbar.pack_start(m_numberedListButton, false, false, 2);
    m_hboxFormattingEditorToolbar.pack_start(m_highlightButton, false, false, 2);
    m_vbox.pack_start(m_hboxFormattingEditorToolbar, false, false, 6);

    // Browser text main drawing area
    m_scrolledWindowMain.add(m_draw_main);
    m_scrolledWindowMain.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    // Secondary drawing area
    m_draw_secondary.setViewSourceMenuItem(false);
    m_scrolledWindowSecondary.add(m_draw_secondary);
    m_scrolledWindowSecondary.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_scrolledWindowSecondary.set_size_request(450, -1);

    // Bottom Search bar
    m_search.connect_entry(m_searchEntry);
    m_exitBottomButton.set_relief(Gtk::RELIEF_NONE);
    m_exitBottomButton.set_label("\u2716");
    m_exitBottomButton.signal_clicked().connect(sigc::mem_fun(m_hboxBottom, &Gtk::Box::hide));
    m_hboxBottom.pack_start(m_exitBottomButton, false, false, 10);
    m_hboxBottom.pack_start(m_searchEntry, false, false, 10);

    m_paned.pack1(m_scrolledWindowMain, true, false);
    m_paned.pack2(m_scrolledWindowSecondary, false, true);

    m_vbox.pack_start(m_paned, true, true, 0);
    m_vbox.pack_end(m_hboxBottom, false, true, 6);

    add(m_vbox);
    show_all_children();
    // Hide by default the bottom & editor box & secondary text view
    m_hboxBottom.hide();
    m_hboxStandardEditorToolbar.hide();
    m_hboxFormattingEditorToolbar.hide();
    m_scrolledWindowSecondary.hide();

    // Grap focus to input field by default
    m_addressBar.grab_focus();

#ifdef NDEBUG
    // Show start page by default
    go_home();
#else
    // Load test.md file in debug
    doRequest("file://../../test.md", true);
#endif
}

/**
 * Fetch document from disk or IPFS, using threading
 */
void MainWindow::doRequest(const std::string &path, bool setAddressBar, bool isHistoryRequest)
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
        m_requestThread = new std::thread(&MainWindow::processRequest, this, path);
        this->postDoRequest(path, setAddressBar, isHistoryRequest);
    }
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
        m_addressBar.get_selection_bounds(start, end);
        m_addressBar.delete_text(start, end);
    }
    else if (m_searchEntry.has_focus())
    {
        int start, end;
        m_searchEntry.get_selection_bounds(start, end);
        m_searchEntry.delete_text(start, end);
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
}

/**
 * Trigger/creating a new document
 */
void MainWindow::new_doc()
{
    // Enable editing mode
    this->enableEdit();
}

void MainWindow::open()
{
    std::cout << "INFO: TODO" << std::endl;
}

void MainWindow::save()
{
    std::cout << "INFO: TODO" << std::endl;
}

void MainWindow::save_as()
{
    std::cout << "INFO: TODO" << std::endl;
}

void MainWindow::publish()
{
    std::cout << "INFO: TODO" << std::endl;
}

/**
 * Post processing request actions
 */
void MainWindow::postDoRequest(const std::string &path, bool setAddressBar, bool isHistoryRequest)
{
    if (setAddressBar)
        m_addressBar.set_text(path);

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

void MainWindow::go_home()
{
    this->requestPath = "";
    this->finalRequestPath = "";
    this->currentContent = "";
    this->m_addressBar.set_text("");
    this->disableEdit();
    m_draw_main.showStartPage();
}

/**
 * Trigger when pressed enter in the search entry
 */
void MainWindow::do_search()
{
    std::cout << "Search for: " << m_searchEntry.get_text() << std::endl;
}

/**
 * Trigger when pressed enter in the address bar
 */
void MainWindow::address_bar_activate()
{
    doRequest(m_addressBar.get_text());
    // When user actually entered the address bar, focus on the main draw
    m_draw_main.grab_focus();
}

void MainWindow::show_search()
{
    if (m_hboxBottom.is_visible())
    {
        m_hboxBottom.hide();
        m_addressBar.grab_focus();
    }
    else
    {
        m_hboxBottom.show();
        m_searchEntry.grab_focus();
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
    doRequest();
}

void MainWindow::enableEdit()
{
    // Inform the Draw class that we are creating a new document
    this->m_draw_main.newDocument();
    // Show editor toolbars
    this->m_hboxStandardEditorToolbar.show();
    this->m_hboxFormattingEditorToolbar.show();
    // Enabled secondary text view (on the right)
    this->m_scrolledWindowSecondary.show();
    // Disable "view source" menu item
    this->m_draw_main.setViewSourceMenuItem(false);
    // Connect changed signal
    this->textChangedSignalHandler = m_draw_main.get_buffer().get()->signal_changed().connect(sigc::mem_fun(this, &MainWindow::editor_changed_text));
    // Set new title
    set_title("Untitled * - " + m_appName);
}

void MainWindow::disableEdit()
{
    if (m_hboxStandardEditorToolbar.is_visible())
    {
        this->m_hboxStandardEditorToolbar.hide();
        this->m_hboxFormattingEditorToolbar.hide();
        this->m_scrolledWindowSecondary.hide();
        // Disconnect text changed signal
        this->textChangedSignalHandler.disconnect();
        // Show "view source" menu item again
        this->m_draw_main.setViewSourceMenuItem(true);
        this->m_draw_secondary.clearText();
        // Restore title
        set_title(m_appName);
    }
}

/**
 * Get the file from disk or IPFS network, from the provided path,
 * parse the content, and display the document
 */
void MainWindow::processRequest(const std::string &path)
{
    currentContent = "";
    if (!path.empty())
    {
        requestPath = path;
    }
    if (requestPath.empty())
    {
        std::cerr << "Info: Empty request path." << std::endl;
    }
    else
    {
        // Check if CID
        if (requestPath.rfind("ipfs://", 0) == 0)
        {
            finalRequestPath = requestPath;
            finalRequestPath.erase(0, 7);
            fetchFromIPFS();
        }
        else if ((requestPath.length() == 46) && (requestPath.rfind("Qm", 0) == 0))
        {
            // CIDv0
            finalRequestPath = requestPath;
            fetchFromIPFS();
        }
        else if (requestPath.rfind("file://", 0) == 0)
        {
            finalRequestPath = requestPath;
            finalRequestPath.erase(0, 7);
            openFromDisk();
        }
        else
        {
            // IPFS as fallback / CIDv1
            finalRequestPath = requestPath;
            fetchFromIPFS();
        }
    }
}

/**
 * Helper method for processRequest(),
 * Display markdown file from IPFS network.
 */
void MainWindow::fetchFromIPFS()
{
    // TODO: Execute the code in a seperate thread/process?
    //  Since otherwise this may block the UI if it takes too long!
    try
    {
        currentContent = m_file.fetch(finalRequestPath);
        cmark_node *doc = Parser::parseContent(currentContent);
        m_draw_main.processDocument(doc);
        cmark_node_free(doc);
    }
    catch (const std::runtime_error &error)
    {
        std::string errorMessage = std::string(error.what());
        std::cerr << "Error: IPFS request failed, with message: " << errorMessage << std::endl;
        if (errorMessage.starts_with("HTTP request failed with status code"))
        {
            // Remove text until ':\n'
            errorMessage.erase(0, errorMessage.find(':') + 2);
            auto content = nlohmann::json::parse(errorMessage);
            std::string message = content.value("Message", "");
            m_draw_main.showMessage("Page not found!", message);
        }
        else if (errorMessage.starts_with("Couldn't connect to server: Failed to connect to localhost"))
        {
            m_draw_main.showMessage("Please wait...", "IPFS daemon is still spinnng-up, please try to refresh shortly...");
        }
        else
        {
            m_draw_main.showMessage("Something went wrong", "Error message: " + std::string(error.what()));
        }
    }
}

/**
 * Helper method for processRequest(),
 * Display markdown file from disk.
 */
void MainWindow::openFromDisk()
{
    try
    {
        currentContent = m_file.read(finalRequestPath);
        cmark_node *doc = Parser::parseContent(currentContent);
        m_draw_main.processDocument(doc);
        cmark_node_free(doc);
    }
    catch (const std::runtime_error &error)
    {
        std::cerr << "Error: File request failed, with message: " << error.what() << std::endl;
        m_draw_main.showMessage("Page not found!", "Error message: " + std::string(error.what()));
    }
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

void MainWindow::editor_changed_text()
{
    // Retrieve text from text editor
    std::string text = m_draw_main.getText();
    // Parse the markdown contents
    cmark_node *doc = Parser::parseContent(text);
    /*std::string md = Parser::renderMarkdown(doc);
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
    m_sourceCodeDialog.setText(currentContent);
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
            std::cerr << "Error: heading combobox id is invalid (not a number)." << std::endl;
        }
        catch (const std::out_of_range &)
        {
            // ignore
        }
    }
}