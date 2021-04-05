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
    explicit IPFS(const std::string &host, int port, const std::string &timeout);
    std::size_t getNrPeers();
    std::string const getClientID();
    std::string const getClientPublicKey();
    std::string const getVersion();
    std::map<std::string, float> getBandwidthRates();
    std::string const fetch(const std::string &path);
    std::string const add(const std::string &path, const std::string &content);

private:
    std::string host;
    int port;
    std::string timeout;
    ipfs::Client client;
};
#endif