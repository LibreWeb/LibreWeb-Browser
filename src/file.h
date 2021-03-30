#ifndef FILE_H
#define FILE_H

#include <string>

/**
 * \class File
 * \brief Fetch markdown files from disk or the IPFS network, only static calls.
 * \note For publishing content to IPFS, we will use ipfs.h
 */
class File
{
public:
    static std::string const read(const std::string &path);
    static void write(const std::string &path, const std::string &content);
    static std::string const fetch(const std::string &path);
    static std::string const getFilename(const std::string &path);
    /// Note: publishing content to IPFS can be done outside threads, so use ipfs.h
};
#endif