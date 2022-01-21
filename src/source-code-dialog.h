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
  void setText(const std::string& text);
  void hide_dialog(int response);

protected:
  // Child widgets
  Gtk::ScrolledWindow m_scrolledWindow;
  Gtk::TextView m_sourceCode;
  Gtk::Button m_closeButton;
  Glib::RefPtr<Gtk::AccelGroup> accelGroup;

private:
};

#endif