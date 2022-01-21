#include "option-group.h"

OptionGroup::OptionGroup() : Glib::OptionGroup("main_group", "Options", "Options"), timeout("120s"), disableIPFSDaemon(false), version(false)
{
  Glib::OptionEntry entry1;
  entry1.set_long_name("timeout");
  entry1.set_short_name('t');
  entry1.set_description("Change time-out value of IPFS fetch; TIMEOUT should be a string, like 5m (default: 120s)");
  entry1.set_arg_description("TIMEOUT");
  add_entry(entry1, timeout);

  Glib::OptionEntry entry2;
  entry2.set_long_name("disable-ipfs-daemon");
  entry2.set_short_name('d');
  entry2.set_description("Do NOT start IPFS daemon during browser start-up (normally you would want to have IPFS "
                         "running, so this option is NOT advised)");
  add_entry(entry2, disableIPFSDaemon);

  Glib::OptionEntry entryVersion;
  entryVersion.set_long_name("version");
  entryVersion.set_short_name('v');
  entryVersion.set_description("Show version");
  add_entry(entryVersion, version);
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