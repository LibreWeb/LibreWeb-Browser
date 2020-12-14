#include "file.h"

#include <stdexcept>
#include <fstream>
#include <sstream> 
#include <iostream>

File::File() {}

/**
 * Get file from disk
 * \param path File path
 * \return AST model of markdown file (cmark_node)
 */
std::string const File::read(const std::string& path)
{
  std::ifstream inFile;
  inFile.open(path, std::ifstream::in);

  std::stringstream strStream;
  strStream << inFile.rdbuf();
  return strStream.str();
}

/**
 * Fetch file from IFPS network
 * \param path File path
 * \throw runtime error when something goes wrong
 * \return AST model of markdown file (cmark_node)
 */
std::string const File::fetch(const std::string& path)
{
    std::stringstream contents;
    network.fetchFile(path, &contents);
    return contents.str();
}
