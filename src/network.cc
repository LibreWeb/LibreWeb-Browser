#include "network.h"

#include <sstream>
#include <iostream>

// Connect to IPFS daemon
Network::Network(): client("localhost", 5001) {}

void Network::fetchFile(const std::string& path, std::iostream* response) {
    try {
        client.FilesGet(path, response);
    } catch (const std::runtime_error &error) {
        std::cerr << "IPFS Deamon is most likely down: " << error.what() << std::endl;
    }
}