#ifndef FILE_H
#define FILE_H

#include <string>

/**
 * \class File
 * \brief Read/write markdown files from disk and retrieve filename from path
 */
class File
{
public:
  static std::string read(const std::string& path);
  static void write(const std::string& path, const std::string& content);
  static std::string getFilename(const std::string& path);
};
#endif