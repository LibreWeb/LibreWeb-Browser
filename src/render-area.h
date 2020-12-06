#ifndef RENDER_AREA_H
#define RENDER_AREA_H

#include <gtkmm/drawingarea.h>
#include <pangomm/layout.h>
#include <cmark-gfm.h>

struct text_struct {
    int x;
    int y;
    Glib::RefPtr<Pango::Layout> layout;
};

struct line_struct {
    int start_x;
    int start_y;
    int end_x;  // -1 means auto-size
    int end_y; 
    int margin_end_x;
    double height;
    std::string hex_color;
    Cairo::LineCap cap;
};

class RenderArea : public Gtk::DrawingArea
{
public:
    RenderArea();
    virtual ~RenderArea();

    void processDocument(cmark_node *root_node);
    void showMessage(const std::string &message, const std::string &detailed_info = "");
    
protected:
    std::list<text_struct> m_textList;
    std::list<line_struct> m_lines;

    // Override default signal handler:
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

private:
    int currentX;
    int currentY;
    int sceneMarginX;
    int sceneMarginY;
    int currentXList;
    int headingLevel;
    int listLevel;
    int wordSpacing;
    int heighestHigh;
    int paragraphMargin;
    int headingMargin;
    int listMargin;
    int horizontalLineMargin;
    int listXOffset;
    bool isBold;
    bool isItalic;
    int bulletListLevel;
    int orderedListLevel;
    bool isOrderedList;
    std::map<int,int> orderedListCounters;
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
    void clear();
    void processNode(cmark_node *node, cmark_event_type ev_type);
    void redraw();
    std::string const intToRoman(int num);
    void hexToRGB(const std::string& hex, double &r, double &g, double &b);
};

#endif