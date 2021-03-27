#ifndef IPFS_H
#define IPFS_H

#include <string>

/**
 * \class IPFS
 * \brief Helper class to start/stop IPFS deamon
 */
class IPFS
{
public:
    static int startIPFSDaemon();
private:
    static bool shouldKillRunningProcess();
    static std::string findIPFSBinary();
};
#endif