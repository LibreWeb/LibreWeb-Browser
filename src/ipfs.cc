#include "ipfs.h"
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <iostream>
#include <string.h>

#ifdef LEGACY_CXX
#include <experimental/filesystem>
namespace n_fs = ::std::experimental::filesystem;
#else
#include <filesystem>
namespace n_fs = ::std::filesystem;
#endif

int IPFS::startIPFSDaemon()
{
  // Be sure to kill any running daemons
  std::system("killall -q ipfs");

  /// open /dev/null for writing
  int fd = open("/dev/null", O_WRONLY);

  dup2(fd, 1); // make stdout a copy of fd (> /dev/null)
  dup2(fd, 2); // ..and same with stderr
  close(fd);   // close fd

  // stdout and stderr now write to /dev/null
  // Ready to call exec to start IPFS Daemon
  std::string currentPath = n_fs::current_path().string();
  std::string executable = currentPath.append("/../../go-ipfs/ipfs");
  const char* exe = executable.c_str();
  std::cout << "Info: Starting IPFS Daemon from: " << exe << std::endl;
  char *proc[] = { strdup(exe), strdup("daemon"), strdup("--init"), strdup("--migrate"), NULL};
  return execv(exe, proc);
}

/*
std::string IPFS::getExecutablePath() {
  char buff[PATH_MAX];
  ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
  if (len != -1) {
    buff[len] = '\0';
    return std::string(buff);
  }
  else {
    return "";
  }
}*/
