#include "file.h"
#include <fstream>
#include <stdexcept>

#ifdef LEGACY_CXX
#include <experimental/filesystem>
namespace n_fs = ::std::experimental::filesystem;
#else
#include <filesystem>
namespace n_fs = ::std::filesystem;
#endif

/**
 * \brief Read file from disk
 * \param path File path location to read the file from
 * \throw std::runtime_error exception when file is not found (or not a regular file),
 *        or std::ios_base::failure when file can't be read
 * \return Contents as string
 */
std::string File::read(const std::string& path)
{
  if (n_fs::exists(path) && n_fs::is_regular_file(path))
  {
    std::ifstream inFile;
    inFile.open(path, std::ifstream::in);

    std::stringstream strStream;
    strStream << inFile.rdbuf();
    return strStream.str();
  }
  else
  {
    // File doesn't exists or isn't a file
    throw std::runtime_error("File does not exists or isn't a regular file.");
  }
}

/**
 * \brief Write file to disk
 * \param path File path location for storing the file
 * \param content Content that needs to be written to file
 * \throw std::ios_base::failure when file can't be written to
 */
void File::write(const std::string& path, const std::string& content)
{
  std::ofstream file;
  file.open(path.c_str());
  file << content;
  file.close();
}

/**
 * \brief Retrieve filename from file path
 * \param path Full path
 * \return filename
 */
std::string File::getFilename(const std::string& path)
{
  return n_fs::path(path).filename().string();
}
