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
 * \struct UndoRedoData
 * \brief Data structure for undo/redo text
 */
struct UndoRedoData
{
    bool isInsert;
    std::string text;
    int beginOffset;
    int endOffset;
};

/**
 * \class Draw
 * \brief Draw text area (GTK TextView), where the document content will be displayed or used a text editor
 */
class Draw : public Gtk::TextView
{
public:
    sigc::signal<void> source_code;
    enum CodeTypeEnum
    {
        NONE = 0,
        INLINE_CODE,
        CODE_BLOCK
    };

    explicit Draw(MainWindow &mainWindow);
    void showMessage(const std::string &message, const std::string &detailed_info = "");
    void showStartPage();
    void processDocument(cmark_node *root_node);
    void setViewSourceMenuItem(bool isEnabled);
    void newDocument();
    std::string getText();
    void setText(const std::string &content);
    void clearText();
    void undo();
    void redo();
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
    void insert_link();
    void insert_image();
    void make_code();
    void insert_bullet_list();
    void insert_numbered_list();
    void make_highlight();
    void begin_user_action();
    void end_user_action();
    void on_insert(const Gtk::TextBuffer::iterator &pos, const Glib::ustring &text, int bytes);
    void on_delete(const Gtk::TextBuffer::iterator &range_start, const Gtk::TextBuffer::iterator &range_end);

protected:
    // Signals
    void event_after(GdkEvent *ev);
    bool motion_notify_event(GdkEventMotion *motion_event);
    void populate_popup(Gtk::Menu *menu);

private:
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
    bool isSuperscript;
    bool isSubscript;
    bool isQuote;
    int bulletListLevel;
    int orderedListLevel;
    bool isOrderedList;
    bool isLink;
    std::string linkURL;
    std::map<int, int> orderedListCounters;
    Glib::RefPtr<Gdk::Cursor> normalCursor;
    Glib::RefPtr<Gdk::Cursor> linkCursor;
    Glib::RefPtr<Gdk::Cursor> textCursor;
    bool hovingOverLink;
    Pango::FontDescription defaultFont;
    bool isUserAction;

    std::vector<UndoRedoData> undoPool;
    std::vector<UndoRedoData> redoPool;
    sigc::connection beginUserActionSignalHandler;
    sigc::connection endUserActionSignalHandler;
    sigc::connection insertTextSignalHandler;
    sigc::connection deleteTextSignalHandler;

    void enableEdit();
    void disableEdit();
    void followLink(Gtk::TextBuffer::iterator &iter);
    void processNode(cmark_node *node, cmark_event_type ev_type);
    // Helper functions for inserting text (thread-safe)
    void insertText(std::string text, const std::string &url = "", CodeTypeEnum codeType = CodeTypeEnum::NONE);
    void insertLink(const std::string &text, const std::string &url, const std::string &urlFont = "");
    void truncateText(int charsTruncated);
    void encodeText(std::string &string);

    void insertMarkupTextOnThread(const std::string &text);
    void clearOnThread();
    void changeCursor(int x, int y);
    static gboolean insertTextIdle(struct DispatchData *data);
    static gboolean insertPlainTextIdle(struct DispatchData *data);
    static gboolean insertLinkIdle(struct DispatchData *data);
    static gboolean truncateTextIdle(struct DispatchData *data);
    static gboolean clearBufferIdle(GtkTextBuffer *textBuffer);
    static std::string const intToRoman(int num);
};

#endif