#include "about.h"

About::About() {
    set_name("Browser");
    set_version("0.1");

	set_website("https://melroy.org/");
    set_copyright("Copyright © 2020-2021 Melroy van den Berg");

    std::vector<Glib::ustring> devs;
    devs.push_back("Melroy van den Berg");
    set_authors(devs);
    set_artists(devs);
    
    set_license_type(Gtk::License::LICENSE_MIT_X11);

    show_all_children();
}

About::~About()
{
}
