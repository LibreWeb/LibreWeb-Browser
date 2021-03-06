#include "draw.h"
#include "node.h"
#include "syntax_extension.h"
#include "strikethrough.h"
#include "mainwindow.h"
#include <gdk/gdkthreads.h>
#include <gdk/gdkselection.h>
#include <gtkmm/textiter.h>
#include <gdkmm/window.h>
#include <iostream>
#include <stdexcept>
#include <regex>

#define PANGO_SCALE_XXX_LARGE ((double)1.98)

struct DispatchData
{
    GtkTextBuffer *buffer;
    // For inserting text
    std::string text;
    std::string url;
    // For removing text
    int charsTruncated;
};

Draw::Draw(MainWindow &mainWindow)
    : mainWindow(mainWindow),
      buffer(Glib::unwrap(this->get_buffer())),
      addViewSourceMenuItem(true),
      fontSize(10 * PANGO_SCALE),
      fontFamily("Ubuntu Monospace"),
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
      defaultFont(fontFamily)
{
    this->disableEdit();
    set_indent(15);
    set_left_margin(10);
    set_right_margin(10);
    set_top_margin(12);
    set_bottom_margin(0);
    set_monospace(true);
    set_app_paintable(true);
    set_pixels_above_lines(1);
    set_pixels_below_lines(1);
    // set_pixels_inside_wrap(1);
    set_wrap_mode(Gtk::WrapMode::WRAP_WORD_CHAR);

    // Set cursors
    auto display = get_display();
    normalCursor = Gdk::Cursor::create(display, "default");
    linkCursor = Gdk::Cursor::create(display, "grab");
    textCursor = Gdk::Cursor::create(display, "text");

    // Connect Signals
    signal_event_after().connect(sigc::mem_fun(this, &Draw::event_after));
    signal_motion_notify_event().connect(sigc::mem_fun(this, &Draw::motion_notify_event));
    signal_populate_popup().connect(sigc::mem_fun(this, &Draw::populate_popup));
}

/**
 * Links can be activated by clicking or touching the screen.
 */
void Draw::event_after(GdkEvent *ev)
{
    gdouble ex, ey;
    Gtk::TextBuffer::iterator iter;
    int x, y;

    if (ev->type == GDK_BUTTON_RELEASE)
    {
        GdkEventButton *event;
        event = (GdkEventButton *)ev;
        if (event->button != GDK_BUTTON_PRIMARY)
            return;
        ex = event->x;
        ey = event->y;
    }
    else if (ev->type == GDK_TOUCH_END)
    {
        GdkEventTouch *event;
        event = (GdkEventTouch *)ev;
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
 * Update the cursor whenever there is a link
 */
bool Draw::motion_notify_event(GdkEventMotion *motion_event)
{
    int x, y;
    window_to_buffer_coords(Gtk::TextWindowType::TEXT_WINDOW_WIDGET, motion_event->x, motion_event->y, x, y);
    this->changeCursor(x, y);
    return false;
}

/**
 * Adapt right-click menu in textview
 */
void Draw::populate_popup(Gtk::Menu *menu)
{
    auto items = menu->get_children();
    for (auto *item : items)
    {
        Gtk::MenuItem *menuItem = static_cast<Gtk::MenuItem *>(item);
        std::string name = menuItem->get_label();
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
        Gtk::MenuItem *sourceCodeMenuItem = Gtk::manage(new Gtk::MenuItem("View Source", true));
        sourceCodeMenuItem->signal_activate().connect(source_code);
        sourceCodeMenuItem->show();
        menu->append(*sourceCodeMenuItem);
    }
}

void Draw::showMessage(const std::string &message, const std::string &detailed_info)
{
    if (get_editable())
        this->disableEdit();
    this->clearOnThread();

    this->headingLevel = 1;
    this->insertText(message);
    this->headingLevel = 0;
    this->insertText(detailed_info);
}

void Draw::showStartPage()
{
    if (get_editable())
        this->disableEdit();
    this->clearOnThread();

    this->headingLevel = 1;
    this->insertText("Welcome to the Decentralized Web (DWeb)");
    this->headingLevel = 0;
    this->insertText("You can surf the web as intended via LibreWeb, by using IPFS as a decentralized solution. This is also the fastest browser in the world.\n\n\
The content is fully written in markdown format, allowing you to easily publish your own site, blog article or e-book.\n\
This browser has even a built-in editor. Check it out in the menu: File->New Document!\n\n");
    this->insertText("See an example page hosted on IPFS: ");
    this->insertLink("Click here for the example", "ipfs://QmQzhn6hEfbYdCfwzYFsSt3eWpubVKA1dNqsgUwci5vHwq");
}

/**
 * Process AST document (markdown format) and draw the text in the GTK TextView
 */
void Draw::processDocument(cmark_node *root_node)
{
    if (get_editable())
        this->disableEdit();
    this->clearOnThread();

    // Loop over AST nodes
    cmark_event_type ev_type;
    cmark_iter *iter = cmark_iter_new(root_node);
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE)
    {
        cmark_node *cur = cmark_iter_get_node(iter);
        try
        {
            processNode(cur, ev_type);
        }
        catch (const std::runtime_error &error)
        {
            std::cerr << "Error: Processing node failed, with message: " << error.what() << std::endl;
            // Continue nevertheless
        }
    }
}

void Draw::setViewSourceMenuItem(bool isEnabled)
{
    this->addViewSourceMenuItem = isEnabled;
}

void Draw::newDocument()
{
    this->clearText();
    enableEdit();
    grab_focus(); // Claim focus on text view
}

std::string Draw::getText()
{
    return get_buffer().get()->get_text();
}

void Draw::clearText()
{
    auto buffer = get_buffer();
    buffer->erase(buffer->begin(), buffer->end());
}

void Draw::enableEdit()
{
    set_editable(true);
    set_cursor_visible(true);
}

void Draw::disableEdit()
{
    set_editable(false);
    set_cursor_visible(false);
}

/**
 * Search for links
 */
void Draw::followLink(Gtk::TextBuffer::iterator &iter)
{
    auto tags = iter.get_tags();
    for (auto const &tag : tags)
    {
        char *url = static_cast<char *>(tag->get_data("url"));
        if (url != 0 && (strlen(url) > 0))
        {
            // Get the URL
            mainWindow.doRequest(url, true);
            break;
        }
    }
}

void Draw::cut()
{
    bool isEditable = get_editable();
    if (isEditable)
    {
        auto buffer = get_buffer();
        auto clipboard = get_clipboard("CLIPBOARD");
        buffer->cut_clipboard(clipboard);
    }
    else
    {
        auto buffer = get_buffer();
        auto clipboard = get_clipboard("CLIPBOARD");
        buffer->copy_clipboard(clipboard);
    }
}

void Draw::copy()
{
    auto buffer = get_buffer();
    auto clipboard = get_clipboard("CLIPBOARD");
    buffer->copy_clipboard(clipboard);
}

void Draw::paste()
{
    bool isEditable = get_editable();
    if (isEditable)
    {
        auto buffer = get_buffer();
        auto clipboard = get_clipboard("CLIPBOARD");
        buffer->paste_clipboard(clipboard);
    }
}

void Draw::del()
{
    bool isEditable = get_editable();
    if (isEditable)
    {
        auto buffer = get_buffer();
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
    }
}

void Draw::selectAll()
{
    auto buffer = get_buffer();
    buffer->select_range(buffer->begin(), buffer->end());
}

/*************************************************************
 * Editor signals calls
 *************************************************************/

void Draw::make_heading(int headingLevel)
{
    Gtk::TextBuffer::iterator start, end;
    auto buffer = get_buffer();
    std::string heading = std::string(headingLevel, '#');
    if (buffer->get_selection_bounds(start, end))
    {
        std::string text = buffer->get_text(start, end);
        buffer->erase_selection();
        buffer->insert_at_cursor(heading + " " + text);
    }
    else
    {
        int insertOffset = buffer->get_insert()->get_iter().get_offset();
        buffer->insert_at_cursor(heading + " \n");
        auto newCursorPos = buffer->get_iter_at_offset(insertOffset + headingLevel + 1);
        buffer->place_cursor(newCursorPos);
    }
}

void Draw::make_bold()
{
    Gtk::TextBuffer::iterator start, end;
    auto buffer = get_buffer();
    if (buffer->get_selection_bounds(start, end))
    {
        std::string text = buffer->get_text(start, end);
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
}

void Draw::make_italic()
{
    Gtk::TextBuffer::iterator start, end;
    auto buffer = get_buffer();
    if (buffer->get_selection_bounds(start, end))
    {
        std::string text = buffer->get_text(start, end);
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
}

void Draw::make_strikethrough()
{
    Gtk::TextBuffer::iterator start, end;
    auto buffer = get_buffer();
    if (buffer->get_selection_bounds(start, end))
    {
        std::string text = buffer->get_text(start, end);
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
}

void Draw::make_super()
{
    Gtk::TextBuffer::iterator start, end;
    auto buffer = get_buffer();
    if (buffer->get_selection_bounds(start, end))
    {
        std::string text = buffer->get_text(start, end);
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
}

void Draw::make_sub()
{
    Gtk::TextBuffer::iterator start, end;
    auto buffer = get_buffer();
    if (buffer->get_selection_bounds(start, end))
    {
        std::string text = buffer->get_text(start, end);
        buffer->erase_selection();
        buffer->insert_at_cursor("~" + text + "~");
    }
    else
    {
        int insertOffset = buffer->get_insert()->get_iter().get_offset();
        buffer->insert_at_cursor("~~");
        auto newCursorPos = buffer->get_iter_at_offset(insertOffset + 1);
        buffer->place_cursor(newCursorPos);
    }
}

void Draw::make_quote()
{
    Gtk::TextBuffer::iterator start, end;
    auto buffer = get_buffer();
    if (buffer->get_selection_bounds(start, end))
    {
        std::string text = buffer->get_text(start, end);
        buffer->erase_selection();
        std::istringstream iss(text);
        std::string line;
        while (std::getline(iss, line))
        {
            buffer->insert_at_cursor(">" + line + "\n");
        }
    }
    else
    {
        buffer->insert_at_cursor("\n> text"); // TODO: only insert new line if there is non before
    }
}

void Draw::insert_link()
{
    Gtk::TextBuffer::iterator start, end;
    auto buffer = get_buffer();
    if (buffer->get_selection_bounds(start, end))
    {
        int insertOffset = buffer->get_insert()->get_iter().get_offset();
        std::string text = buffer->get_text(start, end);
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
}

void Draw::insert_image()
{
    Gtk::TextBuffer::iterator start, end;
    auto buffer = get_buffer();
    if (buffer->get_selection_bounds(start, end))
    {
        std::string text = buffer->get_text(start, end);
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
}

void Draw::make_code()
{
    Gtk::TextBuffer::iterator start, end;
    auto buffer = get_buffer();
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
}

void Draw::insert_bullet_list()
{
    Gtk::TextBuffer::iterator start, end;
    auto buffer = get_buffer();
    if (buffer->get_selection_bounds(start, end))
    {
        std::string text = buffer->get_text(start, end);
        buffer->erase_selection();
        std::istringstream iss(text);
        std::string line;
        while (std::getline(iss, line))
        {
            buffer->insert_at_cursor("* " + line + "\n");
        }
    }
    else
    {
        buffer->insert_at_cursor("\n* ");
    }
}

void Draw::insert_numbered_list()
{
    Gtk::TextBuffer::iterator start, end;
    auto buffer = get_buffer();
    if (buffer->get_selection_bounds(start, end))
    {
        std::string text = buffer->get_text(start, end);
        buffer->erase_selection();
        std::istringstream iss(text);
        std::string line;
        int counter = 1;
        while (std::getline(iss, line))
        {
            buffer->insert_at_cursor(std::to_string(counter) + ". " + line + "\n");
            counter++;
        }
    }
    else
    {
        buffer->insert_at_cursor("\n1. ");
    }
}

void Draw::make_highlight()
{
    Gtk::TextBuffer::iterator start, end;
    auto buffer = get_buffer();
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
}

/************************************************
 * Private methods
 ************************************************/

/**
 * Process and parse each node in the AST
 */
void Draw::processNode(cmark_node *node, cmark_event_type ev_type)
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
            this->insertText("\n");
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
                this->insertText("\n");
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
                    this->insertText(std::string(bulletListLevel, '\u0009') + "\u25e6 ");
                }
                else
                {
                    this->insertText(std::string(bulletListLevel, '\u0009') + "\u2022 ");
                }
            }
            else if (orderedListLevel > 0)
            {
                std::string number;
                if (orderedListLevel % 2 == 0)
                {
                    number = Draw::intToRoman(orderedListCounters[orderedListLevel]) + " ";
                }
                else
                {
                    number = std::to_string(orderedListCounters[orderedListLevel]) + ". ";
                }
                this->insertText(std::string(orderedListLevel, '\u0009') + number);
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
            headingLevel = 0; // reset
        }
        break;

    case CMARK_NODE_CODE_BLOCK:
    {
        std::string code = cmark_node_get_literal(node);
        std::string newline = (isQuote) ? "" : "\n";
        this->insertText(code + newline, CodeTypeEnum::CODE_BLOCK);
    }
    break;

    case CMARK_NODE_HTML_BLOCK:
        break;

    case CMARK_NODE_CUSTOM_BLOCK:
        break;

    case CMARK_NODE_THEMATIC_BREAK:
    {
        this->isBold = true;
        this->insertText("\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\n\n");
        this->isBold = false;
    }
    break;

    case CMARK_NODE_PARAGRAPH:
        // For listings only insert a single new line
        if (!entering && (listLevel > 0))
        {
            this->insertText("\n");
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
            this->insertText("\n\n");
        }
        break;

    case CMARK_NODE_TEXT:
    {
        std::string text = cmark_node_get_literal(node);
        // URL
        if (isLink)
        {
            this->insertLink(text, linkURL);
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
        this->insertText("\n");
        break;

    case CMARK_NODE_SOFTBREAK:
        // only insert space
        this->insertText(" ");
        break;

    case CMARK_NODE_CODE:
    {
        std::string code = cmark_node_get_literal(node);
        this->insertText(code, CodeTypeEnum::INLINE_CODE);
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
        throw std::runtime_error("Node type '" + std::string(cmark_node_get_type_string(node)) + "' not found.");
        break;
    }
}

/**
 * Insert markup text - thread safe
 */
void Draw::insertText(const std::string &input, CodeTypeEnum codeType)
{
    auto font = defaultFont;
    std::string span;
    std::string foreground;
    std::string background;
    const std::string text = this->escapeText(input);

    if (isStrikethrough)
    {
        span.append(" strikethrough=\"true\"");
    }
    if (isSuperscript)
    {
        font.set_size(8000);
        span.append(" rise=\"6000\"");
    }
    if (isSubscript)
    {
        font.set_size(8000);
        span.append(" rise=\"-6000\"");
    }
    if (isBold)
    {
        font.set_weight(Pango::WEIGHT_BOLD);
    }
    if (isItalic)
    {
        font.set_style(Pango::STYLE_ITALIC);
    }
    if (isHighlight)
    {
        foreground = "black";
        background = "#FFFF00";
    }
    if (codeType != Draw::CodeTypeEnum::NONE)
    {
        foreground = "#323232";
        background = "#e0e0e0";
    }
    if (headingLevel > 0)
    {
        font.set_weight(Pango::WEIGHT_BOLD);
        switch (headingLevel)
        {
        case 1:
            font.set_size(fontSize * PANGO_SCALE_XXX_LARGE);
            break;
        case 2:
            font.set_size(fontSize * PANGO_SCALE_XX_LARGE);
            break;
        case 3:
            font.set_size(fontSize * PANGO_SCALE_X_LARGE);
            break;
        case 4:
            font.set_size(fontSize * PANGO_SCALE_LARGE);
            break;
        case 5:
            font.set_size(fontSize * PANGO_SCALE_MEDIUM);
            break;
        case 6:
            font.set_size(fontSize * PANGO_SCALE_MEDIUM);
            foreground = "gray";
            break;
        default:
            break;
        }
    }
    if (isQuote)
    {
        foreground = "blue";
    }
    if (!foreground.empty())
    {
        span.append(" foreground=\"" + foreground + "\"");
    }
    if (!background.empty())
    {
        span.append(" background=\"" + background + "\"");
    }
    span.insert(0, "font_desc=\"" + font.to_string() + "\"");

    if (headingLevel > 0)
    {
        // Special case for headings within quote
        if (isQuote)
            insertMarkupTextOnThread("<span font_desc=\"" + defaultFont.to_string() + "\" foreground=\"blue\">\uFF5C </span><span " + span + ">" + text + "</span><span font_desc=\"" + defaultFont.to_string() + "\" foreground=\"blue\">\n\uFF5C\n</span>");
        // Insert headings the normal way (with break line)
        else
            insertMarkupTextOnThread("<span " + span + ">" + text + "</span>\n\n");
    }
    else
    {
        // Special case for code blocks within quote
        if ((codeType == Draw::CodeTypeEnum::CODE_BLOCK) && isQuote)
        {
            std::istringstream iss(text);
            std::string line;
            // Add a quote for each new code line
            while (getline(iss, line))
            {
                insertMarkupTextOnThread("<span font_desc=\"" + defaultFont.to_string() + "\" foreground=\"blue\">\uFF5C </span><span " + span + ">" + line + "</span>\n");
            }
            insertMarkupTextOnThread("<span font_desc=\"" + defaultFont.to_string() + "\" foreground=\"blue\">\uFF5C\n</span>");
        }
        // Just insert text the normal way
        else
        {
            insertMarkupTextOnThread("<span " + span + ">" + text + "</span>");
        }
    }
}

/**
 * Insert url link - thread safe
 */
void Draw::insertLink(const std::string &name, const std::string &url)
{
    DispatchData *data = g_new0(struct DispatchData, 1);
    data->buffer = buffer;
    data->text = name;
    data->url = url;
    gdk_threads_add_idle((GSourceFunc)insertLinkIdle, data);
}

/**
 * Remove nr. chars from the end of the text buffer - thread safe
 */
void Draw::truncateText(int charsTruncated)
{
    DispatchData *data = g_new0(struct DispatchData, 1);
    data->buffer = buffer;
    data->charsTruncated = charsTruncated;
    gdk_threads_add_idle((GSourceFunc)truncateTextIdle, data);
}

/**
 * Escape input text (eg. ampersand-character)
 * @return Escaped text
 */
std::string const Draw::escapeText(const std::string &input)
{
    return std::regex_replace(input, std::regex("&"), "&amp;");
}

/******************************************************
 * Helper functions below
 *****************************************************/

/**
 * Insert markup pango text - thread safe
 */
void Draw::insertMarkupTextOnThread(const std::string &text)
{
    DispatchData *data = g_new0(struct DispatchData, 1);
    data->buffer = buffer;
    data->text = text;
    gdk_threads_add_idle((GSourceFunc)insertTextIdle, data);
}

/**
 * Clear buffer - thread-safe
 */
void Draw::clearOnThread()
{
    gdk_threads_add_idle((GSourceFunc)clearBufferIdle, buffer);
}

/* Looks at all tags covering the position (x, y) in the text view,
 * and if one of them is a link, change the cursor to the "hands" cursor
 * typically used by web browsers.
 */
void Draw::changeCursor(int x, int y)
{
    Gtk::TextBuffer::iterator iter;
    bool hovering = false;

    get_iter_at_location(iter, x, y);
    auto tags = iter.get_tags();
    for (auto &tag : tags)
    {
        char *url = static_cast<char *>(tag->get_data("url"));
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
 * Insert text on Idle Call function
 */
gboolean Draw::insertTextIdle(struct DispatchData *data)
{
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(data->buffer, &end_iter);
    gtk_text_buffer_insert_markup(data->buffer, &end_iter, data->text.c_str(), -1);
    g_free(data);
    return FALSE;
}

/**
 * Insert link url on Idle Call function
 */
gboolean Draw::insertLinkIdle(struct DispatchData *data)
{
    GtkTextIter end_iter;
    GtkTextTag *tag;
    gtk_text_buffer_get_end_iter(data->buffer, &end_iter);
    tag = gtk_text_buffer_create_tag(data->buffer, NULL,
                                     "foreground", "#569cd6",
                                     "underline", PANGO_UNDERLINE_SINGLE,
                                     NULL);
    g_object_set_data(G_OBJECT(tag), "url", g_strdup(data->url.c_str()));
    gtk_text_buffer_insert_with_tags(data->buffer, &end_iter, data->text.c_str(), -1, tag, NULL);
    g_free(data);
    return FALSE;
}

/**
 * Truncate text from the end of the buffer
 */
gboolean Draw::truncateTextIdle(struct DispatchData *data)
{
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(data->buffer, &end_iter);
    GtkTextIter begin_iter = end_iter;
    gtk_text_iter_backward_chars(&begin_iter, data->charsTruncated);
    gtk_text_buffer_delete(data->buffer, &begin_iter, &end_iter);
    g_free(data);
    return FALSE;
}

/**
 * clearOnThread Text on Idle Call function
 */
gboolean Draw::clearBufferIdle(GtkTextBuffer *textBuffer)
{
    GtkTextIter start_iter, end_iter;
    gtk_text_buffer_get_start_iter(textBuffer, &start_iter);
    gtk_text_buffer_get_end_iter(textBuffer, &end_iter);
    gtk_text_buffer_delete(textBuffer, &start_iter, &end_iter);
    return FALSE;
}

/**
 * Convert number to roman numerals
 */
std::string const Draw::intToRoman(int num)
{
    static const int values[] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
    static const std::string numerals[] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};
    std::string res;
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
