#include "md-render.h"
#include "scene.h"
#include "cmark-gfm.h"

#include <stdlib.h>
#include <time.h>
#include <node.h>
#include <QGraphicsTextItem>
#include <QFont>

Renderer::Renderer(Scene* scene) : scene(scene) {
    srand (time(NULL));
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
        break;

    case CMARK_NODE_TEXT: {
            printf("Text");
            drawText(cmark_node_get_literal(node));
        }
        break;

    case CMARK_NODE_LINEBREAK:
        break;

    case CMARK_NODE_SOFTBREAK:
        break;

    case CMARK_NODE_CODE:
        break;

    case CMARK_NODE_HTML_INLINE:
        break;

    case CMARK_NODE_CUSTOM_INLINE:
        break;

    case CMARK_NODE_STRONG:
        printf("Bold\n");
        break;

    case CMARK_NODE_EMPH:
        printf("Italic\n");
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

void Renderer::drawText(const std::string& text)
{
    QGraphicsTextItem *textItem = new QGraphicsTextItem(QString::fromStdString(text));
    QFont font;
    font.setBold(true);
    font.setFamily("Open Sans"); // Arial
    textItem->setFont(font);
    const int x = rand() % 100;
    const int y = rand() % 100;
    textItem->setPos(x, y);
    textItem->setTextWidth(200);

    scene->addItem(textItem);
}