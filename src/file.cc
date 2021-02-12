#include "file.h"
#include <ipfs/client.h>
#include <stdexcept>
#include <fstream>
#include <sstream>

#ifdef LEGACY_CXX
#include <experimental/filesystem>
namespace n_fs = ::std::experimental::filesystem;
#else
#include <filesystem>
namespace n_fs = ::std::filesystem;
#endif

File::File()
{
}

/**
 * Get file from disk
 * \param path File path
 * \return AST model of markdown file (cmark_node)
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
 * Fetch file from IFPS network (create a new client connection for thread safety)
 * \param path File path
 * \throw runtime error when something goes wrong
 * \return AST model of markdown file (cmark_node)
 */
std::string const File::fetch(const std::string &path)
{
    ipfs::Client client("localhost", 5001, "6s");
    std::stringstream contents;
    client.FilesGet(path, &contents);
    return contents.str();
}
