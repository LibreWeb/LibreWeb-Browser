#ifndef QT_RENDER_H
#define QT_RENDER_H

#include <cmark-gfm.h>
#include <render.h>
#include <QtGlobal>
#include <QString>
#include "renderer-interface.h"

class Scene;
class QRectF;
class QFont;

/**
 * \class QtRenderer Class will use Qt to render AST object to a QGraphicsScene
 */
class QtRenderer : public RendererI
{
public:
    explicit QtRenderer();
    ~QtRenderer();

    void setScene(Scene *scene) override;
    void setUnknownYet() override {}; // No implementation
    void renderDocument(cmark_node *root, int width = 0) override;

private:
    Scene *scene;
    QFont *font;
    int defaultFontSize;
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

    // Copy contructor (not used/non-copyable)
    QtRenderer(const QtRenderer& _);
    // Copy assignment (not used/not assignable)
    QtRenderer& operator=(const QtRenderer& _);

    void renderNode(cmark_node *node, cmark_event_type ev_type);
    QRectF const drawText(const QString& text);
    QRectF const drawBullet();
};
#endif
