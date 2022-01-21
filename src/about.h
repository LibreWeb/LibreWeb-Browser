#ifndef ABOUT_H
#define ABOUT_H

#include <gtkmm/aboutdialog.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/linkbutton.h>
#include <gtkmm/window.h>

/**
 * \class About
 * \brief About dialog window
 */
class About : public Gtk::AboutDialog
{
public:
  explicit About(Gtk::Window& parent);
  void show_about();
  void hide_about(int response);

protected:
  Gtk::Image logo; /*!< The logo of the app */
  Gtk::LinkButton m_visitHomepage;
  Gtk::LinkButton m_visitProjectLinkButton;

private:
  std::string getLogoImage();
  void visit_gitlab_project();
};
#endif