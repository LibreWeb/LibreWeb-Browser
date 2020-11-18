#include "md-render.h"
#include "scene.h"
#include "cmark-gfm.h"

#include <node.h>
#include <QGraphicsTextItem>
#include <QFont>

Renderer::Renderer(Scene* scene) : 
    scene(scene),
    bold(false),
    italic(false),
    currentX(0.0),
    currentY(0.0),
    heighestHigh(0.0),
    paragraphOffsetY(2.0) {
}

/**
 * Render the whole document to scene/screen
 */
void Renderer::renderDocument(cmark_node *root, int width)
{
    cmark_event_type ev_type;
    cmark_iter *iter = cmark_iter_new(root);
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
        cmark_node *cur = cmark_iter_get_node(iter);
        renderNode(cur, ev_type);
    }
}

/**
 * Calculates the locations, render and paint the content/objects
 * to a QGraphicsScene
 */
void Renderer::renderNode(cmark_node *node, cmark_event_type ev_type)
{
    bool entering = (ev_type == CMARK_EVENT_ENTER);
    
    switch (node->type) {
    case CMARK_NODE_DOCUMENT:
        printf("Document\n");
        break;

    case CMARK_NODE_BLOCK_QUOTE:
        break;

    case CMARK_NODE_LIST:
        printf("List\n");
        break;

    case CMARK_NODE_ITEM:
        printf("Item\n");
        break;

    case CMARK_NODE_HEADING:
        printf("Heading\n");
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
        printf("Paragraph\n");
        currentX = 0;
        currentY += heighestHigh + paragraphOffsetY;
        
        // Reset heighest high (Y-axis)
        heighestHigh = 0;
        break;

    case CMARK_NODE_TEXT: {
            printf("Text\n");
            const QRectF rec = drawText(cmark_node_get_literal(node), bold, italic);
            currentX += rec.size().width();
            if (rec.size().height() > heighestHigh)
                heighestHigh = rec.size().height();
            printf("Width: %f\n", rec.size().width());
            printf("Height: %f\n", rec.size().height());
        }
        break;

    case CMARK_NODE_LINEBREAK:
        printf("Line break\n");
        break;

    case CMARK_NODE_SOFTBREAK:
        printf("Soft-Line break\n");
        break;

    case CMARK_NODE_CODE:
        break;

    case CMARK_NODE_HTML_INLINE:
        break;

    case CMARK_NODE_CUSTOM_INLINE:
        break;

    case CMARK_NODE_STRONG:
        printf("Bold\n");
        bold = entering;
        break;

    case CMARK_NODE_EMPH:
        printf("Italic\n");
        italic = entering;
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

QRectF const Renderer::drawText(const std::string& text, bool bold, bool italic)
{
    QGraphicsTextItem *textItem = new QGraphicsTextItem(QString::fromStdString(text));
    QFont font;
    if (bold)
        font.setBold(true);
    if (italic)
        font.setItalic(true);
    font.setFamily("Open Sans"); // Arial
    textItem->setFont(font);
    //textItem->setTextWidth(200);

    textItem->setPos(currentX, currentY);

    scene->addItem(textItem);

    return textItem->boundingRect();
}