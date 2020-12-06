#include "network.h"

#include <sstream>
#include <iostream>

// Connect to IPFS daemon
Network::Network(): m_client("localhost", 5001) {}

/**
 * Fetch a file from IPFS network
 * \param path IPFS hash / IPFS path
 * \param response IOStream of file contents
 * \throw Could throw run_time error if something goes wrong
 */
void Network::fetchFile(const std::string& path, std::iostream* response) {
    m_client.FilesGet(path, response);
}