#include "source-code-dialog.h"
#include <gtkmm/textbuffer.h>

SourceCodeDialog::SourceCodeDialog() : accel_group(Gtk::AccelGroup::create())
{
  set_title("View source code");
  set_default_size(700, 750);
  add_accel_group(accel_group); // TODO: Impl. a menu?

  close_button.add_label("_Close", true);
  close_button.set_margin_top(10);
  close_button.signal_clicked().connect(sigc::mem_fun(this, &SourceCodeDialog::hide));
  source_code.set_editable(false);
  scrolled_window.add(source_code);
  scrolled_window.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

  auto vbox = get_content_area();
  vbox->pack_start(scrolled_window, true, true, 0);
  vbox->pack_start(close_button, false, false, 0);

  show_all_children();
}
SourceCodeDialog::~SourceCodeDialog()
{
}

/**
 * \brief Set multi-line code source
 * \param[in] text Source code text
 */
void SourceCodeDialog::set_text(const std::string& text)
{
  Glib::RefPtr<Gtk::TextBuffer> buffer = source_code.get_buffer();
  buffer->set_text(text);
}

/**
 * \brief Hide the code source dialog
 */
void SourceCodeDialog::hide_dialog(__attribute__((unused)) int response)
{
  hide();
}