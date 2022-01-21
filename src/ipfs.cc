#include "ipfs.h"

/**
 * \brief IPFS Contructor, connect to IPFS
 * \param host IPFS host (eg. localhost)
 * \param port IPFS port number (5001)
 * \param timeout IPFS time-out (which is a string, eg. "6s" for 6 seconds)
 */
IPFS::IPFS(const std::string& host, int port, const std::string& timeout)
    : host(host),
      port(port),
      timeout(timeout),
      client(this->host, this->port, this->timeout)
{
}

/**
 * \brief Get the number of IPFS peers. Does not throw errors.
 * \return number of peers as size_t
 */
std::size_t IPFS::getNrPeers()
{
  ipfs::Json peers;
  client.SwarmPeers(&peers);
  return peers["Peers"].size();
}

/**
 * \brief Retrieve your IPFS client ID. Does not throw errors.
 * \return ID as string
 */
std::string IPFS::getClientID()
{
  ipfs::Json id;
  client.Id(&id);
  return id["ID"];
}

/**
 * \brief Retrieve your IPFS Public Key. Does not throw errors.
 * \return Public key string
 */
std::string IPFS::getClientPublicKey()
{
  ipfs::Json id;
  client.Id(&id);
  return id["PublicKey"];
}

/**
 * \brief Retrieve the Go IPFS daemon version. Does not throw errors.
 * \return Version string
 */
std::string IPFS::getVersion()
{
  ipfs::Json version;
  client.Version(&version);
  return version["Version"];
}

/**
 * \brief Get the number of IPFS peers. Does not throw errors.
 * \return Map with bandwidth information (with keys: 'in' and 'out')
 */
std::map<std::string, float> IPFS::getBandwidthRates()
{
  std::map<std::string, float> bandwidthRates;
  ipfs::Json bandwidth_info;
  client.StatsBw(&bandwidth_info);
  float in = bandwidth_info["RateIn"];
  float out = bandwidth_info["RateOut"];
  bandwidthRates.insert(std::pair<std::string, float>("in", in));
  bandwidthRates.insert(std::pair<std::string, float>("out", out));
  return bandwidthRates;
}

/**
 * \brief Get the stats of the current Repo. Does not throw errors.
 * \return Map with repo stats (with keys: 'repo-size' and 'path')
 */
std::map<std::string, std::variant<int, std::string>> IPFS::getRepoStats()
{
  std::map<std::string, std::variant<int, std::string>> repoStats;
  ipfs::Json repo_stats;
  client.StatsRepo(&repo_stats);
  int repoSize = (int)repo_stats["RepoSize"] / 1000000; // Convert from bytes to MB
  std::string repoPath = repo_stats["RepoPath"];
  repoStats.insert(std::pair<std::string, int>("repo-size", repoSize));
  repoStats.insert(std::pair<std::string, std::string>("path", repoPath));
  return repoStats;
}

/**
 * \brief Fetch file from IFPS network
 * \param path File path
 * \param contents File contents as iostream
 * \throw std::runtime_error when there is a
 * connection-time/something goes wrong while trying to get the file
 */
void IPFS::fetch(const std::string& path, std::iostream* contents)
{
  client.FilesGet(path, contents);
}

/**
 * \brief Add a file to IPFS network
 * \param path File path where the file could be stored in IPFS (like putting a file inside a directory within IPFS)
 * \param content Content that needs to be written to the IPFS network
 * \throw std::runtime_error when there is a connection-time/something goes wrong while adding the file
 * \return IPFS content-addressed identifier (CID) hash
 */
std::string IPFS::add(const std::string& path, const std::string& content)
{
  ipfs::Json result;
  std::string hash;
  // Publish a single file
  client.FilesAdd({{path, ipfs::http::FileUpload::Type::kFileContents, content}}, &result);
  if (result.is_array() && result.size() > 0)
  {
    for (const auto& files : result.items())
    {
      hash = files.value()["hash"];
      break;
    }
  }
  else
  {
    throw std::runtime_error("File is not added, result is incorrect.");
  }
  return hash;
}

/**
 * Abort the request abruptly. Used for stopping the thread.
 */
void IPFS::abort()
{
  client.Abort();
}

/**
 * Reset the state, to allow for new API IPFS requests. Used after the thread.join() and abort() call.
 */
void IPFS::reset()
{
  client.Reset();
}
