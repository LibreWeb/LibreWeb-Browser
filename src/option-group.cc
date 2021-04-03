#include "option-group.h"

OptionGroup::OptionGroup()
    : Glib::OptionGroup("main_group", "Options", "Options"), m_timeout("120s"), m_version(false)
{
    Glib::OptionEntry entry1;
    entry1.set_long_name("timeout");
    entry1.set_short_name('t');
    entry1.set_description("Change IPFS time-out for getting files (default: 120s)");
    add_entry(entry1, m_timeout);

    Glib::OptionEntry entry2;
    entry2.set_long_name("version");
    entry2.set_short_name('v');
    entry2.set_description("Show version");
    add_entry(entry2, m_version);
}

bool OptionGroup::on_pre_parse(Glib::OptionContext &context, Glib::OptionGroup &group)
{
    return Glib::OptionGroup::on_pre_parse(context, group);
}

bool OptionGroup::on_post_parse(Glib::OptionContext &context, Glib::OptionGroup &group)
{
    return Glib::OptionGroup::on_post_parse(context, group);
}

void OptionGroup::on_error(Glib::OptionContext &context, Glib::OptionGroup &group)
{
    Glib::OptionGroup::on_error(context, group);
}