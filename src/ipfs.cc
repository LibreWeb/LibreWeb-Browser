#include "ipfs.h"
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>

int IPFS::startIPFSDaemon()
{
    // Be sure to kill any running daemons
    std::system("killall -q ipfs");

    /// open /dev/null for writing
    int fd = open("/dev/null", O_WRONLY);

    dup2(fd, 1); // make stdout a copy of fd (> /dev/null)
    dup2(fd, 2); // ..and same with stderr
    close(fd);   // close fd

    // Ignore ISO C++ forbids converting a string constant to ‘char*’
    // This is valid in C.
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wwrite-strings"
    // stdout and stderr now write to /dev/null
    // Ready to call exec to start IPFS Daemon
    constexpr char *proc[] = { "../../go-ipfs/ipfs", "daemon", "--init", "--migrate", NULL};
    #pragma GCC diagnostic pop
    return execv(proc[0], proc);
}