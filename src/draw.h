#ifndef DRAW_H
#define DRAW_H

#include <gtkmm/textview.h>
#include <gtkmm/menu.h>
#include <gdkmm/cursor.h>
#include <pangomm/layout.h>
#include <cmark-gfm.h>

class MainWindow;

/**
 * \struct UndoRedoData
 * \brief Data structure for undo/redo text
 */
struct UndoRedoData
{
    bool isInsert;
    Glib::ustring text;
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
    void showMessage(const Glib::ustring &message, const Glib::ustring &detailed_info = "");
    void showStartPage();
    void processDocument(cmark_node *root_node);
    void setViewSourceMenuItem(bool isEnabled);
    void newDocument();
    Glib::ustring getText();
    void setText(const Glib::ustring &content);
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
    Glib::ustring linkURL;
    std::map<int, int> orderedListCounters;
    Glib::RefPtr<Gdk::Cursor> normalCursor;
    Glib::RefPtr<Gdk::Cursor> linkCursor;
    Glib::RefPtr<Gdk::Cursor> textCursor;
    bool hovingOverLink;
    bool isUserAction;

    std::vector<UndoRedoData> undoPool;
    std::vector<UndoRedoData> redoPool;
    sigc::connection beginUserActionSignalHandler;
    sigc::connection endUserActionSignalHandler;
    sigc::connection insertTextSignalHandler;
    sigc::connection deleteTextSignalHandler;

    void addTags();
    void enableEdit();
    void disableEdit();
    void followLink(Gtk::TextBuffer::iterator &iter);
    void processNode(cmark_node *node, cmark_event_type ev_type);
    // Helper functions for inserting text (thread-safe)
    void encodeText(std::string &string);
    void insertText(std::string text, const Glib::ustring &url = "", CodeTypeEnum codeType = CodeTypeEnum::NONE);
    void insertTagText(const Glib::ustring &text, std::vector<Glib::ustring> const &tagNames);
    void insertTagText(const Glib::ustring &text, const Glib::ustring &tagName);
    void insertMarkupText(const Glib::ustring &text);
    void insertLink(const Glib::ustring &text, const Glib::ustring &url);
    void truncateText(int charsTruncated);
    void clearOnThread();

    void changeCursor(int x, int y);
    void insertTagTextIdle(const Glib::ustring &text, std::vector<Glib::ustring> const &tagNames);
    void insertSingleTagTextIdle(const Glib::ustring &text, const Glib::ustring &tagName);    
    void insertMarupTextIdle(const Glib::ustring &text);
    void insertPlainTextIdle(const Glib::ustring &text);
    void insertLinkIdle(const Glib::ustring &text, const Glib::ustring &url);
    void truncateTextIdle(int charsTruncated);
    void clearBufferIdle();
    static Glib::ustring const intToRoman(int num);
};

#endif