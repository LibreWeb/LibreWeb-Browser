#include "about.h"
#include "project_config.h"

#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

About::About(Gtk::Window& parent)
    : m_visitHomepage("https://libreweb.org/", "Visit Homepage"),
      m_visitProjectLinkButton("https://gitlab.melroy.org/libreweb/browser", "Visit the GitLab Project")
{
  std::vector<Glib::ustring> devs;
  devs.push_back("Melroy van den Berg <info@libreweb.org>");
  std::vector<Glib::ustring> docs;
  docs.push_back("Melroy van den Berg <info@libreweb.org>");

  logo.set(this->getLogoImage());

  set_transient_for(parent);
  set_program_name("LibreWeb Browser");
  set_version(PROJECT_VER);
  set_comments("The fastest decentralized & distributed Browser on planet Earth.");
  set_logo(logo.get_pixbuf());
  set_copyright("Copyright © 2020-2021 Melroy van den Berg");
  set_authors(devs);
  set_artists(devs);
  set_documenters(docs);
  set_license("");
  set_license_type(Gtk::License::LICENSE_MIT_X11);
  set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

  Gtk::Box* vbox = get_vbox();
  vbox->pack_end(m_visitProjectLinkButton, Gtk::PackOptions::PACK_SHRINK);
  vbox->pack_end(m_visitHomepage, Gtk::PackOptions::PACK_SHRINK);
  m_visitHomepage.show();
  m_visitProjectLinkButton.show();
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
  // Use data directory first, used when LibreWeb is installed (Linux or Windows)
  for (std::string data_dir : Glib::get_system_data_dirs())
  {
    std::vector<std::string> path_builder{data_dir, "libreweb", "images", "browser_logo_small.png"};
    std::string file_path = Glib::build_path(G_DIR_SEPARATOR_S, path_builder);
    if (Glib::file_test(file_path, Glib::FileTest::FILE_TEST_IS_REGULAR))
    {
      return file_path;
    }
  }

  // Try local path if the images are not installed (yet)
  // When working directory is in the build/bin folder (relative path)
  std::vector<std::string> path_builder{"..", "..", "images", "browser_logo_small.png"};
  std::string file_path = Glib::build_path(G_DIR_SEPARATOR_S, path_builder);
  if (Glib::file_test(file_path, Glib::FileTest::FILE_TEST_IS_REGULAR))
  {
    return file_path;
  }
  else
  {
    return "";
  }
}
