#include "mainwindow.h"
#include "ipfs-process.h"
#include "project_config.h"
#include <iostream>
#include <gtkmm/application.h>

int main(int argc, char *argv[])
{
    // Any arguments provided?
    if (argc > 1)
    {
        int opt;
        while ((opt = getopt(argc, argv, ":v")) != EOF) // -v is optional
            switch (opt)
            {
            case 'v':
                // Display version, and directly exit the program.
                std::cout << "LibreWeb Browser " << PROJECT_VER << std::endl;
                exit(EXIT_SUCCESS);
                break;
            case 'h':
            case '?': // Unknown
                fprintf(stderr, "Usuage: browser [-v] \nDecentralized Web-Browser, part of the LibreWeb Project\n\nOptions are:\n    -v : output version information and exit\n\n");
                exit(EXIT_SUCCESS);
                break;
            }
    }

    pid_t child_pid = fork();
    if (child_pid == 0)
    {
        // Run by child process
        return IPFSProcess::startIPFSDaemon();
    }
    else if (child_pid > 0)
    {
        // Parent process (child_pid is PID of child)
        auto app = Gtk::Application::create(argc, argv, "org.libreweb.browser");

        MainWindow window;
        int exitCode = app->run(window);
        // Kill also the child
        kill(child_pid, SIGTERM);
        return exitCode;
    }
    else // PID < 0, error
    {
        printf("ERROR: fork failed.\n");
    }
}
