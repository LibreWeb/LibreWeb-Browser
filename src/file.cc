#include "file.h"

#include <cmark-gfm.h>
#include <stdexcept>
#include <iostream>

File::File() {}

cmark_node * File::read(const std::string& path)
{
    return parser.parseFile(path);
}

cmark_node * File::fetch(const std::string& path)
{
    std::stringstream contents;
    network.fetchFile(path, &contents);
    return parser.parseStream(contents);
}

void File::free(cmark_node *node)
{
    cmark_node_free(node);
}
