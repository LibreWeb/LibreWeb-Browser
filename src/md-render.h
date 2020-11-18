#ifndef MD_RENDER_H
#define MD_RENDER_H

#include <string>
#include <cmark-gfm.h>
#include <render.h>

class Scene;

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

    void renderNode(cmark_node *node, cmark_event_type ev_type);
    void drawText(const std::string& text);
};
#endif
