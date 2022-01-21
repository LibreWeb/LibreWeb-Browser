#ifndef OPTION_GROUP_H
#define OPTION_GROUP_H

#include <glibmm/optioncontext.h>
#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>

/**
 * \class OptionGroup
 * \brief Command-line Parameters options class
 */
class OptionGroup : public Glib::OptionGroup
{
public:
  OptionGroup();

  // Implement virtuals methods
  bool on_pre_parse(Glib::OptionContext& context, Glib::OptionGroup& group) override;
  bool on_post_parse(Glib::OptionContext& context, Glib::OptionGroup& group) override;
  void on_error(Glib::OptionContext& context, Glib::OptionGroup& group) override;

  Glib::ustring timeout;
  bool disableIPFSDaemon;
  bool version;
};

#endif