#include "about.h"

About::About() {
    std::vector<Glib::ustring> devs;
    devs.push_back("Melroy van den Berg <melroy@melroy.org>");
    icon.set_from_icon_name("emblem-web", Gtk::IconSize(Gtk::ICON_SIZE_DIALOG));

    set_name("Browser");
    set_version("0.1.0");
    set_comments("The fastest decentralized browser on planet Earth.");
    set_icon(icon.get_pixbuf());
	set_website("https://melroy.org/");
    set_copyright("Copyright © 2020-2021 Melroy van den Berg");
    set_authors(devs);
    set_artists(devs);
    set_license_type(Gtk::License::LICENSE_MIT_X11);

    show_all_children();
}

About::~About()
{
}

void About::show_about()
{
    run();
}

void About::hide_about(__attribute__((unused)) int response)
{
    hide();
}