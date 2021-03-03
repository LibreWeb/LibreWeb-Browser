#ifndef DRAW_H
#define DRAW_H

#include <gtkmm/textview.h>
#include <gtkmm/menu.h>
#include <gdkmm/cursor.h>
#include <pangomm/layout.h>
#include <cmark-gfm.h>

class MainWindow;

/**
 * \struct DispatchData
 * \brief Data struct for dispatching calls to GTK thread (on idle)
 */
struct DispatchData;

/**
 * \class Draw
 * \brief Draw text area (GTK TextView), where the document content will be displayed or used a text editor
 */
class Draw : public Gtk::TextView
{
public:
    sigc::signal<void> source_code;

    explicit Draw(MainWindow &mainWindow);
    void showMessage(const std::string &message, const std::string &detailed_info = "");
    void showStartPage();
    void processDocument(cmark_node *root_node);
    void setViewSourceMenuItem(bool isEnabled);
    void newDocument();
    std::string getText();
    void clearText();
    void cut();
    void copy();
    void paste();
    void del();
    void selectAll();

    // Signals editor calls
    void make_heading(int headingLevel);
    void make_bold();
    void make_italic();
    void make_strikethrough();
    void make_super();
    void make_sub();
    void make_quote();
    void make_code();
    void insert_link();
    void insert_image();
    void insert_bullet_list();
    void insert_numbered_list();
    void make_highlight();

protected:
    // Signals
    void event_after(GdkEvent *ev);
    bool motion_notify_event(GdkEventMotion* motion_event);
    void populate_popup(Gtk::Menu *menu);

private:
    void enableEdit();
    void disableEdit();
    void followLink(Gtk::TextBuffer::iterator &iter);
    void processNode(cmark_node *node, cmark_event_type ev_type);
    // Helper functions for inserting text
    void insertText(const std::string &text);
    void insertCode(const std::string &code);
    void insertLink(const std::string &text, const std::string &url);
    void insertHeading1(const std::string &text);
    void insertHeading2(const std::string &text);
    void insertHeading3(const std::string &text);
    void insertHeading4(const std::string &text);
    void insertHeading5(const std::string &text);
    void insertHeading6(const std::string &text);
    void insertBoldItalic(const std::string &text);
    void insertItalic(const std::string &text);
    void insertBold(const std::string &text);
    void insertStrikethrough(const std::string &text);
    void insertHighlight(const std::string &text);

    void insertMarkupTextOnThread(const std::string &text);
    void clearOnThread();
    void changeCursor(int x, int y);
    static gboolean insertTextIdle(struct DispatchData *data);
    static gboolean insertLinkIdle(struct DispatchData *data);
    static gboolean clearBufferIdle(GtkTextBuffer *textBuffer);
    static std::string const intToRoman(int num);

    MainWindow &mainWindow;
    GtkTextBuffer *buffer;
    bool addViewSourceMenuItem;
    int fontSize;
    std::string fontFamily;
    int headingLevel;
    int listLevel;
    bool isBold;
    bool isItalic;
    bool isStrikethrough;
    bool isHighlight;
    int bulletListLevel;
    int orderedListLevel;
    bool isOrderedList;
    bool isLink;
    std::string linkURL;
    std::map<int,int> orderedListCounters;
    Glib::RefPtr<Gdk::Cursor> normalCursor;
    Glib::RefPtr<Gdk::Cursor> linkCursor;
    Glib::RefPtr<Gdk::Cursor> textCursor;
    bool hovingOverLink;

    Pango::FontDescription defaultFont;
    Pango::FontDescription boldItalic;
    Pango::FontDescription bold;
    Pango::FontDescription italic;
    Pango::FontDescription heading1;
    Pango::FontDescription heading2;
    Pango::FontDescription heading3;
    Pango::FontDescription heading4;
    Pango::FontDescription heading5;
    Pango::FontDescription heading6;
};

#endif