#include "render-area.h"

#include "node.h"

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
    bulletPointDynamicOffset(0)
{
    // Resize the drawing area to get scroll bars
    set_size_request(800, 1000);
}

RenderArea::~RenderArea()
{
}

void RenderArea::renderDocument(cmark_node *root_node)
{
    cmark_event_type ev_type;
    cmark_iter *iter = cmark_iter_new(root_node);
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
        cmark_node *cur = cmark_iter_get_node(iter);
        renderNode(cur, ev_type);
    }
    // TODO: call the draw text/etc. methods below.
}

/**
 * Calculates the locations, render and paint the content/objects
 * to a QGraphicsScene
 */
void RenderArea::renderNode(cmark_node *node, cmark_event_type ev_type)
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
            printf("Text: %s\n", cmark_node_get_literal(node));
            /*const QRectF rec = drawText(cmark_node_get_literal(node));
            // Skip paragraph if listing is enabled
            if (listLevel == 0) {
                currentX += rec.width();
            }
            if (rec.height() > heighestHigh)
                heighestHigh = rec.height();
            */
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
        //bold = entering;
        break;

    case CMARK_NODE_EMPH:
        //italic = entering;
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

bool RenderArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    const int rectangle_width = width;
    const int rectangle_height = height / 2;

    // Draw a black rectangle
    cr->set_source_rgb(0.0, 0.0, 0.0);
    draw_rectangle(cr, rectangle_width, rectangle_height);

    // and some white text
    cr->set_source_rgb(1.0, 1.0, 1.0);
    draw_text(cr, rectangle_width, rectangle_height);

    // flip the image vertically
    // see http://www.cairographics.org/documentation/cairomm/reference/classCairo_1_1Matrix.html
    // the -1 corresponds to the yy part (the flipping part)
    // the height part is a translation (we could have just called cr->translate(0, height) instead)
    // it's height and not height / 2, since we want this to be on the second part of our drawing
    // (otherwise, it would draw over the previous part)
    Cairo::Matrix matrix(1.0, 0.0, 0.0, -1.0, 0.0, height);

    // apply the matrix
    cr->transform(matrix);

    // white rectangle
    cr->set_source_rgb(1.0, 1.0, 1.0);
    draw_rectangle(cr, rectangle_width, rectangle_height);

    // black text
    cr->set_source_rgb(0.0, 0.0, 0.0);
    draw_text(cr, rectangle_width, rectangle_height);


    draw_text(cr, 300, 110);
    draw_text(cr, 200, 150);
    draw_text(cr, 100, 240);
    draw_text(cr, 400, 280);

    return true;
}

void RenderArea::draw_rectangle(const Cairo::RefPtr<Cairo::Context>& cr,
                            int width, int height)
{
    cr->rectangle(0, 0, width, height);
    cr->fill();
}

void RenderArea::draw_text(const Cairo::RefPtr<Cairo::Context>& cr,
                       int rectangle_width, int rectangle_height)
{
    // http://developer.gnome.org/pangomm/unstable/classPango_1_1FontDescription.html
    Pango::FontDescription font;

    font.set_family("Ubuntu");
    font.set_weight(Pango::WEIGHT_BOLD);
    if (rectangle_width < 150)
        font.set_style(Pango::Style::STYLE_ITALIC);

    // http://developer.gnome.org/pangomm/unstable/classPango_1_1Layout.html
    auto layout = create_pango_layout("Hi there!");

    layout->set_font_description(font);

    int text_width;
    int text_height;

    //get the text dimensions (it updates the variables -- by reference)
    layout->get_pixel_size(text_width, text_height);

    // Position the text in the middle
    cr->move_to((rectangle_width-text_width)/2, (rectangle_height-text_height)/2);

    layout->show_in_cairo_context(cr);
}
