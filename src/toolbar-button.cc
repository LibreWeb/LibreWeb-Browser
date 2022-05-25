#include "toolbar-button.h"

/**
 * \brief Toolbar Button constructor
 * \param tooltip_text Use the following text as tooltip message
 * \param can_focus Can you focus on the button (default false)
 */
ToolbarButton::ToolbarButton(const std::string& tooltip_text, bool can_focus)
{
  set_tooltip_text(tooltip_text);
  set_can_focus(can_focus);
  set_relief(Gtk::RELIEF_NONE);
}
