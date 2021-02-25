#include "draw.h"
#include "node.h"
#include "strikethrough.h"
#include "mainwindow.h"
#include <gdk/gdkthreads.h>
#include <gdk/gdkselection.h>
#include <gtkmm/textiter.h>
#include <iostream>
#include <stdexcept>

#define PANGO_SCALE_XXX_LARGE ((double)1.98)

struct DispatchData
{
    GtkTextBuffer *buffer;
    std::string text;
    std::string url;
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
      bulletListLevel(0),
      orderedListLevel(0),
      isOrderedList(false),
      isLink(false),
      defaultFont(fontFamily),
      bold(fontFamily),
      italic(fontFamily),
      boldItalic(fontFamily),
      heading1(fontFamily),
      heading2(fontFamily),
      heading3(fontFamily),
      heading4(fontFamily)
{
    this->disableEdit();
    set_indent(15);
    set_left_margin(10);
    set_right_margin(10);
    set_top_margin(5);
    set_bottom_margin(5);
    set_monospace(true);
    set_app_paintable(true);

    defaultFont.set_size(fontSize);
    bold.set_size(fontSize);
    bold.set_weight(Pango::WEIGHT_BOLD);
    italic.set_size(fontSize);
    italic.set_style(Pango::Style::STYLE_ITALIC);
    boldItalic.set_size(fontSize);
    boldItalic.set_weight(Pango::WEIGHT_BOLD);
    boldItalic.set_style(Pango::Style::STYLE_ITALIC);

    heading1.set_size(fontSize * PANGO_SCALE_XXX_LARGE);
    heading1.set_weight(Pango::WEIGHT_BOLD);
    heading2.set_size(fontSize * PANGO_SCALE_XX_LARGE);
    heading2.set_weight(Pango::WEIGHT_BOLD);
    heading3.set_size(fontSize * PANGO_SCALE_X_LARGE);
    heading3.set_weight(Pango::WEIGHT_BOLD);
    heading4.set_size(fontSize * PANGO_SCALE_LARGE);
    heading4.set_weight(Pango::WEIGHT_BOLD);
    heading5.set_size(fontSize * PANGO_SCALE_MEDIUM);
    heading5.set_weight(Pango::WEIGHT_BOLD);
    heading6.set_size(fontSize * PANGO_SCALE_MEDIUM);
    heading6.set_weight(Pango::WEIGHT_BOLD);

    // Connect Signals
    signal_event_after().connect(sigc::mem_fun(this, &Draw::event_after));
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

    insertHeading1(message);
    insertText(detailed_info);
}

void Draw::showStartPage()
{
    if (get_editable())
        this->disableEdit();
    this->clearOnThread();

    insertHeading1("Welcome to the Decentralized Web (DWeb)");
    insertText("See also the: ");
    insertLink("Example page on IPFS", "ipfs://QmQzhn6hEfbYdCfwzYFsSt3eWpubVKA1dNqsgUwci5vHwq");
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

void Draw::selectAll()
{
    if (has_focus())
    {
        auto buffer = get_buffer();
        buffer->select_range(buffer->begin(), buffer->end());
    }
}

void Draw::cut()
{
    if (has_focus())
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
}

void Draw::copy()
{
    if (has_focus())
    {
        auto buffer = get_buffer();
        auto clipboard = get_clipboard("CLIPBOARD");
        buffer->copy_clipboard(clipboard);
    }
}

void Draw::paste()
{
    bool isEditable = get_editable();
    bool hasFocus = has_focus();
    if (isEditable && hasFocus)
    {
        auto buffer = get_buffer();
        auto clipboard = get_clipboard("CLIPBOARD");
        buffer->paste_clipboard(clipboard);
    }
}

void Draw::del()
{
    bool isEditable = get_editable();
    bool hasFocus = has_focus();
    if (isEditable && hasFocus)
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

/*************************************************************
 * Editor signals calls
 *************************************************************/

void Draw::make_heading(int headingLevel)
{
    auto buffer = get_buffer();
    std::string heading = std::string(headingLevel, '#');
    buffer->insert_at_cursor("\n" + heading + " ");
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
        buffer->insert_at_cursor("\n> text");
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

    switch (node->type)
    {
    case CMARK_NODE_DOCUMENT:
        if (entering)
        {
            // Reset
            headingLevel = 0;
            bulletListLevel = 0;
            listLevel = 0;
        }
        break;

    case CMARK_NODE_BLOCK_QUOTE:
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
            // Last list level new line
            if (listLevel == 1)
            {
                insertText("\n");
            }
            listLevel--;
        }
        if (listLevel == 0)
        {
            // Reset bullet/ordered levels
            bulletListLevel = 0;
            orderedListLevel = 0;
            isOrderedList = false;
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
                // TODO: Un-indent list level again
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
        // Line break for each item
        if (entering)
            insertText("\n");
        if (entering && isOrderedList)
        {
            // Increasement ordered list counter
            orderedListCounters[orderedListLevel]++;
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
        insertCode("\n" + code);
    }
    break;

    case CMARK_NODE_HTML_BLOCK:
        break;

    case CMARK_NODE_CUSTOM_BLOCK:
        break;

    case CMARK_NODE_THEMATIC_BREAK:
    {
        insertText("\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015\u2015");
    }
    break;

    case CMARK_NODE_PARAGRAPH:
        if (listLevel == 0)
        {
            // insert new line, but not when listing is enabled
            insertText("\n");
        }
        break;

    case CMARK_NODE_TEXT:
    {
        // Instead of creating seperate pango layouts we may want to use Pango attributes,
        // for changing parts of the text. Which is most likely be faster.
        // https://developer.gnome.org/pango/stable/pango-Text-Attributes.html
        // Pango is using a simple parser for parsing (X)HTML:
        // https://developer.gnome.org/glib/stable/glib-Simple-XML-Subset-Parser.html
        // We can use simular parse functions and using their own 'OpenTag' struct containing a list of Pango attributes:
        // https://gitlab.gnome.org/GNOME/pango/-/blob/master/pango/pango-markup.c#L515

        std::string text = cmark_node_get_literal(node);
        // Insert tabs & bullet/number
        if (bulletListLevel > 0)
        {
            text.insert(0, std::string(bulletListLevel, '\u0009') + "\u2022 ");
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
            text.insert(0, std::string(orderedListLevel, '\u0009') + number);
        }

        // Unsert headings
        if (headingLevel > 0)
        {
            switch (headingLevel)
            {
            case 1:
                insertHeading1(text);
                break;
            case 2:
                insertHeading2(text);
                break;
            case 3:
                insertHeading3(text);
                break;
            case 4:
                insertHeading4(text);
                break;
            case 5:
                insertHeading5(text);
                break;
            case 6:
                insertHeading6(text);
                break;
            default:
                insertHeading5(text); // fallback
                break;
            }
        }
        // Bold/italic text
        else if (isBold && isItalic)
        {
            insertBoldItalic(text);
        }
        else if (isBold)
        {
            insertBold(text);
        }
        else if (isItalic)
        {
            insertItalic(text);
        }
        // URL
        else if (isLink)
        {
            insertLink(text, linkURL);
            linkURL = "";
        }
        else
        {
            // Normal text only
            insertText(text);
        }
    }
    break;

    case CMARK_NODE_LINEBREAK:
        // Hard brake
        insertText("\n");
        break;

    case CMARK_NODE_SOFTBREAK:
        // only insert space
        insertText(" ");
        break;

    case CMARK_NODE_CODE:
    {
        std::string code = cmark_node_get_literal(node);
        insertCode(code);
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

    //case CMARK_NODE_STRIKETHROUGH:
        //break;

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
        throw std::runtime_error("Node type not found. Type (int): " + std::to_string(node->type));
        break;
    }
}

/**
 * Insert markup text - thread safe
 */
void Draw::insertText(const std::string &text)
{
    insertMarkupTextOnThread("<span font_desc=\"" + defaultFont.to_string() + "\">" + text + "</span>");
}

/**
 * Insert code - thread safe
 */
void Draw::insertCode(const std::string &code)
{
    insertMarkupTextOnThread("<span foreground=\"#323232\" background=\"#e0e0e0\">" + code + "</span>");
}

/**
 * Insert url link - thread safe
 */
void Draw::insertLink(const std::string &text, const std::string &url)
{
    DispatchData *data = g_new0(struct DispatchData, 1);
    data->buffer = buffer;
    data->text = text;
    data->url = url;
    gdk_threads_add_idle((GSourceFunc)insertLinkIdle, data);
}

void Draw::insertHeading1(const std::string &text)
{
    insertMarkupTextOnThread("\n<span font_desc=\"" + heading1.to_string() + "\">" + text + "</span>\n");
}

void Draw::insertHeading2(const std::string &text)
{
    insertMarkupTextOnThread("\n<span font_desc=\"" + heading2.to_string() + "\">" + text + "</span>\n");
}

void Draw::insertHeading3(const std::string &text)
{
    insertMarkupTextOnThread("\n<span font_desc=\"" + heading3.to_string() + "\">" + text + "</span>\n");
}

void Draw::insertHeading4(const std::string &text)
{
    insertMarkupTextOnThread("\n<span font_desc=\"" + heading4.to_string() + "\">" + text + "</span>\n");
}

void Draw::insertHeading5(const std::string &text)
{
    insertMarkupTextOnThread("\n<span font_desc=\"" + heading5.to_string() + "\">" + text + "</span>\n");
}

void Draw::insertHeading6(const std::string &text)
{
    insertMarkupTextOnThread("\n<span foreground=\"gray\" font_desc=\"" + heading6.to_string() + "\">" + text + "</span>\n");
}

void Draw::insertBold(const std::string &text)
{
    insertMarkupTextOnThread("<span font_desc=\"" + bold.to_string() + "\">" + text + "</span>");
}

void Draw::insertItalic(const std::string &text)
{
    insertMarkupTextOnThread("<span font_desc=\"" + italic.to_string() + "\">" + text + "</span>");
}

void Draw::insertBoldItalic(const std::string &text)
{
    insertMarkupTextOnThread("<span font_desc=\"" + boldItalic.to_string() + "\">" + text + "</span>");
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
