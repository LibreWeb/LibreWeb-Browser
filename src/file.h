#ifndef FILE_H
#define FILE_H

#include "network.h"

#include <string>

/**
 * \class File
 * \brief Fetch markdown file from disk or IPFS network
 */
class File
{
public:
  File();

  std::string const read(const std::string& path); /*!< Read file from disk */
  std::string const fetch(const std::string& path); /*!< Fetch file from IPFS network */
private:
  Network network;
};
#endif