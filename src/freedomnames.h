#ifndef FREEDOMNAMES_H
#define FREEDOMNAMES_H

#include <atomic>
#include <iostream>
#include <string>
#include <vector>

/**
 * \struct FreedomRecord
 * \brief A single resolved resource record returned by the freedom-names node.
 * Mirrors the JSON RR shape from the node: {"type","value","ttl"}.
 */
struct FreedomRecord
{
  std::string type;  /* A | AAAA | TXT | CNAME (and CONTENT once the node supports it) */
  std::string value; /* IP, hostname, text or (future) content hash */
  unsigned int ttl;  /* seconds */
};

/**
 * \struct FreedomInfo
 * \brief Node status returned by GET /info, used to populate the status popover.
 * Only the fields the phase-1 node actually exposes are kept here.
 */
struct FreedomInfo
{
  std::string mode;         /* DHT mode: Auto | Client | Server | ... */
  std::string node_id;      /* libp2p peer ID of the local node */
  std::size_t peers;        /* number of connected hosts */
  int network_size;         /* estimated DHT network size (-1 if unknown) */
};

/**
 * \class FreedomNames
 * \brief Abstraction layer to the Freedom Names node JSON HTTP API.
 *
 * This is the freedom-names counterpart of the old IPFS client class. It speaks
 * to a locally-running freedom-names node (default 127.0.0.1:8420) over HTTP and
 * exposes just the calls the browser needs: resolve a name to its records and
 * poll node status. Content fetching (raw page bytes) is a phase-3 node feature;
 * resolve_content() is wired to the future /resolve-content endpoint and will
 * start returning data once the node ships it.
 */
class FreedomNames
{
public:
  explicit FreedomNames(const std::string& host, int port, const std::string& timeout);
  ~FreedomNames();

  // Resolution
  std::vector<FreedomRecord> resolve(const std::string& name, const std::string& type = std::string());

  // Content (phase 3 node feature: GET /resolve-content?name=...). Streams the raw
  // page bytes for a name into contents. Throws std::runtime_error until the node
  // exposes the endpoint (or when resolution/fetch fails).
  void resolve_content(const std::string& name, std::iostream* contents);

  // Status
  FreedomInfo get_info();
  std::size_t get_nr_peers();
  std::string get_node_id();

  // Thread control (same contract the middleware relies on for the old IPFS client)
  void abort();
  void reset();

private:
  std::string host_;
  int port_;
  std::string timeout_seconds_; /* parsed from the "6s"/"5m" style timeout string */
  std::atomic<bool> abort_;     /* set by abort(); checked by the curl progress callback */

  std::string base_url() const;
  // Performs a GET and returns the body; throws std::runtime_error on transport/HTTP error.
  std::string http_get(const std::string& url);
  // Performs a GET streaming the body into out; throws on transport/HTTP error.
  void http_get_stream(const std::string& url, std::iostream* out);
  static std::string url_encode(const std::string& value);
  static long parse_timeout_seconds(const std::string& timeout);
};
#endif
