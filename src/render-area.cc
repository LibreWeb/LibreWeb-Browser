#include "render-area.h"
#include "node.h"

#define PANGO_SCALE_XXX_LARGE ((double)1.98)

RenderArea::RenderArea()
:   currentX(0),
    currentY(0),
    sceneMarginX(3),
    sceneMarginY(3),
    headingLevel(0),
    listLevel(0),
    wordSpacing(4), // spacing may depend on the font
    heighestHigh(0),
    paragraphHeightOffset(5),
    headingHeightOffset(10),
    listXOffset(15),
    bulletPointDynamicOffset(0),
    isBold(false),
    isItalic(false),
    fontSize(10),
    fontFamily("Ubuntu")
{
    // Resize the drawing area to get scroll bars
    set_size_request(800, 1000);

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

void RenderArea::processDocument(cmark_node *root_node)
{
    textList.clear(); // reset

    cmark_event_type ev_type;
    cmark_iter *iter = cmark_iter_new(root_node);
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
        cmark_node *cur = cmark_iter_get_node(iter);
        processNode(cur, ev_type);
    }
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
            listLevel = 0;
            heighestHigh = 0;
            bulletPointDynamicOffset = 0;
        }
        break;

    case CMARK_NODE_BLOCK_QUOTE:
        break;

    case CMARK_NODE_LIST:
        if (entering) {
            listLevel++;
        } else {
            listLevel--;
            if (listLevel < 0)
                listLevel = 0;
        }

        if (listLevel == 0) {
            // Reset X to be safe
            currentX = sceneMarginX;
        } else if (listLevel > 0) {
            if (entering) {
                currentX += listXOffset;
            } else {
                currentX -= listXOffset;
            }
        }
        break;

    case CMARK_NODE_ITEM:
        // Line break for list items
        currentY += heighestHigh;
        // Reset heighest high (Y-axis)
        heighestHigh = 0;

        // Add bullet before text items
        if (entering) {
            //const QRectF rec = drawBullet();
            //bulletPointDynamicOffset = rec.width() + 2.0; // + offset
            currentX += bulletPointDynamicOffset;
        } else {
            currentX -= bulletPointDynamicOffset;
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
        currentY += heighestHigh + headingHeightOffset;
        
        // Reset heighest high (Y-axis)
        heighestHigh = 0;

        break;

    case CMARK_NODE_CODE_BLOCK:
        break;

    case CMARK_NODE_HTML_BLOCK:
        break;

    case CMARK_NODE_CUSTOM_BLOCK:
        break;

    case CMARK_NODE_THEMATIC_BREAK:
        break;

    case CMARK_NODE_PARAGRAPH:
        // Skip paragraph if listing is enabled
        if (listLevel == 0) {
            // Move to left again
            currentX = sceneMarginX;
            // New paragraph
            currentY += heighestHigh + paragraphHeightOffset;
            
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
        auto layout = create_pango_layout(cmark_node_get_literal(node));
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
        text textStruct;
        textStruct.x = currentX;
        textStruct.y = currentY;
        textStruct.layout = layout;
        textList.push_back(textStruct);

        if (textHeight > heighestHigh)
            heighestHigh = textHeight;
        // Skip paragraph if listing is enabled,
        // in all other cases increase x with text width
        if (listLevel == 0)
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

// Overrided method of GTK DrawingArea
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

    std::list<text>::iterator it;
    for(it = textList.begin(); it != textList.end(); ++it) {        
        auto text = (*it);
        cr->move_to(text.x, text.y);
        text.layout->show_in_cairo_context(cr);
    }
    return true;
}
