#include "render-area.h"
#include "node.h"

#include <gtkmm/widget.h>
#include <stdio.h>
#include <algorithm>
#include <chrono>
#include <iostream>

#define PANGO_SCALE_XXX_LARGE ((double)1.98)

RenderArea::RenderArea()
:   currentX(0),
    currentY(0),
    sceneMarginX(25),
    sceneMarginY(15),
    currentXList(sceneMarginX),
    headingLevel(0),
    listLevel(0),
    wordSpacing(4), // spacing may depend on the font
    heighestHigh(0),
    paragraphMargin(5),
    headingMargin(10),
    listMargin(5),
    horizontalLineMargin(2),
    listXOffset(20),
    isBold(false),
    isItalic(false),
    bulletListLevel(0),
    orderedListLevel(0),
    isOrderedList(false),
    fontSize(10),
    fontFamily("Ubuntu")
{
    // Resize the drawing area to get scroll bars
    set_size_request(300, 1000);
    createPangoContexts();
}

RenderArea::~RenderArea()
{
}

void RenderArea::createPangoContexts()
{
    defaultFont.set_family(fontFamily);
    defaultFont.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_MEDIUM);
    
    boldFont.set_family(fontFamily);
    boldFont.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_MEDIUM);
    boldFont.set_weight(Pango::WEIGHT_BOLD);

    italicFont.set_family(fontFamily);
    italicFont.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_MEDIUM);
    italicFont.set_style(Pango::Style::STYLE_ITALIC);

    boldItalicFont.set_family(fontFamily);
    boldItalicFont.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_MEDIUM);
    boldItalicFont.set_weight(Pango::WEIGHT_BOLD);
    boldItalicFont.set_style(Pango::Style::STYLE_ITALIC);

    heading1Font.set_family(fontFamily);
    heading1Font.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_XXX_LARGE);
    heading1Font.set_weight(Pango::WEIGHT_BOLD);
    heading2Font.set_family(fontFamily);
    heading2Font.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_XX_LARGE);
    heading2Font.set_weight(Pango::WEIGHT_BOLD);
    heading3Font.set_family(fontFamily);
    heading3Font.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_X_LARGE);
    heading3Font.set_weight(Pango::WEIGHT_BOLD);
    heading4Font.set_family(fontFamily);
    heading4Font.set_size(fontSize * PANGO_SCALE * PANGO_SCALE_LARGE);
    heading4Font.set_weight(Pango::WEIGHT_BOLD);
}

/**
 * Clean-up render area fields
 */
void RenderArea::clear()
{
    m_textList.clear();
    m_lines.clear();
}

/**
 * Process AST document and eventually render to screen (drawing area)
 */
void RenderArea::processDocument(cmark_node *root_node)
{
    this->clear();

    /*typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::milliseconds ms;
    typedef std::chrono::duration<float> fsec;
    auto t0 = Time::now();*/

    // Loop over AST nodes
    cmark_event_type ev_type;
    cmark_iter *iter = cmark_iter_new(root_node);
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
        cmark_node *cur = cmark_iter_get_node(iter);
        processNode(cur, ev_type);
    }

    /*auto t1 = Time::now();
    fsec fs = t1 - t0;
    ms d = std::chrono::duration_cast<ms>(fs);
    std::cout << fs.count() << "s\n";
    std::cout << d.count() << "ms\n";*/

    this->redraw();
}

/**
 * Show a message on screen
 * \param message Message to be displayed
 */
void RenderArea::showMessage(const std::string &message, const std::string &detailed_info)
{
    this->clear();

    auto layout = create_pango_layout(message);
    layout->set_font_description(heading1Font);
    text_struct textStruct;
    textStruct.x = 40;
    textStruct.y = 20;
    textStruct.layout = layout;
    m_textList.push_back(textStruct);

    if (!detailed_info.empty()) {
        auto detail_layout = create_pango_layout(detailed_info);
        detail_layout->set_font_description(defaultFont);
        text_struct textStructDetail;
        textStructDetail.x = 40;
        textStructDetail.y = 60;
        textStructDetail.layout = detail_layout;
        m_textList.push_back(textStructDetail);
    }
    
    this->redraw();
}

/**
 * Calculates the drawing locations and parse each node in the AST
 */
void RenderArea::processNode(cmark_node *node, cmark_event_type ev_type)
{
    bool entering = (ev_type == CMARK_EVENT_ENTER);
    
    switch (node->type) {
    case CMARK_NODE_DOCUMENT:
        if (entering) {
            // Reset
            currentX = sceneMarginX;
            currentY = sceneMarginY;
            headingLevel = 0;
            bulletListLevel = 0;
            listLevel = 0;
            heighestHigh = 0;
        }
        break;

    case CMARK_NODE_BLOCK_QUOTE:
        break;

    case CMARK_NODE_LIST: {
        cmark_list_type listType = node->as.list.list_type;

        if (entering) {
            if (listLevel == 0) {
                currentY += listMargin; // First level Y margin
            }
            listLevel++;
        } else {
            if (listLevel == 1) {
                currentY += listMargin; // First level Y margin
            }
            listLevel--;
        }
        if (listLevel == 0) {
            // Reset X to be safe
            currentX = sceneMarginX;
            currentXList = currentX;
            // Reset bullet/ordered levels
            bulletListLevel = 0;
            orderedListLevel = 0;
            isOrderedList = false;
        } else if (listLevel > 0) {
            if (entering) {
                currentXList += listXOffset;
                if (listType == cmark_list_type::CMARK_BULLET_LIST) {
                    bulletListLevel++;
                } else if(listType == cmark_list_type::CMARK_ORDERED_LIST) {
                    orderedListLevel++;
                    // Create the counter (and reset to zero)
                    orderedListCounters[orderedListLevel] = 0;
                }
            } else {
                currentXList -= listXOffset;
                if (listType == cmark_list_type::CMARK_BULLET_LIST) {
                    bulletListLevel--;
                } else if(listType == cmark_list_type::CMARK_ORDERED_LIST) {
                    orderedListLevel--;
                }
            }

            isOrderedList = (orderedListLevel > 0) && (bulletListLevel <= 0);
        }
        }
        break;

    case CMARK_NODE_ITEM:
        // Line break for list items
        currentY += heighestHigh;
        // Reset heighest high (Y-axis)
        heighestHigh = 0;
        // Set new node item to the correct X position
        currentX = currentXList;

        if (entering && isOrderedList) {
            // Increasement ordered list counter
            orderedListCounters[orderedListLevel]++;
        }
        break;

    case CMARK_NODE_HEADING:
        if (entering) {
            headingLevel = node->as.heading.level;
        } else {
            headingLevel = 0; // reset
        }
        // Move to left again
        currentX = sceneMarginX;
        // New heading
        currentY += heighestHigh + headingMargin;
        
        // Reset heighest high (Y-axis)
        heighestHigh = 0;

        break;

    case CMARK_NODE_CODE_BLOCK:
        break;

    case CMARK_NODE_HTML_BLOCK:
        break;

    case CMARK_NODE_CUSTOM_BLOCK:
        break;

    case CMARK_NODE_THEMATIC_BREAK: {
        currentY += horizontalLineMargin;
        line_struct line;
        line.start_x = 20;
        line.start_y = currentY;
        line.end_x = -1; // auto-size width
        line.end_y = currentY;
        line.margin_end_x = 20;
        line.height = 0.2;
        line.hex_color = "2e2e2e";
        line.cap = Cairo::LineCap::LINE_CAP_ROUND;
        m_lines.push_back(line);
        currentY += horizontalLineMargin;
        }
        break;

    case CMARK_NODE_PARAGRAPH:
        // Skip paragraph if listing is enabled
        if (listLevel == 0) {
            // Move to left again
            currentX = sceneMarginX;
            // New paragraph
            currentY += heighestHigh + paragraphMargin;
            
            // Reset heighest high (Y-axis)
            heighestHigh = 0;
        }
        break;

    case CMARK_NODE_TEXT: {
        // Instead of creating seperate pango layouts we may want to use Pango attributes,
        // for changing parts of the text. Which is most likely be faster.
        // https://developer.gnome.org/pango/stable/pango-Text-Attributes.html
        // Pango is using a simple parser for parsing (X)HTML:
        // https://developer.gnome.org/glib/stable/glib-Simple-XML-Subset-Parser.html
        // We can use simular parse functions and using their own 'OpenTag' struct containing a list of Pango attributes:
        // https://gitlab.gnome.org/GNOME/pango/-/blob/master/pango/pango-markup.c#L515

        // For some reason Pango::Layout:create objects doesn't show up in cairo content
        std::string text = cmark_node_get_literal(node);
        if (bulletListLevel > 0) {
            text.insert(0, "\u2022 ");
        } else if(orderedListLevel > 0) {
            std::string number;
            if (orderedListLevel % 2 == 0) {
                number = intToRoman(orderedListCounters[orderedListLevel]) + " ";
            } else {
                number = std::to_string(orderedListCounters[orderedListLevel]) + ". ";
            }
            text.insert(0, number);
        }

        auto layout = create_pango_layout(text);

        if (headingLevel > 0) {
            switch (headingLevel)
            {
            case 1:
                layout->set_font_description(heading1Font);
                break;
            case 2:
                layout->set_font_description(heading2Font);
                break;
            case 3:
                layout->set_font_description(heading3Font);
                break;
            case 4:
                layout->set_font_description(heading4Font);
                break;
            default:
                break;
            }
        } else if (isBold && isItalic) {
            layout->set_font_description(boldItalicFont);
        } else if (isBold) {
            layout->set_font_description(boldFont);
        } else if (isItalic) {
            layout->set_font_description(italicFont);
        } else {
            layout->set_font_description(defaultFont);
        }
        //layout->set_width(100);
        int textWidth;
        int textHeight;
        layout->get_pixel_size(textWidth, textHeight);

        // Add text to list
        text_struct textStruct;
        textStruct.x = currentX;
        textStruct.y = currentY;
        textStruct.layout = layout;
        m_textList.push_back(textStruct);

        if (textHeight > heighestHigh) {
            heighestHigh = textHeight;
        }
        // Increase X with text width
        currentX += textWidth;
        }
        break;

    case CMARK_NODE_LINEBREAK:
        // Move to left again
        currentX = sceneMarginX;
        // Line break (no soft break)
        currentY += heighestHigh;
        
        // Reset heighest high (Y-axis)
        heighestHigh = 0;
        break;

    case CMARK_NODE_SOFTBREAK:
        // ignore
        // Only insert a space between the words
        currentX += wordSpacing;
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

/**
 * Force redraw
 */
void RenderArea::redraw()
{
    queue_draw_area(0, 0, get_allocation().get_width(), get_allocation().get_height());
}

/**
 * Overrided method of GTK DrawingArea
 */
bool RenderArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    Gtk::Allocation allocation = get_allocation();
    // Total drawing area size
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    // White background
    cr->set_source_rgb(1.0, 1.0, 1.0);
    cr->rectangle(0, 0, width, height);
    cr->fill();

    // Set to black for text
    cr->set_source_rgb(0.0, 0.0, 0.0);

    // Draw text
    std::list<text_struct>::iterator textIt;
    for(textIt = m_textList.begin(); textIt != m_textList.end(); ++textIt) {        
        auto text = (*textIt);
        cr->move_to(text.x, text.y);
        text.layout->show_in_cairo_context(cr);
    }

    // Draw lines
    std::list<line_struct>::iterator lineIt;
    for(lineIt = m_lines.begin(); lineIt != m_lines.end(); ++lineIt) {        
        auto line = (*lineIt);
        double r, g, b;
        hexToRGB(line.hex_color, r, g, b);
        int endX = line.end_x;
        if (line.end_x == -1)
            endX = width - line.margin_end_x;
        cr->set_line_cap(line.cap);
        cr->set_source_rgb(r, g, b);
        cr->set_line_width(line.height);
        cr->move_to(line.start_x, line.start_y);
        cr->line_to(endX, line.end_y);
        cr->stroke();
    }
    return true;
}

/**
 * Convert number to roman number
 */
std::string const RenderArea::intToRoman(int num)
{
    static const int values[] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1 };
    static const std::string numerals[] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I" };
    std::string res;
    for (int i = 0; i < 13; ++i) {
        while (num >= values[i]) {
            num -= values[i];
            res += numerals[i];
        }
    }
    return res;
}

/**
 * Convert hex string to seperate RGB values
 */
void RenderArea::hexToRGB(const std::string& hex, double &r, double &g, double &b)
{
    unsigned int intR, intG, intB;
    sscanf(hex.c_str(), "%02x%02x%02x", &intR, &intG, &intB);
    r = intR / 255.0;
    g = intG / 255.0;
    b = intB / 255.0;
}