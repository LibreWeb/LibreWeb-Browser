#ifndef MD_RENDER_H
#define MD_RENDER_H

#include <cmark-gfm.h>
#include <render.h>
#include <QtGlobal>
#include <QString>

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
    int headingLevel;
    int listLevel;
    qreal currentX;
    qreal currentY;
    QString fontFamilty;
    qreal wordSpacing;
    qreal heighestHigh;
    qreal paragraphHeightOffset;
    qreal headingHeightOffset;
    qreal listXOffset;

    qreal bulletWithTemp;

    void renderNode(cmark_node *node, cmark_event_type ev_type);
    QRectF const drawText(const QString& text);
    QRectF const drawBullet();
};
#endif
