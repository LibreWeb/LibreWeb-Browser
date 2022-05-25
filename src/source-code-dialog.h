#ifndef SOURCE_CODE_WINDOW_H
#define SOURCE_CODE_WINDOW_H

#include <gtkmm/dialog.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>
#include <string>

/**
 * \class SourceCodeDialog
 * \brief Source-code pop-up window
 */
class SourceCodeDialog : public Gtk::Dialog
{
public:
  SourceCodeDialog();
  virtual ~SourceCodeDialog();
  void set_text(const std::string& text);
  void hide_dialog(int response);

protected:
  // Child widgets
  Gtk::ScrolledWindow scrolled_window;
  Gtk::TextView source_code;
  Gtk::Button close_button;
  Glib::RefPtr<Gtk::AccelGroup> accel_group;

private:
};

#endif