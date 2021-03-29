#include "file.h"
#include <ipfs/client.h>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>

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
std::string const File::read(const std::string &path)
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
void File::write(const std::string &path, const std::string &content)
{
    std::ofstream file;
    file.open(path.c_str());
    file << content;
    file.close();
}

/**
 * \brief Fetch file from IFPS network (create a new client connection for thread safety)
 * \param path File path
 * \throw runtime error when something goes wrong
 * \return content as string
 */
std::string const File::fetch(const std::string &path)
{
    ipfs::Client client("localhost", 5001, "6s");
    std::stringstream contents;
    client.FilesGet(path, &contents);
    return contents.str();
}

/**
 * \brief Publish file to IPFS network (does *not* need to be thead-safe, but is thread-safe nevertheless now)
 * \param filename Filename that gets stored in IPFS
 * \param content Content that needs to be written to the IPFS network
 * \return IPFS content-addressed identifier (CID)
 */
std::string const File::publish(const std::string &filename, const std::string &content)
{
    // TODO: Publish file to IPFS
    return "CID";
}

/**
 * \brief Retrieve filename from file path
 * \param path Full path 
 * \return filename
 */
std::string const File::getFilename(const std::string &path)
{
    return n_fs::path(path).filename();
}
