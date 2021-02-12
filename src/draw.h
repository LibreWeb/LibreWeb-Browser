#ifndef DRAW_H
#define DRAW_H

#include <gtkmm/textview.h>
#include <pangomm/layout.h>
#include <cmark-gfm.h>

struct DispatchData;

class Draw : public Gtk::TextView
{
public:
    Draw();
    void showMessage(const std::string &message, const std::string &detailed_info = "");
    void showStartPage();
    void processDocument(cmark_node *root_node);

private:
    void processNode(cmark_node *node, cmark_event_type ev_type);
    // Helper functions for adding text
    void addText(const std::string &text);
    void addHeading1(const std::string &text);
    void addHeading2(const std::string &text);
    void addHeading3(const std::string &text);
    void addHeading4(const std::string &text);
    void addItalic(const std::string &text);
    void addBold(const std::string &text);
    void addBoldItalic(const std::string &text);

    void addMarkupText(const std::string &text);
    void clear();
    static gboolean addTextIdle(struct DispatchData *data);
    static gboolean clearIdle(GtkTextBuffer *textBuffer);
    std::string const intToRoman(int num);
    void hexToRGB(const std::string& hex, double &r, double &g, double &b);

    int fontSize;
    std::string fontFamily;
    int headingLevel;
    int listLevel;
    bool isBold;
    bool isItalic;
    int bulletListLevel;
    int orderedListLevel;
    bool isOrderedList;
    std::map<int,int> orderedListCounters;

    Pango::FontDescription defaultFont;
    Pango::FontDescription bold;
    Pango::FontDescription italic;
    Pango::FontDescription boldItalic;
    Pango::FontDescription heading1;
    Pango::FontDescription heading2;
    Pango::FontDescription heading3;
    Pango::FontDescription heading4;
};

#endif