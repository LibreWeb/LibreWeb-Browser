#ifndef IPFS_H
#define IPFS_H

#include "ipfs/client.h"
#include <iostream>
#include <map>
#include <string>
#include <variant>

/**
 * \class IPFS
 * \brief IPFS Abstraction Layer to the C++ IPFS HTTP Client
 */
class IPFS
{
public:
  explicit IPFS(const std::string& host, int port, const std::string& timeout);
  std::size_t get_nr_peers();
  std::string get_client_id();
  std::string get_client_public_key();
  std::string get_version();
  std::map<std::string, float> get_bandwidth_rates();
  std::map<std::string, std::variant<int, std::string>> get_repo_stats();
  void fetch(const std::string& path, std::iostream* contents);
  std::string add(const std::string& path, const std::string& content);
  void abort();
  void reset();

private:
  std::string host_;    /* IPFS host name */
  int port_;            /* IFPS port number */
  std::string timeout_; /* IPFS timeout (eg. 6s) */
  ipfs::Client client_; /* IPFS Client object */
};
#endif