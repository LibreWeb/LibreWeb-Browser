#include "network.h"

#include <sstream>

Network::Network()
: client("localhost", 5001)
{
}

Network::~Network() {
}

void Network::fetchFile(const std::string& path, std::iostream* response) {
    // Demo file
    client.FilesGet(path, response);
}