#ifndef DRAW_H
#define DRAW_H

#include <cmark-gfm.h>
#include <gdkmm/cursor.h>
#include <gtkmm/menu.h>
#include <gtkmm/textview.h>
#include <gtkmm/tooltip.h>
#include <pangomm/layout.h>

class MiddlewareInterface;

/**
 * \struct UndoRedoData
 * \brief Data structure for undo/redo text
 */
struct UndoRedoData
{
  bool is_insert;
  Glib::ustring text;
  int begin_offset;
  int end_offset;
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
    CODE_TYPE_NONE = 0,
    CODE_TYPE_INLINE_CODE,
    CODE_TYPE_CODE_BLOCK
  };

  explicit Draw(MiddlewareInterface& middleware);
  void set_message(const Glib::ustring& message, const Glib::ustring& details = "");
  void show_homepage();
  void set_document(cmark_node* root_node);
  void set_view_source_menu_item(bool is_enabled);
  void new_document();
  Glib::ustring get_text() const;
  void set_text(const Glib::ustring& text);
  void clear();
  void undo();
  void redo();
  void cut();
  void copy();
  void paste();
  void del();
  void select_all();
  std::vector<Glib::RefPtr<Gtk::TextMark>> get_headings();

  // Signals editor calls
  void make_heading(int heading_level);
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
  void on_insert(const Gtk::TextBuffer::iterator& pos, const Glib::ustring& text, int bytes);
  void on_delete(const Gtk::TextBuffer::iterator& range_start, const Gtk::TextBuffer::iterator& range_end);

protected:
  // Signals
  void event_after(GdkEvent* ev);
  bool motion_notify_event(GdkEventMotion* motion_event);
  bool query_tooltip(int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip);
  void populate_popup(Gtk::Menu* menu);

private:
  MiddlewareInterface& middleware_;
  bool add_view_source_menu_item_;
  int heading_level_;
  int list_level_;
  bool is_bold_;
  bool is_italic_;
  bool is_strikethrough_;
  bool is_highlight_;
  bool is_superscript_;
  bool is_subscript_;
  bool is_quote_;
  int bullet_list_level_;
  int ordered_list_level_;
  bool is_ordered_list_;
  bool is_link;
  Glib::ustring link_url_;
  std::map<int, int> ordered_list_counters;
  Glib::RefPtr<Gdk::Cursor> normal_cursor_;
  Glib::RefPtr<Gdk::Cursor> link_cursor_;
  Glib::RefPtr<Gdk::Cursor> text_cursor_;
  bool hoving_over_link_;
  bool is_user_action_;
  std::vector<Glib::RefPtr<Gtk::TextMark>> headings_toc_;

  std::vector<UndoRedoData> undo_pool_;
  std::vector<UndoRedoData> redo_pool_;
  sigc::connection begin_user_action_signal_handler;
  sigc::connection end_user_action_signal_handler;
  sigc::connection insert_text_signal_handler;
  sigc::connection delete_text_signal_handler;

  void add_tags();
  void enable_edit();
  void disable_edit();
  void follow_link(Gtk::TextBuffer::iterator& iter);
  void process_node(cmark_node* node, cmark_event_type ev_type);
  void encode_text(std::string& string) const;
  void insert_text(std::string text, const Glib::ustring& url = "", CodeTypeEnum codeType = CodeTypeEnum::CODE_TYPE_NONE);
  void insert_tag_text(const Glib::ustring& text, std::vector<Glib::ustring> const& tag_names);
  void add_heading_mark(const Glib::ustring& text, int heading_level);
  void insert_tag_text(const Glib::ustring& text, const Glib::ustring& tag_name);
  void insert_markup_text(const Glib::ustring& text);
  void insert_link_text(const Glib::ustring& text, const Glib::ustring& url);
  void truncate_text(int chars_truncated);
  void change_cursor(int x, int y);
  static Glib::ustring int_to_roman(int num);
};

#endif