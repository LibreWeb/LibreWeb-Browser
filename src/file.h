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
    static std::string const read(const std::string &path);
    static void write(const std::string &path, const std::string &content);
    static std::string const fetch(const std::string &path);
    static std::string const publish(const std::string &filename, const std::string &content);
};
#endif