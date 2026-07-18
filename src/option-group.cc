#include "option-group.h"

OptionGroup::OptionGroup() : Glib::OptionGroup("main_group", "Options", "Options"), timeout("120s"), disable_freedom_node(false), version(false)
{
  Glib::OptionEntry entry_timeout;
  entry_timeout.set_long_name("timeout");
  entry_timeout.set_short_name('t');
  entry_timeout.set_description("Change time-out value of content fetch; TIMEOUT should be a string, like 5m (default: 120s)");
  entry_timeout.set_arg_description("TIMEOUT");
  add_entry(entry_timeout, timeout);

  Glib::OptionEntry entry_disable_freedom;
  entry_disable_freedom.set_long_name("disable-freedom-node");
  entry_disable_freedom.set_short_name('d');
  entry_disable_freedom.set_description("Do NOT start the Freedom Names node during browser start-up (normally you would want the "
                                        "node running, so this option is NOT advised)");
  add_entry(entry_disable_freedom, disable_freedom_node);

  Glib::OptionEntry entry_version;
  entry_version.set_long_name("version");
  entry_version.set_short_name('v');
  entry_version.set_description("Show version");
  add_entry(entry_version, version);
}

bool OptionGroup::on_pre_parse(Glib::OptionContext& context, Glib::OptionGroup& group)
{
  return Glib::OptionGroup::on_pre_parse(context, group);
}

bool OptionGroup::on_post_parse(Glib::OptionContext& context, Glib::OptionGroup& group)
{
  return Glib::OptionGroup::on_post_parse(context, group);
}

void OptionGroup::on_error(Glib::OptionContext& context, Glib::OptionGroup& group)
{
  Glib::OptionGroup::on_error(context, group);
}