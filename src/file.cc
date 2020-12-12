#include "file.h"

#include <cmark-gfm.h>
#include <stdexcept>
#include <iostream>

File::File() {}

/**
 * Get file from disk
 * \param path File path
 * \return AST model of markdown file (cmark_node)
 */
cmark_node * File::read(const std::string& path)
{
    return parser.parseFile(path);
}

/**
 * Fetch file from IFPS network
 * \param path File path
 * \throw runtime error when something goes wrong
 * \return AST model of markdown file (cmark_node)
 */
cmark_node * File::fetch(const std::string& path)
{
    std::stringstream contents;
    network.fetchFile(path, &contents);
    return parser.parseStream(contents);
}

std::string const File::getSource(cmark_node *node)
{
  return parser.getSource(node);
}

/**
 * Free AST cmark_node memory, to avoid memory leaks
 */
void File::free(cmark_node *node)
{
    cmark_node_free(node);
}
