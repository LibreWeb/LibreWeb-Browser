#include "network.h"

#include <sstream>
#include <iostream>

// Connect to IPFS daemon (with 6 seconds time-out for requests)
Network::Network(): m_client("localhost", 5001, "6s") {}

/**
 * Fetch a file from IPFS network
 * \param path IPFS hash / IPFS path
 * \param response IOStream of file contents
 * \throw Could throw run_time error if something goes wrong
 */
void Network::fetchFile(const std::string& path, std::iostream* response) {
    // TODO: What about time-outs, if a request takes too long? eg. file doesn't exists
    m_client.FilesGet(path, response);
}