#ifndef RENDER_AREA_H
#define RENDER_AREA_H

#include <gtkmm/drawingarea.h>
#include <pangomm/layout.h>
#include <cmark-gfm.h>

struct text {
    int x;
    int y;
    Glib::RefPtr<Pango::Layout> layout;
};

class RenderArea : public Gtk::DrawingArea
{
public:
    RenderArea();
    virtual ~RenderArea();

    void processDocument(cmark_node *root_node);
    
protected:
    std::list<text> textList;

    // Override default signal handler:
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
    bool isBold;
    bool isItalic;
    int fontSize;
    std::string fontFamily;

    Pango::FontDescription defaultFont;
    Pango::FontDescription boldFont;
    Pango::FontDescription italicFont;
    Pango::FontDescription boldItalicFont;
    Pango::FontDescription heading1Font;
    Pango::FontDescription heading2Font;
    Pango::FontDescription heading3Font;
    Pango::FontDescription heading4Font;

    void createPangoContexts();
    void processNode(cmark_node *node, cmark_event_type ev_type);
};

#endif