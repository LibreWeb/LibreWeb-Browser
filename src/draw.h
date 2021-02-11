#ifndef DRAW_H
#define DRAW_H

#include <gtkmm/textview.h>

struct DispatchData {
    GtkTextBuffer *buffer;
    std::string text;
};

class Draw : public Gtk::TextView
{
public:
  Draw();
  void showMessage(const std::string &message, const std::string &detailed_info = "");
  void addMarkupText(const std::string &text);
  void clear();

private:
  static gboolean addText(struct DispatchData *data);
  static gboolean clearBuffer(GtkTextBuffer *textBuffer);
};

#endif