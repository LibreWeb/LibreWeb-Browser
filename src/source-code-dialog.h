#ifndef SOURCE_CODE_WINDOW_H
#define SOURCE_CODE_WINDOW_H

#include <gtkmm/dialog.h>
#include <gtkmm/textview.h>
#include <gtkmm/scrolledwindow.h>
#include <string>

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
private:

};

#endif