#include "ipfs.h"

/**
 * \brief IPFS Contructor, connect to IPFS
 * \param host IPFS host (eg. localhost)
 * \param port IPFS port number (5001)
 * \param timeout IPFS time-out (which is a string, eg. "6s" for 6 seconds)
 */
IPFS::IPFS(const std::string& host, int port, const std::string& timeout)
    : host_(host),
      port_(port),
      timeout_(timeout),
      client_(this->host_, this->port_, this->timeout_)
{
}

/**
 * \brief Get the number of IPFS peers.
 * \return number of peers as size_t
 */
std::size_t IPFS::get_nr_peers()
{
  ipfs::Json peers;
  client_.SwarmPeers(&peers);
  return peers["Peers"].size();
}

/**
 * \brief Retrieve your IPFS client ID.
 * \return ID as string
 */
std::string IPFS::get_client_id()
{
  ipfs::Json id;
  client_.Id(&id);
  return id["ID"];
}

/**
 * \brief Retrieve your IPFS Public Key.
 * \return Public key string
 */
std::string IPFS::get_client_public_key()
{
  ipfs::Json id;
  client_.Id(&id);
  return id["PublicKey"];
}

/**
 * \brief Retrieve the Go IPFS daemon version.
 * \return Version string
 */
std::string IPFS::get_version()
{
  ipfs::Json version;
  client_.Version(&version);
  return version["Version"];
}

/**
 * \brief Get the number of IPFS peers.
 * \return Map with bandwidth information (with keys: 'in' and 'out')
 */
std::map<std::string, float> IPFS::get_bandwidth_rates()
{
  std::map<std::string, float> bandwidth_rates;
  ipfs::Json bandwidth_info;
  client_.StatsBw(&bandwidth_info);
  float in = bandwidth_info["RateIn"];
  float out = bandwidth_info["RateOut"];
  bandwidth_rates.insert(std::pair<std::string, float>("in", in));
  bandwidth_rates.insert(std::pair<std::string, float>("out", out));
  return bandwidth_rates;
}

/**
 * \brief Get the stats of the current Repo.
 * \return Map with repo stats (with keys: 'repo-size' and 'path')
 */
std::map<std::string, std::variant<int, std::string>> IPFS::get_repo_stats()
{
  std::map<std::string, std::variant<int, std::string>> repo_stats;
  ipfs::Json repo_stats_info;
  client_.StatsRepo(&repo_stats_info);
  int repo_size = (int)repo_stats_info["RepoSize"] / 1000000; // Convert from bytes to MB
  std::string repo_path = repo_stats_info["RepoPath"];
  repo_stats.insert(std::pair<std::string, int>("repo-size", repo_size));
  repo_stats.insert(std::pair<std::string, std::string>("path", repo_path));
  return repo_stats;
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
  client_.FilesGet(path, contents);
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
  client_.FilesAdd({{path, ipfs::http::FileUpload::Type::kFileContents, content}}, &result);
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
  client_.Abort();
}

/**
 * Reset the state, to allow for new API IPFS requests. Used after the thread.join() and abort() call.
 */
void IPFS::reset()
{
  client_.Reset();
}
