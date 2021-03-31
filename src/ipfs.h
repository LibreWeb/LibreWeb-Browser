#ifndef IPFS_H
#define IPFS_H

#include <string>
#include "ipfs/client.h"

/**
 * \class IPFS
 * \brief Start IPFS connection and contain IPFS related calls
 */
class IPFS
{
public:
    explicit IPFS(const std::string &host, int port);
    std::size_t getNrPeers();
    std::map<std::string, float> getBandwidthRates();
    static std::string const fetch(const std::string &path);
    std::string const add(const std::string &path, const std::string &content);

private:
    ipfs::Client client;
};
#endif