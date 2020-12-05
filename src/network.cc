#include "network.h"

#include <sstream>

// Connect to IPFS daemon
Network::Network()
: client(NULL)
{
    try {
        client = new ipfs::Client("localhost", 5001);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        // Something else
        client = NULL;
    } catch (...) {
        std::cerr << "Something else!?" << std::endl;
        client = NULL;
    }
}

void Network::fetchFile(const std::string& path, std::iostream* response) {
    if (client != NULL)
        client->FilesGet(path, response);
}