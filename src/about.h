#ifndef ABOUT_H
#define ABOUT_H

#include <gtkmm/aboutdialog.h>

/**
 * \class About
 * \brief About dialog
 */
class About: public Gtk::AboutDialog
{
public:
  About();
  virtual ~About();

protected:
};
#endif