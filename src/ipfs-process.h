#ifndef IPFS_PROCESS_H
#define IPFS_PROCESS_H

#include <string>

/**
 * \class IPFSProcess
 * \brief Helper class to start/stop IPFS deamon, all static methods
 */
class IPFSProcess
{
public:
    static int startIPFSDaemon();

private:
    static pid_t getRunningDaemonPID();
    static bool shouldProcessTerminated(pid_t pid);
    static std::string findIPFSBinary();
};
#endif