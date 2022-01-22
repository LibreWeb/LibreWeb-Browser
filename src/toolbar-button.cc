#include "toolbar-button.h"

/**
 * \brief Toolbar Button constructor
 * \param tooltipText Use the following text as tooltip message
 * \param canFocus Can you focus on the button (default false)
 */
ToolbarButton::ToolbarButton(const std::string& tooltipText, bool canFocus)
{
  set_tooltip_text(tooltipText);
  set_can_focus(canFocus);
  set_relief(Gtk::RELIEF_NONE);
}
