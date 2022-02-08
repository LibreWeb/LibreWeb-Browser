#ifndef TOOLBAR_BUITTON_H
#define TOOLBAR_BUITTON_H

#include <gtkmm/button.h>

/**
 * \class ToolbarButton
 * \brief Default Toolbar Button (inherit Gtk::Button)
 */
class ToolbarButton : public Gtk::Button
{
public:
  explicit ToolbarButton(const std::string& tooltipText, bool canFocus = false);
};
#endif