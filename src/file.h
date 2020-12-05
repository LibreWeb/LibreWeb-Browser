#ifndef FILE_H
#define FILE_H

#include "md-parser.h"
#include "network.h"

#include <string>

/**
 * \class File
 * \brief Fetch markdown file from disk or IPFS network and parse to AST
 */
class File
{
public:
    File();

    cmark_node * read(const std::string& path); /*!< Read file from disk */
    cmark_node * fetch(const std::string& path); /*!< Fetch file from IPFS network */
    void free(cmark_node *node);
private:
    Parser parser;
    Network network;
};
#endif