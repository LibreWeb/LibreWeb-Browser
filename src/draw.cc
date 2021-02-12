#include "draw.h"
#include <gdk/gdkthreads.h>
#include "node.h"
#define PANGO_SCALE_XXX_LARGE ((double)1.98)

struct DispatchData
{
    GtkTextBuffer *buffer;
    std::string text;
};

Draw::Draw() : fontSize(10),
               fontFamily("Ubuntu"),
               headingLevel(0),
               listLevel(0),
               isBold(false),
               isItalic(false),
               bulletListLevel(0),
               orderedListLevel(0),
               isOrderedList(false),
               defaultFont(fontFamily),
               bold(fontFamily),
               italic(fontFamily),
               boldItalic(fontFamily),
               heading1(fontFamily),
               heading2(fontFamily),
               heading3(fontFamily),
               heading4(fontFamily)
{
    set_editable(false);
    set_indent(15);
    set_left_margin(10);
    set_right_margin(10);
    set_top_margin(5);
    set_bottom_margin(5);
    set_monospace(true);
    set_cursor_visible(false);
    set_app_paintable(true);

    defaultFont.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_MEDIUM);
    bold.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_MEDIUM);
    bold.set_weight(Pango::WEIGHT_BOLD);
    italic.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_MEDIUM);
    italic.set_style(Pango::Style::STYLE_ITALIC);
    boldItalic.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_MEDIUM);
    boldItalic.set_weight(Pango::WEIGHT_BOLD);
    boldItalic.set_style(Pango::Style::STYLE_ITALIC);

    heading1.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_XXX_LARGE);
    heading1.set_weight(Pango::WEIGHT_BOLD);
    heading2.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_XX_LARGE);
    heading2.set_weight(Pango::WEIGHT_BOLD);
    heading3.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_X_LARGE);
    heading3.set_weight(Pango::WEIGHT_BOLD);
    heading4.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_LARGE);
    heading4.set_weight(Pango::WEIGHT_BOLD);
}

void Draw::showMessage(const std::string &message, const std::string &detailed_info)
{
    this->clear();

    addHeading1(message);
    addText(detailed_info);
}

/**
 * Show start page
 */
void Draw::showStartPage()
{
    this->clear();

    addHeading1("Welcome to the Decentralized Web (DWeb)");
    addText("For the test example, go to: ipfs://QmQzhn6hEfbYdCfwzYFsSt3eWpubVKA1dNqsgUwci5vHwq");
}

/**
 * Process AST document and draw the text in the GTK TextView
 */
void Draw::processDocument(cmark_node *root_node)
{
    this->clear();

    // Loop over AST nodes
    cmark_event_type ev_type;
    cmark_iter *iter = cmark_iter_new(root_node);
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE)
    {
        cmark_node *cur = cmark_iter_get_node(iter);
        processNode(cur, ev_type);
    }
}

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
                // TODO: Indent for each list level
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
        break;

    case CMARK_NODE_HTML_BLOCK:
        break;

    case CMARK_NODE_CUSTOM_BLOCK:
        break;

    case CMARK_NODE_THEMATIC_BREAK:
    {
        /*
        TODO: Can we draw a line in textview?
        line.margin_end_x = 20;
        line.height = 0.2;
        line.hex_color = "2e2e2e";
        line.cap = Cairo::LineCap::LINE_CAP_ROUND;
        cr->set_line_cap(line.cap);
        cr->set_source_rgb(r, g, b);
        cr->set_line_width(line.height);
        cr->move_to(line.start_x, line.start_y);
        cr->line_to(endX, line.end_y);
        cr->stroke();
        */
    }
    break;

    case CMARK_NODE_PARAGRAPH:
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

        // For some reason Pango::Layout:create objects doesn't show up in cairo content
        std::string text = cmark_node_get_literal(node);
        if (bulletListLevel > 0)
        {
            text.insert(0, "\u2022 ");
        }
        else if (orderedListLevel > 0)
        {
            std::string number;
            if (orderedListLevel % 2 == 0)
            {
                number = intToRoman(orderedListCounters[orderedListLevel]) + " ";
            }
            else
            {
                number = std::to_string(orderedListCounters[orderedListLevel]) + ". ";
            }
            text.insert(0, number);
        }

        if (headingLevel > 0)
        {
            switch (headingLevel)
            {
            case 1:
                addHeading1(text);
                break;
            case 2:
                addHeading2(text);
                break;
            case 3:
                addHeading3(text);
                break;
            case 4:
                addHeading4(text);
                break;
            default:
                break;
            }
        }
        else if (isBold && isItalic)
        {
            addBoldItalic(text);
        }
        else if (isBold)
        {
            addBold(text);
        }
        else if (isItalic)
        {
            addItalic(text);
        }
        else
        {
            addText(text);
        }
    }
    break;

    case CMARK_NODE_LINEBREAK:
        // TODO: Hard line break (\n ?)
        break;

    case CMARK_NODE_SOFTBREAK:
        // ignore
        break;

    case CMARK_NODE_CODE:
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
        break;

    case CMARK_NODE_IMAGE:
        break;

    case CMARK_NODE_FOOTNOTE_REFERENCE:
        break;

    case CMARK_NODE_FOOTNOTE_DEFINITION:
        break;
    default:
        assert(false);
        break;
    }
}

void Draw::addText(const std::string &text)
{
    addMarkupText("<span font_desc=\"" + defaultFont.to_string() + "\">" + text + "</span>");
}

void Draw::addHeading1(const std::string &text)
{
    addMarkupText("<span font_desc=\"" + heading1.to_string() + "\">" + text + "</span>\n\n");
}

void Draw::addHeading2(const std::string &text)
{
    addMarkupText("<span font_desc=\"" + heading2.to_string() + "\">" + text + "</span>\n\n");
}

void Draw::addHeading3(const std::string &text)
{
    addMarkupText("<span font_desc=\"" + heading3.to_string() + "\">" + text + "</span>\n\n");
}

void Draw::addHeading4(const std::string &text)
{
    addMarkupText("<span font_desc=\"" + heading4.to_string() + "\">" + text + "</span>\n\n");
}

void Draw::addBold(const std::string &text)
{
    addMarkupText("<span font_desc=\"" + bold.to_string() + "\">" + text + "</span>\n\n");
}

void Draw::addItalic(const std::string &text)
{
    addMarkupText("<span font_desc=\"" + italic.to_string() + "\">" + text + "</span>\n\n");
}

void Draw::addBoldItalic(const std::string &text)
{
    addMarkupText("<span font_desc=\"" + boldItalic.to_string() + "\">" + text + "</span>\n\n");
}

void Draw::addMarkupText(const std::string &text)
{
    auto buffer = Glib::unwrap(this->get_buffer());
    DispatchData *data = g_new0(struct DispatchData, 1);
    data->buffer = buffer;
    data->text = text;
    gdk_threads_add_idle((GSourceFunc)addTextIdle, data);
}

void Draw::clear()
{
    auto buffer = Glib::unwrap(this->get_buffer());
    gdk_threads_add_idle((GSourceFunc)clearIdle, buffer);
}

gboolean Draw::addTextIdle(struct DispatchData *data)
{
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(data->buffer, &end_iter);
    gtk_text_buffer_insert_markup(data->buffer, &end_iter, data->text.c_str(), -1);
    g_free(data);
    return FALSE;
}

gboolean Draw::clearIdle(GtkTextBuffer *textBuffer)
{
    GtkTextIter start_iter, end_iter;
    gtk_text_buffer_get_start_iter(textBuffer, &start_iter);
    gtk_text_buffer_get_end_iter(textBuffer, &end_iter);
    gtk_text_buffer_delete(textBuffer, &start_iter, &end_iter);
    return FALSE;
}

/**
 * Convert number to roman number
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

/**
 * Convert hex string to seperate RGB values
 */
void Draw::hexToRGB(const std::string &hex, double &r, double &g, double &b)
{
    unsigned int intR, intG, intB;
    sscanf(hex.c_str(), "%02x%02x%02x", &intR, &intG, &intB);
    r = intR / 255.0;
    g = intG / 255.0;
    b = intB / 255.0;
}