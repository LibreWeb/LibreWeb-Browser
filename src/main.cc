#include "mainwindow.h"
#include "ipfs.h"
#include <gtkmm/application.h>

int main(int argc, char *argv[])
{
  pid_t child_pid = fork();
  if (child_pid == 0)
  {
    // Run by child process
    return IPFS::startIPFSDaemon();
  }
  else if (child_pid > 0 )
  {
    // Parent process (child_pid is PID of child)
    auto app = Gtk::Application::create(argc, argv, "org.melroy.browser");

    MainWindow window;
    int exitCode =  app->run(window);
    // Kill also the child
    kill(child_pid, SIGTERM);
    return exitCode;
  }
  else // PID < 0, error
  {
    printf("ERROR: fork failed.\n");
  }
}
