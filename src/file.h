#ifndef FILE_H
#define FILE_H

#include <string>

/**
 * \class File
 * \brief Fetch markdown file from disk or the IPFS network
 */
class File
{
public:
    static std::string const read(const std::string &path);  /*!< Read file from disk */
    static std::string const fetch(const std::string &path); /*!< Fetch file from IPFS network */
};
#endif