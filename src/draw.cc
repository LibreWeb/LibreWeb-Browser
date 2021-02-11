#include "draw.h"
#include <gdk/gdkthreads.h>
#include <mutex>
#include <memory>

// https://github.com/GNOME/gtkmm/blob/master/demos/gtk-demo/example_textview.cc

Draw::Draw()
{
  set_editable(false);
  set_indent(15);
  set_left_margin(10);
  set_right_margin(10);
  set_top_margin(5);
  set_bottom_margin(5);
  set_monospace(true);
  set_cursor_visible(false);
  set_app_paintable(true);
}

void Draw::showMessage(const std::string &message, const std::string &detailed_info)
{
  clear();
  addMarkupText("<span size=\"x-large\">" + message + "</span>\n\n");
  addMarkupText("<span>" + detailed_info + "</span>");
}

void Draw::addMarkupText(const std::string &text)
{
  auto buffer = Glib::unwrap(this->get_buffer());
  DispatchData *data = g_new0(struct DispatchData, 1);
  data->buffer = buffer;
  data->text = text;
  gdk_threads_add_idle((GSourceFunc)addText, data);
}

void Draw::clear()
{
  auto buffer = Glib::unwrap(this->get_buffer());
  gdk_threads_add_idle((GSourceFunc)clearBuffer, buffer);
}

gboolean Draw::addText(struct DispatchData *data)
{
  GtkTextIter end_iter; 
  gtk_text_buffer_get_end_iter(data->buffer, &end_iter);
  gtk_text_buffer_insert_markup(data->buffer, &end_iter, data->text.c_str(), -1);
  g_free(data);
  return FALSE;
}

gboolean Draw::clearBuffer(GtkTextBuffer *textBuffer)
{
  GtkTextIter start_iter, end_iter; 
  gtk_text_buffer_get_start_iter(textBuffer, &start_iter);
  gtk_text_buffer_get_end_iter(textBuffer, &end_iter);
  gtk_text_buffer_delete(textBuffer, &start_iter, &end_iter);
  return FALSE;
}