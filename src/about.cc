#include "about.h"
#include "project_config.h"

#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>

About::About()
{
    std::vector<Glib::ustring> devs;
    devs.push_back("Melroy van den Berg <melroy@melroy.org>");
    logo.set(this->getLogoImage());

    set_program_name("LibreWeb Browser");
    set_version(PROJECT_VER);
    set_comments("The fastest decentralized & distributed Browser on planet Earth.");
    set_logo(logo.get_pixbuf());
    set_website("https://libreweb.org/");
    set_copyright("Copyright © 2020-2021 Melroy van den Berg");
    set_authors(devs);
    set_artists(devs);
    set_license_type(Gtk::License::LICENSE_MIT_X11);
    set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

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

std::string About::getLogoImage()
{
    // Try absolute path first
    for (std::string data_dir : Glib::get_system_data_dirs())
    {
        std::vector<std::string> path_builder{data_dir, "libreweb-browser", "images", "browser_logo_small.png"};
        std::string file_path = Glib::build_path(G_DIR_SEPARATOR_S, path_builder);
        if (Glib::file_test(file_path, Glib::FileTest::FILE_TEST_IS_REGULAR))
        {
            return file_path;
        }
    }

    // Try local path if the images are not installed (yet)
    // When working directory is in the build/bin folder (relative path)
    std::string file_path = Glib::build_filename("../../images", "browser_logo_small.png");
    if (Glib::file_test(file_path, Glib::FileTest::FILE_TEST_IS_REGULAR))
    {
        return file_path;
    }
    else
    {
        return "";
    }
}