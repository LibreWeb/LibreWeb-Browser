#include "draw.h"
#include "middleware-i.h"
#include "node.h"
#include "syntax_extension.h"
#include <cmark-gfm.h>
#include <cstdint>
#include <gdkmm/window.h>
#include <glibmm.h>
#include <gtkmm/textiter.h>
#include <iostream>
#include <regex>
#include <stdexcept>

Draw::Draw(MiddlewareInterface& middleware)
    : middleware(middleware),
      buffer(Glib::unwrap(this->get_buffer())),
      addViewSourceMenuItem(true),
      headingLevel(0),
      listLevel(0),
      isBold(false),
      isItalic(false),
      isStrikethrough(false),
      isHighlight(false),
      isSuperscript(false),
      isSubscript(false),
      isQuote(false),
      bulletListLevel(0),
      orderedListLevel(0),
      isOrderedList(false),
      isLink(false),
      hovingOverLink(false),
      isUserAction(false)
{
  this->disableEdit();
  set_top_margin(12);
  set_left_margin(10);  // fallback
  set_right_margin(10); // fallback
  set_bottom_margin(0);
  set_monospace(false);
  set_app_paintable(true);
  set_pixels_above_lines(1);
  set_pixels_below_lines(2);
  set_pixels_inside_wrap(2);
  set_wrap_mode(Gtk::WrapMode::WRAP_WORD_CHAR);
  set_has_tooltip();

  // Set cursors
  auto display = get_display();
  normalCursor = Gdk::Cursor::create(display, "default");
  linkCursor = Gdk::Cursor::create(display, "pointer");
  textCursor = Gdk::Cursor::create(display, "text");

  // Create text-tags
  addTags();

  // Connect Signals
  signal_event_after().connect(sigc::mem_fun(this, &Draw::event_after));
  signal_motion_notify_event().connect(sigc::mem_fun(this, &Draw::motion_notify_event));
  signal_query_tooltip().connect(sigc::mem_fun(this, &Draw::query_tooltip));
  signal_populate_popup().connect(sigc::mem_fun(this, &Draw::populate_popup));
}

/**
 * \brief Adding tags.
 * See also: https://gitlab.gnome.org/GNOME/gtkmm/-/blob/master/demos/gtk-demo/example_textview.cc#L100
 */
void Draw::addTags()
{
  auto buffer = get_buffer();
  Glib::RefPtr<Gtk::TextBuffer::Tag> tmpTag;

  // Italic / bold
  buffer->create_tag("italic")->property_style() = Pango::Style::STYLE_ITALIC;
  buffer->create_tag("bold")->property_weight() = Pango::Weight::WEIGHT_BOLD;

  // Add headings
  tmpTag = buffer->create_tag("heading1");
  tmpTag->property_scale() = 2.3;
  tmpTag->property_weight() = Pango::Weight::WEIGHT_BOLD;
  tmpTag = buffer->create_tag("heading2");
  tmpTag->property_scale() = 2.0;
  tmpTag->property_weight() = Pango::Weight::WEIGHT_BOLD;
  tmpTag = buffer->create_tag("heading3");
  tmpTag->property_scale() = 1.8;
  tmpTag->property_weight() = Pango::Weight::WEIGHT_BOLD;
  tmpTag = buffer->create_tag("heading4");
  tmpTag->property_scale() = 1.6;
  tmpTag->property_weight() = Pango::Weight::WEIGHT_BOLD;
  tmpTag = buffer->create_tag("heading5");
  tmpTag->property_scale() = 1.4;
  tmpTag->property_weight() = Pango::Weight::WEIGHT_BOLD;
  tmpTag = buffer->create_tag("heading6");
  tmpTag->property_scale() = 1.3;
  tmpTag->property_weight() = Pango::Weight::WEIGHT_BOLD;
  tmpTag->property_foreground() = "gray";

  // Strikethrough, underline, double underline
  buffer->create_tag("strikethrough")->property_strikethrough() = true;
  buffer->create_tag("underline")->property_underline() = Pango::Underline::UNDERLINE_SINGLE;
  buffer->create_tag("double_underline")->property_underline() = Pango::Underline::UNDERLINE_DOUBLE;

  // Superscript/subscript
  tmpTag = buffer->create_tag("superscript");
  tmpTag->property_rise() = 6 * Pango::SCALE;
  tmpTag->property_scale() = 0.8;
  tmpTag = buffer->create_tag("subscript");
  tmpTag->property_rise() = -6 * Pango::SCALE;
  tmpTag->property_scale() = 0.8;

  // code block
  tmpTag = buffer->create_tag("code");
  tmpTag->property_family() = "monospace";
  tmpTag->property_foreground() = "#323232";
  tmpTag->property_background() = "#e0e0e0";

  // quote
  tmpTag = buffer->create_tag("quote");
  tmpTag->property_foreground() = "blue";
  tmpTag->property_wrap_mode() = Gtk::WrapMode::WRAP_WORD;
  tmpTag->property_indent() = 15;

  // highlight
  tmpTag = buffer->create_tag("highlight");
  tmpTag->property_foreground() = "black";
  tmpTag->property_background() = "#FFFF00";
}

/**
 * \brief Links can be activated by clicking or touching the screen.
 */
void Draw::event_after(GdkEvent* ev)
{
  gdouble ex, ey;
  Gtk::TextBuffer::iterator iter;
  int x, y;

  if (ev->type == GDK_BUTTON_RELEASE)
  {
    GdkEventButton* event;
    event = (GdkEventButton*)ev;
    if (event->button != GDK_BUTTON_PRIMARY)
      return;
    ex = event->x;
    ey = event->y;
  }
  else if (ev->type == GDK_TOUCH_END)
  {
    GdkEventTouch* event;
    event = (GdkEventTouch*)ev;
    ex = event->x;
    ey = event->y;
  }
  else
    return;

  // Get the textview coordinates and retrieve an iterator
  window_to_buffer_coords(Gtk::TextWindowType::TEXT_WINDOW_WIDGET, ex, ey, x, y);
  get_iter_at_location(iter, x, y);
  // Find the links
  followLink(iter);
}

/**
 * \brief Update the cursor whenever there is a link
 */
bool Draw::motion_notify_event(GdkEventMotion* motion_event)
{
  int x, y;
  window_to_buffer_coords(Gtk::TextWindowType::TEXT_WINDOW_WIDGET, motion_event->x, motion_event->y, x, y);
  this->changeCursor(x, y);
  return false;
}

/***
 * \brief Show tooltip when mouse-hover over URL
 */
bool Draw::query_tooltip(int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip)
{
  Gtk::TextIter iter;
  if (keyboard_tooltip)
  {
    auto buffer = get_buffer();
    int offset = buffer->property_cursor_position().get_value();
    iter = buffer->get_iter_at_offset(offset);
  }
  else
  {
    int mouseX, mouseY;
    window_to_buffer_coords(Gtk::TextWindowType::TEXT_WINDOW_WIDGET, x, y, mouseX, mouseY);
    get_iter_at_location(iter, mouseX, mouseY);
  }
  bool found = false;
  auto tags = iter.get_tags();
  for (auto const& tag : tags)
  {
    char* url = static_cast<char*>(tag->get_data("url"));
    if (url != 0 && (strlen(url) > 0))
    {
      // Show link as tooltip
      tooltip->set_markup(url);
      found = true;
      break;
    }
  }
  if (found)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * \brief Adapt right-click menu in textview
 */
void Draw::populate_popup(Gtk::Menu* menu)
{
  auto items = menu->get_children();
  for (auto* item : items)
  {
    Gtk::MenuItem* menuItem = static_cast<Gtk::MenuItem*>(item);
    Glib::ustring name = menuItem->get_label();
    if (name.compare("Cu_t") == 0)
    {
      menuItem->set_label("Cu_t (Ctrl+X)");
    }
    else if (name.compare("_Copy") == 0)
    {
      menuItem->set_label("_Copy (Ctrl+C)");
    }
    else if (name.compare("_Paste") == 0)
    {
      menuItem->set_label("_Paste (Ctrl+V)");
    }
    else if (name.compare("_Delete") == 0)
    {
      menuItem->set_label("_Delete (Del)");
    }
    else if (name.compare("Select _All") == 0)
    {
      menuItem->set_label("Select _All (Ctrl+A)");
    }
    else if (name.compare("Insert _Emoji") == 0)
    {
      item->hide();
    }
  }
  if (this->addViewSourceMenuItem)
  {
    Gtk::MenuItem* sourceCodeMenuItem = Gtk::manage(new Gtk::MenuItem("View Source", true));
    sourceCodeMenuItem->signal_activate().connect(source_code);
    sourceCodeMenuItem->show();
    menu->append(*sourceCodeMenuItem);
  }
}

/**
 * \brief Show a message on screen
 * \param message Headliner
 * \param details Additional text info
 */
void Draw::setMessage(const Glib::ustring& message, const Glib::ustring& details)
{
  if (get_editable())
    this->disableEdit();
  this->clear();

  this->headingLevel = 1;
  this->insertText(message);
  this->headingLevel = 0;
  this->insertMarkupText("\n\n");
  this->insertText(details);
}

/**
 * \brief Draw homepage
 */
void Draw::showHomepage()
{
  if (get_editable())
    this->disableEdit();
  this->clear();

  this->headingLevel = 1;
  this->insertText("Welcome to LibreWeb 🌍🚀");
  this->headingLevel = 0;
  this->insertText("\n\n");
  this->insertMarkupText("Welcome to the decentralized web (also known as web 3.0). Thanks for using LibreWeb!👍\n\n"
                         "LibreWeb is a free &amp; open-source decentralized web browser. With LibreWeb can surf the world-wide-web as originally "
                         "intended, by leveraging IPFS as a decentralized file storage. LibreWeb is also the fastest browser in the world.\n\n"
                         "The content can be fully written in <i>markdown format</i>, allowing you to easily publish your own site, blog article or "
                         "e-book. And markdown makes surfing the web very safe.\n"
                         "This browser has even a <b>built-in editor</b>. Check it out in the menu: <tt>File->New Document</tt>!");
  this->insertText("\n\nSee an example page hosted on IPFS: ");
  this->insertLink("Click here for the example page", "ipfs://QmQQQyYm8GcLBEE7H3NMQWfkyfU5yHiT5i1J98gbfDGRuX");
}

/**
 * \brief Process AST document (markdown format) and draw the text in the GTK TextView
 * The cmark_node pointer will be automatically freed for you.
 * \param rootNode Markdown AST tree that will be displayed on screen
 */
void Draw::setDocument(cmark_node* rootNode)
{
  if (get_editable())
    this->disableEdit();
  this->clear();

  // Loop over AST nodes
  cmark_event_type ev_type;
  cmark_iter* iter = cmark_iter_new(rootNode);
  while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE)
  {
    cmark_node* cur = cmark_iter_get_node(iter);
    try
    {
      processNode(cur, ev_type);
    }
    catch (const std::runtime_error& error)
    {
      std::cerr << "ERROR: Processing node failed, with message: " << error.what() << std::endl;
      // Continue nevertheless
    }
  }
  // Clean-up the memory
  cmark_node_free(rootNode);
}

void Draw::setViewSourceMenuItem(bool isEnabled)
{
  this->addViewSourceMenuItem = isEnabled;
}

/**
 * \brief Prepare for new document
 */
void Draw::newDocument()
{
  this->undoPool.clear();
  this->redoPool.clear();
  this->clear();

  enableEdit();
  grab_focus(); // Claim focus on text view
}

/**
 * \brief Retrieve the current text buffer
 */
Glib::ustring Draw::getText() const
{
  return get_buffer()->get_text();
}

/**
 * \brief Set text in text buffer (for example plain text)
 * \param text Text string
 */
void Draw::setText(const Glib::ustring& text)
{
  get_buffer()->set_text(text);
}

/**
 * Clear text-buffer & clear marks
 */
void Draw::clear()
{
  auto buffer = get_buffer();
  buffer->erase(buffer->begin(), buffer->end());
  for (const Glib::RefPtr<Gtk::TextMark>& mark : headingsToc)
  {
    buffer->delete_mark(mark);
  }
  headingsToc.clear();
}

/**
 * Undo action (Ctrl + Z)
 */
void Draw::undo()
{
  if (get_editable() && (undoPool.size() > 0))
  {
    auto undoAction = undoPool.at(undoPool.size() - 1);
    auto buffer = get_buffer();
    undoPool.pop_back();
    if (undoAction.isInsert)
    {
      Gtk::TextBuffer::iterator startIter = buffer->get_iter_at_offset(undoAction.beginOffset);
      Gtk::TextBuffer::iterator endIter = buffer->get_iter_at_offset(undoAction.endOffset);
      buffer->erase(startIter, endIter);
      buffer->place_cursor(buffer->get_iter_at_offset(undoAction.beginOffset));
    }
    else
    {
      Gtk::TextBuffer::iterator startIter = buffer->get_iter_at_offset(undoAction.beginOffset);
      buffer->insert(startIter, undoAction.text);
      buffer->place_cursor(buffer->get_iter_at_offset(undoAction.endOffset));
    }
    redoPool.push_back(undoAction);
  }
}

/**
 * Redo action (Ctrl + Y)
 */
void Draw::redo()
{
  if (get_editable() && (redoPool.size() > 0))
  {
    auto redoAction = redoPool.at(redoPool.size() - 1);
    auto buffer = get_buffer();
    redoPool.pop_back();
    if (redoAction.isInsert)
    {
      Gtk::TextBuffer::iterator startIter = buffer->get_iter_at_offset(redoAction.beginOffset);
      buffer->insert(startIter, redoAction.text);
      buffer->place_cursor(buffer->get_iter_at_offset(redoAction.endOffset));
    }
    else
    {
      Gtk::TextBuffer::iterator startIter = buffer->get_iter_at_offset(redoAction.beginOffset);
      Gtk::TextBuffer::iterator endIter = buffer->get_iter_at_offset(redoAction.endOffset);
      buffer->erase(startIter, endIter);
      buffer->place_cursor(buffer->get_iter_at_offset(redoAction.beginOffset));
    }
    undoPool.push_back(redoAction);
  }
}

/**
 * \brief Cut text into clipboard
 */
void Draw::cut()
{
  if (get_editable())
  {
    auto clipboard = get_clipboard("CLIPBOARD");
    get_buffer()->cut_clipboard(clipboard);
  }
  else
  {
    auto clipboard = get_clipboard("CLIPBOARD");
    get_buffer()->copy_clipboard(clipboard);
  }
}

/**
 * \brief Copy text into clipboard
 */
void Draw::copy()
{
  auto clipboard = get_clipboard("CLIPBOARD");
  get_buffer()->copy_clipboard(clipboard);
}

/**
 * \brief Paste text from clipboard
 */
void Draw::paste()
{
  if (get_editable())
  {
    auto clipboard = get_clipboard("CLIPBOARD");
    get_buffer()->paste_clipboard(clipboard);
  }
}

/**
 * \brief Delete selected text (only if textview is editable)
 */
void Draw::del()
{
  if (get_editable())
  {
    auto buffer = get_buffer();
    buffer->begin_user_action();
    Gtk::TextBuffer::iterator begin, end;
    if (buffer->get_selection_bounds(begin, end))
    {
      buffer->erase(begin, end);
    }
    else
    {
      ++end;
      buffer->erase(begin, end);
    }
    buffer->end_user_action();
  }
}

/**
 * \brief Select all text
 */
void Draw::selectAll()
{
  auto buffer = get_buffer();
  buffer->select_range(buffer->begin(), buffer->end());
}

/**
 * \brief Return the Texr mark headings for Table of contents
 */
std::vector<Glib::RefPtr<Gtk::TextMark>> Draw::getHeadings()
{
  return headingsToc;
}

/*************************************************************
 * Editor signals calls
 *************************************************************/

void Draw::make_heading(int headingLevel)
{
  Gtk::TextBuffer::iterator start, start_line, end_line, _;
  auto buffer = get_buffer();
  buffer->begin_user_action();
  Glib::ustring heading = Glib::ustring(headingLevel, '#');
  buffer->get_selection_bounds(start, _);

  start_line = buffer->get_iter_at_line(start.get_line());
  // Lookup to 12 places further
  int insertLocation = start_line.get_offset();
  end_line = buffer->get_iter_at_offset(insertLocation + 12);
  std::string text = start_line.get_text(end_line);
  if (!text.empty() && text.starts_with("#"))
  {
    std::size_t countHashes = 0;
    bool hasSpace = false;
    std::size_t len = text.size();
    for (Glib::ustring::size_type i = 0; i < len; i++)
    {
      if (text[i] == '#')
        countHashes++;
      else
        break;
    }
    // Check for next character after the #-signs, is there already a space?
    if (countHashes < len)
    {
      if (text[countHashes] == ' ')
        hasSpace = true;
    }
    Gtk::TextBuffer::iterator delete_iter_end = buffer->get_iter_at_offset(insertLocation + countHashes);
    // Delete hashes at the beginning on the line
    buffer->erase(start_line, delete_iter_end);
    // Buffer is now modified, previous iteraters are now invalid, so get a new iter
    Gtk::TextBuffer::iterator new_start = buffer->get_iter_at_offset(insertLocation);

    // Finally, insert the new heading (add additional space indeed needed)
    Glib::ustring insertHeading = (hasSpace) ? heading : heading + " ";
    buffer->insert(new_start, insertHeading);
  }
  else
  {
    buffer->insert(start_line, heading + " ");
  }
  buffer->end_user_action();
}

void Draw::make_bold()
{
  Gtk::TextBuffer::iterator start, end;
  auto buffer = get_buffer();
  buffer->begin_user_action();
  if (buffer->get_selection_bounds(start, end))
  {
    Glib::ustring text = buffer->get_text(start, end);
    buffer->erase_selection();
    buffer->insert_at_cursor("**" + text + "**");
  }
  else
  {
    int insertOffset = buffer->get_insert()->get_iter().get_offset();
    buffer->insert_at_cursor("****");
    auto newCursorPos = buffer->get_iter_at_offset(insertOffset + 2);
    buffer->place_cursor(newCursorPos);
  }
  buffer->end_user_action();
}

void Draw::make_italic()
{
  Gtk::TextBuffer::iterator start, end;
  auto buffer = get_buffer();
  buffer->begin_user_action();
  if (buffer->get_selection_bounds(start, end))
  {
    Glib::ustring text = buffer->get_text(start, end);
    buffer->erase_selection();
    buffer->insert_at_cursor("*" + text + "*");
  }
  else
  {
    int insertOffset = buffer->get_insert()->get_iter().get_offset();
    buffer->insert_at_cursor("**");
    auto newCursorPos = buffer->get_iter_at_offset(insertOffset + 1);
    buffer->place_cursor(newCursorPos);
  }
  buffer->end_user_action();
}

void Draw::make_strikethrough()
{
  Gtk::TextBuffer::iterator start, end;
  auto buffer = get_buffer();
  buffer->begin_user_action();
  if (buffer->get_selection_bounds(start, end))
  {
    Glib::ustring text = buffer->get_text(start, end);
    buffer->erase_selection();
    buffer->insert_at_cursor("~~" + text + "~~");
  }
  else
  {
    int insertOffset = buffer->get_insert()->get_iter().get_offset();
    buffer->insert_at_cursor("~~~~");
    auto newCursorPos = buffer->get_iter_at_offset(insertOffset + 2);
    buffer->place_cursor(newCursorPos);
  }
  buffer->end_user_action();
}

void Draw::make_super()
{
  Gtk::TextBuffer::iterator start, end;
  auto buffer = get_buffer();
  buffer->begin_user_action();
  if (buffer->get_selection_bounds(start, end))
  {
    Glib::ustring text = buffer->get_text(start, end);
    buffer->erase_selection();
    buffer->insert_at_cursor("^" + text + "^");
  }
  else
  {
    int insertOffset = buffer->get_insert()->get_iter().get_offset();
    buffer->insert_at_cursor("^^");
    auto newCursorPos = buffer->get_iter_at_offset(insertOffset + 1);
    buffer->place_cursor(newCursorPos);
  }
  buffer->end_user_action();
}

void Draw::make_sub()
{
  Gtk::TextBuffer::iterator start, end;
  auto buffer = get_buffer();
  buffer->begin_user_action();
  if (buffer->get_selection_bounds(start, end))
  {
    Glib::ustring text = buffer->get_text(start, end);
    buffer->erase_selection();
    buffer->insert_at_cursor("%" + text + "%");
  }
  else
  {
    int insertOffset = buffer->get_insert()->get_iter().get_offset();
    buffer->insert_at_cursor("%%");
    auto newCursorPos = buffer->get_iter_at_offset(insertOffset + 1);
    buffer->place_cursor(newCursorPos);
  }
  buffer->end_user_action();
}

void Draw::make_quote()
{
  Gtk::TextBuffer::iterator start, end;
  auto buffer = get_buffer();
  buffer->begin_user_action();
  if (buffer->get_selection_bounds(start, end))
  {
    std::string text = buffer->get_text(start, end);
    buffer->erase_selection();
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line))
    {
      buffer->insert_at_cursor("> " + line + "\n");
    }
  }
  else
  {
    buffer->insert_at_cursor("\n> text"); // TODO: only insert new line if there is non before
  }
  buffer->end_user_action();
}

void Draw::insert_link()
{
  Gtk::TextBuffer::iterator start, end;
  auto buffer = get_buffer();
  buffer->begin_user_action();
  if (buffer->get_selection_bounds(start, end))
  {
    int insertOffset = buffer->get_insert()->get_iter().get_offset();
    Glib::ustring text = buffer->get_text(start, end);
    buffer->erase_selection();
    buffer->insert_at_cursor("[" + text + "](ipfs://url)");
    auto beginCursorPos = buffer->get_iter_at_offset(insertOffset + text.length() + 10);
    auto endCursorPos = buffer->get_iter_at_offset(insertOffset + text.length() + 13);
    buffer->select_range(beginCursorPos, endCursorPos);
  }
  else
  {
    int insertOffset = buffer->get_insert()->get_iter().get_offset();
    buffer->insert_at_cursor("[link](ipfs://url)");
    auto beginCursorPos = buffer->get_iter_at_offset(insertOffset + 14);
    auto endCursorPos = buffer->get_iter_at_offset(insertOffset + 17);
    buffer->select_range(beginCursorPos, endCursorPos);
  }
  buffer->end_user_action();
}

void Draw::insert_image()
{
  Gtk::TextBuffer::iterator start, end;
  auto buffer = get_buffer();
  buffer->begin_user_action();
  if (buffer->get_selection_bounds(start, end))
  {
    Glib::ustring text = buffer->get_text(start, end);
    buffer->erase_selection();
    buffer->insert_at_cursor("![](" + text + "]");
  }
  else
  {
    int insertOffset = buffer->get_insert()->get_iter().get_offset();
    buffer->insert_at_cursor("![](ipfs://image.jpg)");
    auto beginCursorPos = buffer->get_iter_at_offset(insertOffset + 11);
    auto endCursorPos = buffer->get_iter_at_offset(insertOffset + 20);
    buffer->select_range(beginCursorPos, endCursorPos);
  }
  buffer->end_user_action();
}

// TODO: set_monospace(true)
void Draw::make_code()
{
  Gtk::TextBuffer::iterator start, end;
  auto buffer = get_buffer();
  buffer->begin_user_action();
  if (buffer->get_selection_bounds(start, end))
  {
    std::string text = buffer->get_text(start, end);
    buffer->erase_selection();
    // Strip begin & end line breaks
    if (text.starts_with('\n'))
    {
      text.erase(0, 1);
    }
    if (text.ends_with('\n'))
    {
      text.erase(text.size() - 1);
    }
    if (text.find('\n') != std::string::npos)
    {
      // Insert code block
      buffer->insert_at_cursor("```\n" + text + "\n```\n");
    }
    else
    {
      // Insert inline code
      buffer->insert_at_cursor("`" + text + "`");
    }
  }
  else
  {
    int insertOffset = buffer->get_insert()->get_iter().get_offset();
    buffer->insert_at_cursor("``");
    auto newCursorPos = buffer->get_iter_at_offset(insertOffset + 1);
    buffer->place_cursor(newCursorPos);
  }
  buffer->end_user_action();
}

void Draw::insert_bullet_list()
{
  Gtk::TextBuffer::iterator start, end;
  auto buffer = get_buffer();
  buffer->begin_user_action();
  bool selected = buffer->get_selection_bounds(start, end);
  if (selected)
  {
    std::string text = buffer->get_text(start, end);
    buffer->erase_selection();
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line))
    {
      // Line already begins with a bullet, remove bullet list item
      if (line.starts_with("* "))
        buffer->insert_at_cursor(line.substr(2) + "\n");
      else if (line.starts_with("*"))
        buffer->insert_at_cursor(line.substr(1) + "\n");
      else if (!line.empty() && (line.find_first_not_of(" \t\n\v\f\r") != std::string::npos))
        buffer->insert_at_cursor("* " + line + "\n");
    }
  }
  else
  {
    int curLineNumber = start.get_line();
    Gtk::TextBuffer::iterator begin_current_line_iter = buffer->get_iter_at_line(curLineNumber);
    if (start.is_start())
    {
      buffer->insert(begin_current_line_iter, "* ");
    }
    else
    {
      Gtk::TextBuffer::iterator end_current_line_iter, prev_lines_iter;
      // Get the end of the line iter
      end_current_line_iter = buffer->get_iter_at_line_offset(curLineNumber, begin_current_line_iter.get_chars_in_line());
      std::string currentLineText = begin_current_line_iter.get_text(end_current_line_iter);
      // Get previous line (if possible)
      prev_lines_iter = buffer->get_iter_at_line(start.get_line() - 1);
      std::string prevLineText = prev_lines_iter.get_text(start);
      if (currentLineText.compare("* ") == 0)
      {
        // remove empty bullet list
        buffer->erase(begin_current_line_iter, end_current_line_iter);
      }
      else // Insert bullet list
      {
        // Was there already a bullet list item? Continue adding bullet item
        if (currentLineText.starts_with("* "))
        {
          // We're still on the current line, new-line is required
          int insertCharOffset = end_current_line_iter.get_offset();
          buffer->insert(end_current_line_iter, "\n* ");
          Gtk::TextBuffer::iterator insert_iter = buffer->get_iter_at_offset(insertCharOffset + 3); // add 3 additional chars
          buffer->place_cursor(insert_iter);
        }
        else if (prevLineText.starts_with("* "))
        {
          buffer->insert(begin_current_line_iter, "* ");
        }
        else // Insert new bullet list
        {
          // Get also the previous two lines (if possible)
          Gtk::TextBuffer::iterator two_prev_lines_iter = buffer->get_iter_at_line(start.get_line() - 2);
          std::string prevTwoLinesText = two_prev_lines_iter.get_text(start);

          std::string additionalNewlines;
          if (prevTwoLinesText.ends_with("\n\n"))
          {
            // No additional lines needed
          }
          else if (prevLineText.ends_with("\n"))
          {
            additionalNewlines = "\n";
          }
          else
          {
            additionalNewlines = "\n\n";
          }
          // Add additional new lines (if needed), before the bullet item
          buffer->insert_at_cursor(additionalNewlines + "* ");
        }
      }
    }
  }
  buffer->end_user_action();
}

void Draw::insert_numbered_list()
{
  Gtk::TextBuffer::iterator start, end;
  auto buffer = get_buffer();
  buffer->begin_user_action();
  bool selected = buffer->get_selection_bounds(start, end);
  if (selected)
  {
    std::string text = buffer->get_text(start, end);
    buffer->erase_selection();
    std::istringstream iss(text);
    std::string line;
    std::smatch match;
    int counter = 1;
    while (std::getline(iss, line))
    {
      // Line already begins with a numbering, remove numbered list item
      if (std::regex_search(line, match, std::regex("^[0-9]+\\. ")))
        buffer->insert_at_cursor(line.substr(match[0].length()) + "\n");
      else if (std::regex_search(line, match, std::regex("^[0-9]+\\.")))
        buffer->insert_at_cursor(line.substr(match[0].length()) + "\n");
      else if (!line.empty() && (line.find_first_not_of(" \t\n\v\f\r") != std::string::npos))
      {
        buffer->insert_at_cursor(std::to_string(counter) + ". " + line + "\n");
        counter++;
      }
    }
  }
  else
  {
    int curLineNumber = start.get_line();
    Gtk::TextBuffer::iterator begin_current_line_iter = buffer->get_iter_at_line(curLineNumber);
    if (start.is_start())
    {
      buffer->insert(begin_current_line_iter, "1. ");
    }
    else
    {
      Gtk::TextBuffer::iterator end_current_line_iter, prev_lines_iter;
      // Get the end of the line iter
      end_current_line_iter = buffer->get_iter_at_line_offset(curLineNumber, begin_current_line_iter.get_chars_in_line());
      std::string currentLineText = begin_current_line_iter.get_text(end_current_line_iter);
      // Get previous line (if possible)
      prev_lines_iter = buffer->get_iter_at_line(start.get_line() - 1);
      std::string prevLineText = prev_lines_iter.get_text(start);
      if (std::regex_match(currentLineText, std::regex("^[0-9]+\\. ")))
      {
        // remove empty numbered list
        buffer->erase(begin_current_line_iter, end_current_line_iter);
      }
      else // Insert numbered list
      {
        std::smatch match;
        // Was there already a numbered list item? Continue adding numbered item
        if (std::regex_search(currentLineText, match, std::regex("^[0-9]+\\. ")))
        {
          try
          {
            int number = std::stoi(match[0]);
            std::string newNumber = std::to_string(++number);
            int insertCharOffset = end_current_line_iter.get_offset();
            // We're still on the current line, new-line is required
            buffer->insert(end_current_line_iter, "\n" + newNumber + ". ");
            Gtk::TextBuffer::iterator insert_iter =
                buffer->get_iter_at_offset(insertCharOffset + 3 + newNumber.length()); // add 3 additional chars + number
            buffer->place_cursor(insert_iter);
          }
          catch (std::invalid_argument& error)
          {
            // Fall-back
            int insertCharOffset = end_current_line_iter.get_offset();
            buffer->insert(end_current_line_iter, "\n1. ");
            Gtk::TextBuffer::iterator insert_iter = buffer->get_iter_at_offset(insertCharOffset + 3); // add 3 additional chars + number
            buffer->place_cursor(insert_iter);
            // Give warning
            std::cerr << "WARN: Couldn't convert heading to a number?" << std::endl;
          }
        }
        else if (std::regex_search(prevLineText, match, std::regex("^[0-9]+\\. ")))
        {
          try
          {
            int number = std::stoi(match[0]);
            std::string newNumber = std::to_string(++number);
            buffer->insert(begin_current_line_iter, newNumber + ". ");
          }
          catch (std::invalid_argument& error)
          {
            // Fall-back
            buffer->insert(begin_current_line_iter, "1. ");
            // Give warning
            std::cerr << "WARN: Couldn't convert heading to a number?" << std::endl;
          }
        }
        else // Insert new numbered list
        {
          // Get also the previous two lines (if possible)
          Gtk::TextBuffer::iterator two_prev_lines_iter = buffer->get_iter_at_line(start.get_line() - 2);
          std::string prevTwoLinesText = two_prev_lines_iter.get_text(start);

          std::string additionalNewlines;
          if (prevTwoLinesText.ends_with("\n\n"))
          {
            // No additional lines needed
          }
          else if (prevLineText.ends_with("\n"))
          {
            additionalNewlines = "\n";
          }
          else
          {
            additionalNewlines = "\n\n";
          }
          // Add additional new lines (if needed), before the bullet item
          buffer->insert_at_cursor(additionalNewlines + "1. ");
        }
      }
    }
  }
  buffer->end_user_action();
}

void Draw::make_highlight()
{
  Gtk::TextBuffer::iterator start, end;
  auto buffer = get_buffer();
  buffer->begin_user_action();
  if (buffer->get_selection_bounds(start, end))
  {
    std::string text = buffer->get_text(start, end);
    buffer->erase_selection();
    buffer->insert_at_cursor("==" + text + "==");
  }
  else
  {
    int insertOffset = buffer->get_insert()->get_iter().get_offset();
    buffer->insert_at_cursor("====");
    auto newCursorPos = buffer->get_iter_at_offset(insertOffset + 2);
    buffer->place_cursor(newCursorPos);
  }
  buffer->end_user_action();
}

void Draw::begin_user_action()
{
  this->isUserAction = true;
}

void Draw::end_user_action()
{
  this->isUserAction = false;
}

/**
 * Triggered when text gets inserted
 */
void Draw::on_insert(const Gtk::TextBuffer::iterator& pos, const Glib::ustring& text, int bytes __attribute__((unused)))
{
  if (this->isUserAction)
  {
    UndoRedoData undoData = {};
    undoData.isInsert = true;
    undoData.beginOffset = pos.get_offset();
    undoData.endOffset = pos.get_offset() + text.size();
    undoData.text = text;
    this->undoPool.push_back(undoData);
    this->redoPool.clear();
  }
}

/**
 * Triggered when text gets deleted/removed
 */
void Draw::on_delete(const Gtk::TextBuffer::iterator& range_start, const Gtk::TextBuffer::iterator& range_end)
{
  if (this->isUserAction)
  {
    auto text = get_buffer()->get_text(range_start, range_end);
    UndoRedoData undoData = {};
    undoData.isInsert = false;
    undoData.beginOffset = range_start.get_offset();
    undoData.endOffset = range_end.get_offset();
    undoData.text = text;
    this->undoPool.push_back(undoData);
  }
}

/************************************************
 * Private methods
 ************************************************/

void Draw::enableEdit()
{
  set_editable(true);
  set_cursor_visible(true);
  auto buffer = get_buffer();
  this->beginUserActionSignalHandler = buffer->signal_begin_user_action().connect(sigc::mem_fun(this, &Draw::begin_user_action), false);
  this->endUserActionSignalHandler = buffer->signal_end_user_action().connect(sigc::mem_fun(this, &Draw::end_user_action), false);
  this->insertTextSignalHandler = buffer->signal_insert().connect(sigc::mem_fun(this, &Draw::on_insert), false);
  this->deleteTextSignalHandler = buffer->signal_erase().connect(sigc::mem_fun(this, &Draw::on_delete), false);
}

void Draw::disableEdit()
{
  set_editable(false);
  set_cursor_visible(false);
  // Disconnect signal handles
  this->beginUserActionSignalHandler.disconnect();
  this->endUserActionSignalHandler.disconnect();
  this->insertTextSignalHandler.disconnect();
  this->deleteTextSignalHandler.disconnect();
}

/**
 * Search for links
 */
void Draw::followLink(Gtk::TextBuffer::iterator& iter)
{
  auto tags = iter.get_tags();
  for (auto const& tag : tags)
  {
    char* url = static_cast<char*>(tag->get_data("url"));
    if (url != 0 && (strlen(url) > 0))
    {
      // Got to URL
      middleware.doRequest(url);
      break;
    }
  }
}

/**
 * Process and parse each node in the AST
 */
void Draw::processNode(cmark_node* node, cmark_event_type ev_type)
{
  bool entering = (ev_type == CMARK_EVENT_ENTER);

  // Take care of the markdown extensions
  if (node->extension)
  {
    if (strcmp(node->extension->name, "strikethrough") == 0)
    {
      isStrikethrough = entering;
      return;
    }
    else if (strcmp(node->extension->name, "highlight") == 0)
    {
      isHighlight = entering;
      return;
    }
    else if (strcmp(node->extension->name, "superscript") == 0)
    {
      isSuperscript = entering;
      return;
    }
    else if (strcmp(node->extension->name, "subscript") == 0)
    {
      isSubscript = entering;
      return;
    }
  }

  switch (node->type)
  {
  case CMARK_NODE_DOCUMENT:
    if (entering)
    {
      // Reset all (better safe than sorry)
      headingLevel = 0;
      bulletListLevel = 0;
      orderedListLevel = 0;
      listLevel = 0;
      isOrderedList = false;
      isBold = false;
      isItalic = false;
      isStrikethrough = false;
      isHighlight = false;
      isSuperscript = false;
      isSubscript = false;
      isQuote = false;
    }
    break;

  case CMARK_NODE_BLOCK_QUOTE:
    isQuote = entering;
    if (!entering)
    {
      // Replace last quote '|'-sign with a normal blank line
      this->truncateText(2);
      this->insertMarkupText("\n");
    }
    break;

  case CMARK_NODE_LIST:
  {
    cmark_list_type listType = node->as.list.list_type;

    if (entering)
    {
      listLevel++;
    }
    else
    {
      listLevel--;
    }
    if (listLevel == 0)
    {
      // Reset bullet/ordered levels
      bulletListLevel = 0;
      orderedListLevel = 0;
      isOrderedList = false;
      if (!entering)
        this->insertMarkupText("\n");
    }
    else if (listLevel > 0)
    {
      if (entering)
      {
        if (listType == cmark_list_type::CMARK_BULLET_LIST)
        {
          bulletListLevel++;
        }
        else if (listType == cmark_list_type::CMARK_ORDERED_LIST)
        {
          orderedListLevel++;
          // Create the counter (and reset to zero)
          orderedListCounters[orderedListLevel] = 0;
        }
      }
      else
      {
        // Un-indent list level again
        if (listType == cmark_list_type::CMARK_BULLET_LIST)
        {
          bulletListLevel--;
        }
        else if (listType == cmark_list_type::CMARK_ORDERED_LIST)
        {
          orderedListLevel--;
        }
      }

      isOrderedList = (orderedListLevel > 0) && (bulletListLevel <= 0);
    }
  }
  break;

  case CMARK_NODE_ITEM:
    if (entering)
    {
      if (isOrderedList)
      {
        // Increasement ordered list counter
        orderedListCounters[orderedListLevel]++;
      }

      // Insert tabs & bullet/number
      if (bulletListLevel > 0)
      {
        if (bulletListLevel % 2 == 0)
        {
          this->insertText(Glib::ustring(bulletListLevel, '\u0009') + "\u25e6 ");
        }
        else
        {
          this->insertText(Glib::ustring(bulletListLevel, '\u0009') + "\u2022 ");
        }
      }
      else if (orderedListLevel > 0)
      {
        Glib::ustring number;
        if (orderedListLevel % 2 == 0)
        {
          number = Draw::intToRoman(orderedListCounters[orderedListLevel]) + " ";
        }
        else
        {
          number = std::to_string(orderedListCounters[orderedListLevel]) + ". ";
        }
        this->insertText(Glib::ustring(orderedListLevel, '\u0009') + number);
      }
    }
    break;

  case CMARK_NODE_HEADING:
    if (entering)
    {
      headingLevel = node->as.heading.level;
    }
    else
    {
      // Insert line break after heading
      this->insertMarkupText("\n\n");
      headingLevel = 0; // reset
    }
    break;

  case CMARK_NODE_CODE_BLOCK:
  {
    Glib::ustring code = cmark_node_get_literal(node);
    Glib::ustring newline = (isQuote) ? "" : "\n";
    this->insertText(code + newline, "", CodeTypeEnum::CODE_TYPE_CODE_BLOCK);
  }
  break;

  case CMARK_NODE_HTML_BLOCK:
    break;

  case CMARK_NODE_CUSTOM_BLOCK:
    break;

  case CMARK_NODE_THEMATIC_BREAK:
  {
    this->isBold = true;
    this->insertText("\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015"
                     "\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\n\n");
    this->isBold = false;
  }
  break;

  case CMARK_NODE_PARAGRAPH:
    // For listings only insert a single new line
    if (!entering && (listLevel > 0))
    {
      this->insertMarkupText("\n");
    }
    // Dealing with paragraphs in quotes
    else if (entering && isQuote)
    {
      this->insertText("\uFF5C ");
    }
    else if (!entering && isQuote)
    {
      this->insertText("\n\uFF5C\n");
    }
    // Normal paragraph, just blank line
    else if (!entering)
    {
      this->insertMarkupText("\n\n");
    }
    break;

  case CMARK_NODE_TEXT:
  {
    Glib::ustring text = cmark_node_get_literal(node);
    // URL
    if (isLink)
    {
      this->insertText(text, linkURL);
      linkURL = "";
    }
    // Text (with optional inline formatting)
    else
    {
      this->insertText(text);
    }
  }
  break;

  case CMARK_NODE_LINEBREAK:
    // Hard brake
    this->insertMarkupText("\n");
    break;

  case CMARK_NODE_SOFTBREAK:
    // only insert space
    this->insertMarkupText(" ");
    break;

  case CMARK_NODE_CODE:
  {
    Glib::ustring code = cmark_node_get_literal(node);
    this->insertText(code, "", CodeTypeEnum::CODE_TYPE_INLINE_CODE);
  }
  break;

  case CMARK_NODE_HTML_INLINE:
    break;

  case CMARK_NODE_CUSTOM_INLINE:
    break;

  case CMARK_NODE_STRONG:
    isBold = entering;
    break;

  case CMARK_NODE_EMPH:
    isItalic = entering;
    break;

  case CMARK_NODE_LINK:
    isLink = entering;
    if (entering)
    {
      linkURL = cmark_node_get_url(node);
    }
    break;

  case CMARK_NODE_IMAGE:
    break;

  case CMARK_NODE_FOOTNOTE_REFERENCE:
    break;

  case CMARK_NODE_FOOTNOTE_DEFINITION:
    break;
  default:
    throw std::runtime_error("Node type '" + Glib::ustring(cmark_node_get_type_string(node)) + "' not found.");
    break;
  }
}

/******************************************************
 * Helper functions below
 *****************************************************/
/**
 * Encode text string (eg. ampersand-character)
 * @param[in/out] string
 */
void Draw::encodeText(std::string& string) const
{
  std::string buffer;
  buffer.reserve(string.size() + 5);
  for (size_t pos = 0; pos != string.size(); ++pos)
  {
    switch (string[pos])
    {
    case '&':
      buffer.append("&amp;");
      break;
    default:
      buffer.append(&string[pos], 1);
      break;
    }
  }
  string.swap(buffer);
}

/**
 * Insert markup text (markup only works if you first parsed the text, otherwise use the insertMarkupText method)
 */
void Draw::insertText(std::string text, const Glib::ustring& url, CodeTypeEnum codeType)
{
  std::vector<Glib::ustring> tagNames;

  // Use by reference to replace the string
  // TODO: For normal text, you want to bring back the '&amp;' to '&' symbol again
  this->encodeText(text);

  if (isStrikethrough)
  {
    tagNames.push_back("strikethrough");
  }
  if (isSuperscript)
  {
    tagNames.push_back("superscript");
  }
  // You can not have superscript & subscript applied together
  else if (isSubscript)
  {
    tagNames.push_back("subscript");
  }
  if (isBold)
  {
    tagNames.push_back("bold");
  }
  if (isItalic)
  {
    tagNames.push_back("italic");
  }
  if (isHighlight)
  {
    tagNames.push_back("highlight");
  }
  if (codeType != Draw::CodeTypeEnum::CODE_TYPE_NONE)
  {
    tagNames.push_back("code");
  }
  if (headingLevel > 0)
  {
    switch (headingLevel)
    {
    case 1:
      tagNames.push_back("heading1");
      break;
    case 2:
      tagNames.push_back("heading2");
      break;
    case 3:
      tagNames.push_back("heading3");
      break;
    case 4:
      tagNames.push_back("heading4");
      break;
    case 5:
      tagNames.push_back("heading5");
      break;
    case 6:
      tagNames.push_back("heading6");
      break;
    default:
      break;
    }
    // Already add the mark for the heading
    addHeadingMark(text, headingLevel);
  }
  if (isQuote)
  {
    tagNames.push_back("quote");
  }

  // Insert text (+ headings)
  if (url.empty())
  {
    // Special case for code blocks within quote
    if ((codeType == Draw::CodeTypeEnum::CODE_TYPE_CODE_BLOCK) && isQuote)
    {
      std::istringstream iss(text);
      std::string line;
      // Add a quote for each new code line
      while (getline(iss, line))
      {
        insertTagText("\uFF5C ", "quote");
        insertTagText(line + "\n", tagNames);
      }
      insertTagText("\uFF5C\n", "quote");
    }
    // Special case for heading within quote
    else if ((headingLevel > 0) && isQuote)
    {
      insertTagText("\uFF5C ", "quote");
      insertTagText(text, tagNames);
    }
    else
    {
      // Just insert text (default) - which includes headings as well
      insertTagText(text, tagNames);
    }
  }
  // Insert URL
  else
  {
    insertLink(text, url);
  }
}

/**
 * Insert pango text with multiple tags
 */
void Draw::insertTagText(const Glib::ustring& text, std::vector<Glib::ustring> const& tagNames)
{
  auto buffer = get_buffer();
  auto endIter = buffer->end();
  buffer->insert_with_tags_by_name(endIter, text, tagNames);
}

/**
 * \brief Add mark for heading (ToC)
 */
void Draw::addHeadingMark(const Glib::ustring& text, int headingLevel)
{
  auto buffer = get_buffer();
  auto endIter = buffer->end();
  // Make anonymous marks, to avoid existing naming conflicts
  Glib::RefPtr<Gtk::TextMark> textMark = Gtk::TextMark::create();
  textMark->set_data("name", g_strdup(text.c_str()));
  textMark->set_data("level", (void*)(intptr_t)headingLevel);
  buffer->add_mark(textMark, endIter);
  headingsToc.push_back(textMark);
}

/**
 * Insert pango text with a single tag name
 */
void Draw::insertTagText(const Glib::ustring& text, const Glib::ustring& tagName)
{
  auto buffer = get_buffer();
  auto endIter = buffer->end();
  buffer->insert_with_tag(endIter, text, tagName);
}

/**
 * Insert markup pango text
 */
void Draw::insertMarkupText(const Glib::ustring& text)
{
  auto buffer = get_buffer();
  auto endIter = buffer->end();
  buffer->insert_markup(endIter, text);
}

/**
 * Insert url link
 */
void Draw::insertLink(const Glib::ustring& text, const Glib::ustring& url)
{
  auto buffer = get_buffer();
  auto endIter = buffer->end();
  auto tag = buffer->create_tag();
  // TODO: Create a tag name with name "url" and reuse tag if possible.
  tag->property_foreground() = "#569cd6";
  tag->property_underline() = Pango::Underline::UNDERLINE_SINGLE;
  tag->set_data("url", g_strdup(url.c_str()));
  buffer->insert_with_tag(endIter, text, tag);
}

/**
 * Remove nr. chars from the end of the text buffer
 */
void Draw::truncateText(int charsTruncated)
{
  auto buffer = get_buffer();
  auto endIter = buffer->end();
  auto beginIter = endIter;
  beginIter.backward_chars(charsTruncated);
  buffer->erase(beginIter, endIter);
}

/**
 *  Looks at all tags covering the position (x, y) in the text view,
 * and if one of them is a link, change the cursor to the "hands" cursor
 * typically used by web browsers.
 */
void Draw::changeCursor(int x, int y)
{
  Gtk::TextBuffer::iterator iter;
  bool hovering = false;

  get_iter_at_location(iter, x, y);
  auto tags = iter.get_tags();
  for (auto const& tag : tags)
  {
    char* url = static_cast<char*>(tag->get_data("url"));
    if (url != 0 && (strlen(url) > 0))
    {
      // Link
      hovering = true;
      break;
    }
  }

  if (hovering != hovingOverLink)
  {
    hovingOverLink = hovering;
    auto window = get_window(Gtk::TextWindowType::TEXT_WINDOW_TEXT);
    if (hovingOverLink)
      window->set_cursor(linkCursor);
    else
      window->set_cursor(normalCursor);
  }
}

/**
 * Convert number to roman numerals
 */
Glib::ustring Draw::intToRoman(int num)
{
  static const int values[] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
  static const Glib::ustring numerals[] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};
  Glib::ustring res;
  for (int i = 0; i < 13; ++i)
  {
    while (num >= values[i])
    {
      num -= values[i];
      res += numerals[i];
    }
  }
  return res;
}
