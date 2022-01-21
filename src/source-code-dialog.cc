#include "source-code-dialog.h"
#include <gtkmm/textbuffer.h>

SourceCodeDialog::SourceCodeDialog() : accelGroup(Gtk::AccelGroup::create())
{
  set_title("View source code");
  set_default_size(700, 750);
  add_accel_group(accelGroup); // TODO: Impl. a menu?

  m_closeButton.add_label("_Close", true);
  m_closeButton.set_margin_top(10);
  m_closeButton.signal_clicked().connect(sigc::mem_fun(this, &SourceCodeDialog::hide));
  m_sourceCode.set_editable(false);
  m_scrolledWindow.add(m_sourceCode);
  m_scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

  auto vbox = get_content_area();
  vbox->pack_start(m_scrolledWindow, true, true, 0);
  vbox->pack_start(m_closeButton, false, false, 0);

  show_all_children();
}
SourceCodeDialog::~SourceCodeDialog()
{
}

/**
 * \brief Set multi-line code source
 * \param[in] text Source code text
 */
void SourceCodeDialog::setText(const std::string& text)
{
  Glib::RefPtr<Gtk::TextBuffer> buffer = m_sourceCode.get_buffer();
  buffer->set_text(text);
}

/**
 * \brief Hide the code source dialog
 */
void SourceCodeDialog::hide_dialog(__attribute__((unused)) int response)
{
  hide();
}