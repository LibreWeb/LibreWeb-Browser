
#include "mainwindow.h"
#include <gtkmm/application.h>

int main(int argc, char *argv[])
{
    auto app = Gtk::Application::create(argc, argv, "org.melroy.browser");

    MainWindow window;
    return app->run(window);
}
