#ifndef NETWORK_H
#define NETWORK_H

#include <ipfs/client.h>
#include <iostream>
#include <string>

/**
 * \class Network
 * \brief IPFS Network
 */
class Network
{
public:
  Network();
    
  void fetchFile(const std::string& path, std::iostream* response);
private:
    ipfs::Client client;
};
#endif