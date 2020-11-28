#ifndef RENDER_AREA_H
#define RENDER_AREA_H

#include <gtkmm/drawingarea.h>
#include <cmark-gfm.h>

class RenderArea : public Gtk::DrawingArea
{
public:
    RenderArea();
    virtual ~RenderArea();

    void renderDocument(cmark_node *root_node);
    
protected:
    //Override default signal handler:
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

private:
    int currentX;
    int currentY;
    int sceneMarginX;
    int sceneMarginY;
    int headingLevel;
    int listLevel;
    int wordSpacing;
    int heighestHigh;
    int paragraphHeightOffset;
    int headingHeightOffset;
    int listXOffset;
    int bulletPointDynamicOffset;

    void renderNode(cmark_node *node, cmark_event_type ev_type);
    void draw_rectangle(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);
    void draw_text(const Cairo::RefPtr<Cairo::Context>& cr, int rectangle_width, int rectangle_height);
};

#endif