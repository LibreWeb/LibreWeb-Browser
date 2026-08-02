#include "publish-dialog.h"

#include <gtkmm/messagedialog.h>

/**
 * \brief PublishDialog constructor
 * \param parent Parent window, so the dialog is modal to the browser
 */
PublishDialog::PublishDialog(Gtk::Window& parent)
    : Gtk::Dialog("Publish document", parent, true),
      vbox_choices(Gtk::ORIENTATION_VERTICAL),
      intro_label("Publish to:"),
      hint_label(""),
      hbox_new_name(Gtk::ORIENTATION_HORIZONTAL)
{
  set_default_size(420, -1);
  set_resizable(false);

  intro_label.set_xalign(0.0);
  intro_label.set_margin_bottom(4);

  Gtk::RadioButton::Group group;

  // The user's existing names come first: re-publishing an existing one is by
  // far the common case once someone has a site.
  if (FreedomNamesCli::available())
    keys_ = FreedomNamesCli::list_keys();
  for (const FreedomKey& key : keys_)
  {
    // A key whose full name could not be read still publishes fine -- the CLI
    // resolves the label itself -- so show the label rather than hiding it.
    const std::string caption = key.full_name.empty() ? key.label : key.full_name;
    key_buttons_.push_back(std::make_unique<Gtk::RadioButton>(group, caption));
    key_buttons_.back()->set_tooltip_text("Owner key \"" + key.label + "\" in ~/.freedom/keys");
    vbox_choices.pack_start(*key_buttons_.back(), false, false, 0);
  }

  if (FreedomNamesCli::available())
  {
    new_name_button_ = std::make_unique<Gtk::RadioButton>(group, "New name:");
    new_label_entry.set_placeholder_text("blog");
    new_label_entry.set_tooltip_text("A short label. Your full name becomes \"<label>.<your key ID>.fn\".");
    new_label_entry.set_max_length(63);
    hbox_new_name.set_spacing(6);
    hbox_new_name.pack_start(*new_name_button_, false, false, 0);
    hbox_new_name.pack_start(new_label_entry, true, true, 0);
    vbox_choices.pack_start(hbox_new_name, false, false, 0);
  }

  content_hash_button_ = std::make_unique<Gtk::RadioButton>(group, "Content hash only (no name)");
  content_hash_button_->set_tooltip_text("Store the bytes and get an fn://<hash> address. Nobody owns it and it cannot be updated.");
  vbox_choices.pack_start(*content_hash_button_, false, false, 0);

  // Default to the first existing name, else creating one, else the hash. That
  // ordering means someone who already has a site just presses Publish.
  if (!key_buttons_.empty())
    key_buttons_.front()->set_active(true);
  else if (new_name_button_)
    new_name_button_->set_active(true);
  else
    content_hash_button_->set_active(true);

  hint_label.set_xalign(0.0);
  hint_label.set_margin_top(8);
  hint_label.get_style_context()->add_class("dim-label");
  hint_label.set_line_wrap(true);

  for (auto& button : key_buttons_)
    button->signal_toggled().connect(sigc::mem_fun(this, &PublishDialog::on_selection_changed));
  if (new_name_button_)
    new_name_button_->signal_toggled().connect(sigc::mem_fun(this, &PublishDialog::on_selection_changed));
  content_hash_button_->signal_toggled().connect(sigc::mem_fun(this, &PublishDialog::on_selection_changed));
  // Typing a label is a clear enough signal that this is the option meant.
  new_label_entry.signal_changed().connect(
      [this]()
      {
        if (new_name_button_ && !new_label_entry.get_text().empty())
          new_name_button_->set_active(true);
      });

  vbox_choices.set_spacing(4);
  vbox_choices.set_margin_start(12);
  vbox_choices.set_margin_end(12);
  vbox_choices.set_margin_top(12);
  vbox_choices.set_margin_bottom(12);
  vbox_choices.pack_start(hint_label, false, false, 0);

  Gtk::Box* content_area = get_content_area();
  content_area->pack_start(intro_label, false, false, 0);
  content_area->pack_start(vbox_choices, true, true, 0);
  intro_label.set_margin_start(12);
  intro_label.set_margin_top(12);

  add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  add_button("_Publish", Gtk::RESPONSE_OK);
  set_default_response(Gtk::RESPONSE_OK);

  on_selection_changed();
  show_all_children();
}

PublishDialog::~PublishDialog()
{
}

/**
 * \brief Keep the hint honest about what the selected option will actually do.
 */
void PublishDialog::on_selection_changed()
{
  if (target() == Target::ContentHash)
  {
    hint_label.set_text(
        "The content gets an address derived from its bytes. It stays available, but nobody owns it and you cannot point it at a new version later.");
  }
  else
  {
    hint_label.set_text("Signs a record with your owner key and publishes it to the network. The record is valid for 7 days -- publish again before "
                        "then to renew it.");
  }
}

/**
 * \brief Run the dialog, validating the chosen option before accepting it.
 * \return true when the user confirmed a usable choice
 */
bool PublishDialog::run_dialog()
{
  while (true)
  {
    if (run() != Gtk::RESPONSE_OK)
      return false;

    // The CLI validates labels authoritatively (they end up as file names, so it
    // rejects anything path-like); this only catches the empty case, which would
    // otherwise produce a confusing error from a child process.
    if (target() == Target::Name && is_new_label() && label().empty())
    {
      Gtk::MessageDialog error(*this, "Please enter a name to publish to.", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
      error.set_secondary_text("A short label such as \"blog\" is enough; the rest of the name comes from your key.");
      error.run();
      continue;
    }
    return true;
  }
}

/**
 * \brief What the user chose to publish to.
 */
PublishDialog::Target PublishDialog::target() const
{
  return content_hash_button_->get_active() ? Target::ContentHash : Target::Name;
}

/**
 * \brief Owner label to publish to (only meaningful for Target::Name).
 */
std::string PublishDialog::label() const
{
  for (std::size_t i = 0; i < key_buttons_.size(); i++)
  {
    if (key_buttons_.at(i)->get_active())
      return keys_.at(i).label;
  }
  return new_label_entry.get_text();
}

/**
 * \brief Whether the chosen label still needs a key generated for it.
 */
bool PublishDialog::is_new_label() const
{
  return new_name_button_ && new_name_button_->get_active();
}
