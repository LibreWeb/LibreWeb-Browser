#ifndef MD_RENDER_H
#define MD_RENDER_H

#include <string>
#include <cmark-gfm.h>
#include <render.h>
#include <QtGlobal>

class Scene;
class QRectF;

/**
 * The Renderer will use Qt to render AST directly to a QGraphicsScene
 */
class Renderer
{
public:
    explicit Renderer(Scene *scene);
    void renderDocument(cmark_node *root, int width = 0);

private:
    Scene *scene;
    qreal sceneMarginX;
    qreal sceneMarginY;
    bool bold;
    bool italic;
    qreal currentX;
    qreal currentY;
    qreal heighestHigh;
    qreal paragraphOffsetHeight;

    void renderNode(cmark_node *node, cmark_event_type ev_type);
    QRectF const drawText(const std::string& text, bool bold = false, bool italic = false);
};
#endif
