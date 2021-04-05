#include "ipfs.h"
#include <sstream>

/**
 * \brief IPFS Contructor, connect to IPFS
 * \param host IPFS host (eg. localhost)
 * \param port IPFS port number (5001)
 * \param timeout IPFS time-out (which is a string, eg. "6s" for 6 seconds)
 */
IPFS::IPFS(const std::string &host, int port, const std::string &timeout)
    : host(host),
      port(port),
      timeout(timeout),
      client(this->host, this->port, this->timeout) {}

/**
 * \brief Get the number of IPFS peers
 * \return number of peers
 */
std::size_t IPFS::getNrPeers()
{
    try
    {
        ipfs::Json peers;
        client.SwarmPeers(&peers);
        return peers["Peers"].size();
    }
    catch (const std::runtime_error &error)
    {
        // ignore connection issues
    }
    return 0;
}

/**
 * \brief Retrieve your IPFS client ID
 * \return ID
 */
std::string const IPFS::getClientID()
{
    try
    {
        ipfs::Json id;
        client.Id(&id);
        return id["ID"];
    }
    catch (const std::runtime_error &error)
    {
        // ignore connection issues
    }
    return "";
}

/**
 * \brief Retrieve your IPFS Public Key
 * \return Public Key
 */
std::string const IPFS::getClientPublicKey()
{
    try
    {
        ipfs::Json id;
        client.Id(&id);
        return id["PublicKey"];
    }
    catch (const std::runtime_error &error)
    {
        // ignore connection issues
    }
    return "";
}

/**
 * \brief Retrieve the Go IPFS daemon version
 * \return Public Key
 */
std::string const IPFS::getVersion()
{
    try
    {
        ipfs::Json version;
        client.Version(&version);
        return version["Version"];
    }
    catch (const std::runtime_error &error)
    {
        // ignore connection issues
    }
    return "";
}

/**
 * \brief Get the number of IPFS peers
 * \return Map with bandwidth information (with keys: 'in' and 'out')
 */
std::map<std::string, float> IPFS::getBandwidthRates()
{
    std::map<std::string, float> bandwidthRates;
    try
    {
        ipfs::Json bandwidth_info;
        client.StatsBw(&bandwidth_info);
        float in = bandwidth_info["RateIn"];
        float out = bandwidth_info["RateOut"];
        bandwidthRates.insert(std::pair<std::string, float>("in", in));
        bandwidthRates.insert(std::pair<std::string, float>("out", out));
    }
    catch (const std::runtime_error &error)
    {
        // ignore connection issues
    }
    return bandwidthRates;
}

/**
 * \brief Fetch file from IFPS network (create a new client object each time - which is thread-safe), static method
 * \param path File path
 * \throw std::runtime_error when there is a connection-time/something goes wrong while trying to get the file
 * \return content as string
 */
std::string const IPFS::fetch(const std::string &path)
{
    // Create new client each time for thread-safety
    ipfs::Client client(this->host, this->port, this->timeout);
    std::stringstream contents;
    client.FilesGet(path, &contents);
    return contents.str();
}

/**
 * \brief Add a file to IPFS network (not thread-safe)
 * \param path File path where the file could be stored in IPFS (like puting a file inside a directory within IPFS)
 * \param content Content that needs to be written to the IPFS network
 * \throw std::runtime_error when there is a connection-time/something goes wrong while trying to get the file
 * \return IPFS content-addressed identifier (CID) hash
 */
std::string const IPFS::add(const std::string &path, const std::string &content)
{
    ipfs::Json result;
    // Publish a single file
    client.FilesAdd({{path, ipfs::http::FileUpload::Type::kFileContents, content}}, &result);
    if (result.is_array())
    {
        for (const auto &files : result.items())
        {
            return files.value()["hash"];
        }
    }
    // something is wrong, fallback
    return "";
}
