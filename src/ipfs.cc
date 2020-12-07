#include "ipfs.h"
#include <unistd.h>

int IPFS::startIPFSDaemon()
{
    // Start IPFS daemon
    constexpr char *proc[] = { "../../go-ipfs/ipfs", "daemon", "--init", "--migrate", 0};
    return execv(proc[0], proc);
}