#ifndef NETWORK_H
#define NETWORK_H

#include <ipfs/client.h>

/**
 * \class Network
 * \brief IPFS Network
 */
class Network
{
public:
  Network();
  virtual ~Network();

  void FetchReadme();
private:
    ipfs::Client client;
};
#endif