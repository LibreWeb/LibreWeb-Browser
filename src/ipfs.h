#ifndef IPFS_H
#define IPFS_H

#include <string>
#include "ipfs/client.h"

/**
 * \class IPFS
 * \brief Helper class to start/stop IPFS deamon and other IPFS calls
 */
class IPFS
{
public:
    explicit IPFS(const std::string &host, int port);

    static int startIPFSDaemon();
    std::size_t getNrPeers();
    std::map<std::string, float> getBandwidthRates();
private:
    ipfs::Client client;

    static bool shouldKillRunningProcess();
    static std::string findIPFSBinary();
};
#endif