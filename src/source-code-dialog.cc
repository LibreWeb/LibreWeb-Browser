#include "source-code-dialog.h"
#include <gtkmm/textbuffer.h>

SourceCodeDialog::SourceCodeDialog()
{
  set_title("View source code");
  set_default_size(700, 750);

  m_scrolledWindow.add(m_sourceCode);
  m_scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

  auto vbox = get_content_area();
  vbox->pack_start(m_scrolledWindow, true, true, 0);

  show_all_children();
}
SourceCodeDialog::~SourceCodeDialog() {}

/**
 * Set multi-line code source
 * \param[in] text Source code text
 */
void SourceCodeDialog::setText(const std::string& text)
{
  Glib::RefPtr<Gtk::TextBuffer> buffer = m_sourceCode.get_buffer();
  buffer->set_text(text);
}

void SourceCodeDialog::hide_dialog(__attribute__((unused)) int response)
{
    hide();
}