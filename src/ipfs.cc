#include "ipfs.h"
#include <cstdlib>
#include <unistd.h>

int IPFS::startIPFSDaemon()
{
    // Be sure to kill any running daemons
    std::system("killall -q ipfs");

    // Start IPFS daemon
    constexpr char *proc[] = { "../../go-ipfs/ipfs", "daemon", "--init", "--migrate", 0};
    return execv(proc[0], proc);
}