#include "network.h"

#include <sstream>

// Connect to IPFS daemon
Network::Network()
: client("localhost", 5001)
{
}

void Network::fetchFile(const std::string& path, std::iostream* response) {
    client.FilesGet(path, response);
}