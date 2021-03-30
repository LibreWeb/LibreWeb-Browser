#include "ipfs.h"

/**
 * \brief IPFS Contructor, connect to IPFS
 */
IPFS::IPFS(const std::string &host, int port) : client(host, port, "6s") {}

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
 * \brief Publish file to IPFS network
 * \param filename Filename that gets stored in IPFS
 * \param content Content that needs to be written to the IPFS network
 * \return IPFS content-addressed identifier (CID)
 */
std::string const IPFS::publish(const std::string &filename, const std::string &content)
{
    // TODO: Publish file to IPFS
    return "CID";
}
