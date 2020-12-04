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
    client.FilesGet("QmQzhn6hEfbYdCfwzYFsSt3eWpubVKA1dNqsgUwci5vHwq", &contents);
    std::cout << contents.str() << std::endl;
}