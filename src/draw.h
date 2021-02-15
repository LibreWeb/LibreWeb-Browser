#ifndef DRAW_H
#define DRAW_H

#include <gtkmm/textview.h>
#include <pangomm/layout.h>
#include <cmark-gfm.h>

class MainWindow;
struct DispatchData;

class Draw : public Gtk::TextView
{
public:
    explicit Draw(MainWindow &mainWindow);
    void showMessage(const std::string &message, const std::string &detailed_info = "");
    void showStartPage();
    void processDocument(cmark_node *root_node);

protected:
    // Signals
    void event_after(GdkEvent *ev);

private:
    void followLink(Gtk::TextBuffer::iterator &iter);

    void processNode(cmark_node *node, cmark_event_type ev_type);
    // Helper functions for inserting text
    void insertText(const std::string &text);
    void insertHeading1(const std::string &text);
    void insertHeading2(const std::string &text);
    void insertHeading3(const std::string &text);
    void insertHeading4(const std::string &text);
    void insertHeading5(const std::string &text);
    void insertHeading6(const std::string &text);
    void insertItalic(const std::string &text);
    void insertBold(const std::string &text);
    void insertBoldItalic(const std::string &text);

    void insertMarkupText(const std::string &text);
    void insertLink(const std::string &text, const std::string &url);
    void clear();
    static gboolean insertTextIdle(struct DispatchData *data);
    static gboolean insertLinkIdle(struct DispatchData *data);
    static gboolean clearIdle(GtkTextBuffer *textBuffer);
    static std::string const intToRoman(int num);

    MainWindow &mainWindow;
    GtkTextBuffer *buffer;
    int fontSize;
    std::string fontFamily;
    int headingLevel;
    int listLevel;
    bool isBold;
    bool isItalic;
    int bulletListLevel;
    int orderedListLevel;
    bool isOrderedList;
    bool isLink;
    std::string linkURL;
    std::map<int,int> orderedListCounters;

    Pango::FontDescription defaultFont;
    Pango::FontDescription bold;
    Pango::FontDescription italic;
    Pango::FontDescription boldItalic;
    Pango::FontDescription heading1;
    Pango::FontDescription heading2;
    Pango::FontDescription heading3;
    Pango::FontDescription heading4;
    Pango::FontDescription heading5;
    Pango::FontDescription heading6;
};

#endif