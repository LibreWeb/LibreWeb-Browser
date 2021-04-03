#include "mainwindow.h"
#include "ipfs-process.h"
#include "project_config.h"
#include "option-group.h"

#include <gtkmm/application.h>
#include <iomanip>
#include <iostream>

/**
 * \brief Entry point of the app
 */
int main(int argc, char *argv[])
{
    // Set the command-line parameters option settings
    Glib::OptionContext context("LibreWeb Browser - Decentralized Web Browser");
    OptionGroup group;
    context.set_main_group(group);

    // Create the GTK application
    auto app = Gtk::Application::create();
    app->set_flags(Gio::ApplicationFlags::APPLICATION_NON_UNIQUE);
    
    try
    {
        // Parse the content
        context.parse(argc, argv);
        if (group.m_version)
        {
            std::cout << "LibreWeb Browser " << PROJECT_VER << std::endl;
            exit(EXIT_SUCCESS);
        }
    }
    catch (const Glib::Error &ex)
    {
        std::cerr << "Parase failure: " << ex.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    pid_t child_pid = fork();
    if (child_pid == 0)
    {
        // Run by child process
        return IPFSProcess::startIPFSDaemon();
    }
    else if (child_pid > 0)
    {
        // Run the GTK window in the parent process (child_pid is the PID of child process)
        MainWindow window(group.m_timeout);
        int exitCode = app->run(window);

        // TODO: If we have multiple browsers running, maybe don't kill the IPFS daemon child process yet..?
        // For now, let's don't kill the IPFS process
        //kill(child_pid, SIGTERM);
        return exitCode;
    }
    else // PID < 0, error
    {
        printf("ERROR: fork failed.\n");
        return EXIT_FAILURE;
    }
}
