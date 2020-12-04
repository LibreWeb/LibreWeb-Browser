#include "network.h"

#include <iostream>
#include <sstream>

Network::Network()
: client("localhost", 5001)
{
}

Network::~Network() {
}

void Network::FetchReadme() {
    // Demo ...
    std::stringstream contents;
    client.FilesGet("/ipfs/QmYwAPJzv5CZsnA625s3Xf2nemtYgPpHdWEz79ojWnPbdG/readme", &contents);
    std::cout << contents.str() << std::endl;
}