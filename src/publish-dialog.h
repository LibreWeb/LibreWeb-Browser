#ifndef PUBLISH_DIALOG_H
#define PUBLISH_DIALOG_H

#include "freedomnames.h"

#include <gtkmm/box.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/radiobutton.h>
#include <memory>
#include <string>
#include <vector>

/**
 * \class PublishDialog
 * \brief Asks what the current document should be published as.
 *
 * Publishing bytes and owning a name are two different things. The node will
 * store content for anyone (that only yields a hash), but pointing a *name* at
 * that content means signing a record with an owner key. This dialog is where
 * the user picks between the two, and where a first key gets created.
 */
class PublishDialog : public Gtk::Dialog
{
public:
  /**
   * \enum Target
   * \brief What the user chose to publish to.
   */
  enum class Target
  {
    Name,       /*!< publish to an owner name, creating the key first when new */
    ContentHash /*!< store the bytes only, yielding an fn://<hash> address */
  };

  PublishDialog(Gtk::Window& parent, const std::vector<FreedomName>& names, bool authoring_available);
  virtual ~PublishDialog();

  // Runs the dialog modally. Returns true when the user confirmed.
  bool run_dialog();

  Target target() const;
  // Owner label to publish to; only meaningful for Target::Name.
  std::string label() const;
  // True when that label still has to be created through the authoring API.
  bool is_new_label() const;

protected:
  Gtk::Box vbox_choices;
  Gtk::Label intro_label;
  Gtk::Label hint_label;
  Gtk::Box hbox_new_name;
  Gtk::Entry new_label_entry;

private:
  std::vector<FreedomName> names_;
  // One radio per existing name, in the same order as names_. Held by pointer
  // because the set is only known at construction time.
  std::vector<std::unique_ptr<Gtk::RadioButton>> key_buttons_;
  std::unique_ptr<Gtk::RadioButton> new_name_button_;
  std::unique_ptr<Gtk::RadioButton> content_hash_button_;

  void on_selection_changed();
};
#endif
