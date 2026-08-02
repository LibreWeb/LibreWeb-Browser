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
  std::string type;     /* A | AAAA | TXT | CNAME | CONTENT */
  std::string value;    /* IP, hostname, text or content hash */
  unsigned int ttl = 0; /* seconds */
};

/**
 * \struct FreedomInfo
 * \brief Node status returned by GET /info, used to populate the status popover.
 */
struct FreedomInfo
{
  std::string mode;      /* DHT mode: Auto | Client | Server | ... */
  std::string node_id;   /* libp2p peer ID of the local node */
  std::size_t peers = 0; /* number of connected hosts */
  int network_size = -1; /* estimated DHT network size (-1 if unknown) */
  /* Node build version. /info reports the same value as /health, so a caller
   * that already polls /info needs no second request to learn it. */
  std::string version;
  /* Peer IDs in the DHT routing table. Deliberately not the same thing as
   * `peers` above: that counts the hosts we currently hold a connection to,
   * while a routing-table entry is a peer we know how to reach. Either can be
   * the larger of the two. */
  std::vector<std::string> routing_table;
  std::vector<std::string> listen_addresses; /* multiaddrs the node listens on */
  std::vector<std::string> protocols;        /* libp2p protocols the node speaks */
};

/**
 * \struct FreedomHealth
 * \brief Liveness + version handshake returned by GET /health.
 */
struct FreedomHealth
{
  std::string version; /* node build version, e.g. "0.8.4" */
  bool ready = false;  /* true once the DHT is initialized */
  /* Node role, since node 0.8.4: "node" (normal) | "bootstrap". Always present
   * on a 0.8.4+ node, even while ready is still false. Empty for older nodes. */
  std::string role;
};

/**
 * \class FreedomNames
 * \brief Abstraction layer to the Freedom Names node JSON HTTP API.
 *
 * This is the freedom-names counterpart of the old IPFS client class. It speaks
 * to a locally-running freedom-names node (default 127.0.0.1:8420) over HTTP and
 * exposes the calls the browser needs: resolve a name to page bytes, store/fetch
 * content by hash, and poll node status.
 */
class FreedomNames
{
public:
  explicit FreedomNames(const std::string& host, int port, const std::string& timeout);
  ~FreedomNames();

  // Resolution
  std::vector<FreedomRecord> resolve(const std::string& name, const std::string& type = std::string());

  // Content: resolve a "label.<pubKeyID>.fn" name to its CONTENT record and
  // stream the page bytes into contents in one call (GET /resolve-content).
  void resolve_content(const std::string& name, std::iostream* contents);

  // Content: fetch raw bytes by content hash (GET /content?hash=), fetching from
  // network providers on a local miss. The fn:// counterpart of `ipfs cat <cid>`.
  void get_content(const std::string& hash, std::iostream* contents);

  // Content: store bytes in the node's blobstore and start providing them
  // (POST /content). Returns the content hash. Replaces `ipfs add`.
  std::string add_content(const std::string& data);

  // Status
  FreedomInfo get_info();
  FreedomHealth get_health();
  std::size_t get_nr_peers();
  std::string get_node_id();

  // Drop the node's resolver cache (DELETE /clear_cache), so the next lookup of
  // every name goes back to the DHT instead of answering from a cached record.
  void clear_cache();

  // Thread control (same contract the middleware relies on for the old IPFS client)
  void abort();
  void reset();

private:
  std::string host_;
  int port_;
  long timeout_seconds_;    /* parsed from the "6s"/"5m" style timeout string */
  std::atomic<bool> abort_; /* set by abort(); checked by the curl progress callback */

  std::string base_url() const;
  // Performs a GET/POST and returns the body; throws std::runtime_error on transport/HTTP error.
  std::string http_get(const std::string& url);
  std::string http_post(const std::string& url, const std::string& body);
  std::string http_delete(const std::string& url);
  std::string perform(void* curl_handle, const std::string& url);
  static std::string url_encode(const std::string& value);
  static long parse_timeout_seconds(const std::string& timeout);
};
#endif
