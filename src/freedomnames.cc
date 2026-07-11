#include "freedomnames.h"

#include <cctype>
#include <cstdio>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace
{
// libcurl write callback appending to a std::string.
size_t write_to_string(char* ptr, size_t size, size_t nmemb, void* userdata)
{
  auto* out = static_cast<std::string*>(userdata);
  out->append(ptr, size * nmemb);
  return size * nmemb;
}

// How long to wait for the TCP connect to the (local) node.
constexpr long kConnectTimeoutSeconds = 30;

// Progress callback: aborts the transfer when the shared abort flag is set.
int progress_callback(void* clientp, curl_off_t, curl_off_t, curl_off_t, curl_off_t)
{
  auto* aborted = static_cast<std::atomic<bool>*>(clientp);
  return (aborted && aborted->load()) ? 1 : 0; // non-zero aborts the transfer
}
} // namespace

/**
 * \brief FreedomNames constructor.
 * \param host node host (eg. 127.0.0.1)
 * \param port node HTTP API port (default 8420)
 * \param timeout time-out string like "6s" or "5m" (matched to the IPFS client contract)
 */
FreedomNames::FreedomNames(const std::string& host, int port, const std::string& timeout)
    : host_(host), port_(port), timeout_seconds_(parse_timeout_seconds(timeout)), abort_(false)
{
}

FreedomNames::~FreedomNames()
{
}

std::string FreedomNames::base_url() const
{
  return "http://" + host_ + ":" + std::to_string(port_);
}

/**
 * \brief Resolve a "label.<pubKeyID>.fn" name to its resource records.
 * \param name full freedom name
 * \param type optional record type filter (A|AAAA|TXT|CNAME|...)
 * \return list of resource records
 * \throw std::runtime_error on transport/HTTP/parse error
 */
std::vector<FreedomRecord> FreedomNames::resolve(const std::string& name, const std::string& type)
{
  std::string url = base_url() + "/resolve?name=" + url_encode(name);
  if (!type.empty())
    url += "&type=" + url_encode(type);

  std::string body = http_get(url);

  std::vector<FreedomRecord> records;
  auto json = nlohmann::json::parse(body);
  if (json.contains("records") && json["records"].is_array())
  {
    for (const auto& rr : json["records"])
    {
      FreedomRecord record;
      record.type = rr.value("type", "");
      record.value = rr.value("value", "");
      record.ttl = rr.value("ttl", 0u);
      records.push_back(record);
    }
  }
  return records;
}

/**
 * \brief Resolve a name to its CONTENT record and stream the page bytes into
 * contents, in one GET /resolve-content call.
 */
void FreedomNames::resolve_content(const std::string& name, std::iostream* contents)
{
  const std::string url = base_url() + "/resolve-content?name=" + url_encode(name);
  *contents << http_get(url);
}

/**
 * \brief Fetch raw bytes by content hash; the node serves from its local store
 * or fetches from network providers on a miss.
 */
void FreedomNames::get_content(const std::string& hash, std::iostream* contents)
{
  const std::string url = base_url() + "/content?hash=" + url_encode(hash);
  *contents << http_get(url);
}

/**
 * \brief Store bytes in the node's blobstore and start providing them.
 * \return the content hash (base36 sha2-256 multihash)
 * \throw std::runtime_error on transport/HTTP/parse error
 */
std::string FreedomNames::add_content(const std::string& data)
{
  const std::string body = http_post(base_url() + "/content", data);
  auto json = nlohmann::json::parse(body);
  return json.value("hash", "");
}

/**
 * \brief Get node status from GET /info.
 * \return FreedomInfo with the fields the phase-1 node exposes
 * \throw std::runtime_error on transport/HTTP/parse error
 */
FreedomInfo FreedomNames::get_info()
{
  std::string body = http_get(base_url() + "/info");
  auto json = nlohmann::json::parse(body);

  FreedomInfo info;
  info.mode = json.value("mode", "");
  info.node_id = json.value("peerID", "");
  info.peers = json.value("hostsConnected", 0);
  info.network_size = json.value("networkSize", -1);
  return info;
}

/**
 * \brief Get the node's liveness + version handshake from GET /health.
 * \throw std::runtime_error on transport/HTTP/parse error
 */
FreedomHealth FreedomNames::get_health()
{
  std::string body = http_get(base_url() + "/health");
  auto json = nlohmann::json::parse(body);

  FreedomHealth health;
  health.version = json.value("version", "");
  health.ready = json.value("ready", false);
  return health;
}

std::size_t FreedomNames::get_nr_peers()
{
  return get_info().peers;
}

std::string FreedomNames::get_node_id()
{
  return get_info().node_id;
}

/**
 * \brief Abort an in-flight request; the progress callback stops the transfer.
 */
void FreedomNames::abort()
{
  abort_ = true;
}

/**
 * \brief Reset abort state so new requests can run (after thread.join()).
 */
void FreedomNames::reset()
{
  abort_ = false;
}

/************************************************
 * Private helpers
 ************************************************/

/**
 * \brief Perform an already-initialized curl request and return the response body.
 * Takes ownership of the handle (always cleans it up).
 *
 * Time-out semantics: the configured time-out acts as an *idle* time-out (via
 * CURLOPT_LOW_SPEED_*) rather than a cap on total transfer time, so a large
 * page that is still making progress is never killed mid-download, while a node
 * that silently hangs (e.g. a long DHT lookup) still errors after the time-out.
 *
 * Error contract (matched by the middleware):
 *  - "Request was aborted"                          — abort() was called
 *  - "Request timed out: ..."                       — idle/connect time-out
 *  - "Couldn't connect to server: ..."              — node not (yet) reachable
 *  - "Request failed: ..."                          — any other transport error
 *  - "HTTP request failed with status code N: body" — non-2xx response
 */
std::string FreedomNames::perform(void* curl_handle, const std::string& url)
{
  CURL* curl = static_cast<CURL*>(curl_handle);
  std::string response;
  long http_code = 0;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, kConnectTimeoutSeconds);
  curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
  curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, timeout_seconds_);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
  curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &abort_);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl);

  switch (res)
  {
  case CURLE_OK:
    break;
  case CURLE_ABORTED_BY_CALLBACK:
    throw std::runtime_error("Request was aborted");
  case CURLE_OPERATION_TIMEDOUT:
    throw std::runtime_error(std::string("Request timed out: ") + curl_easy_strerror(res));
  case CURLE_COULDNT_CONNECT:
  case CURLE_COULDNT_RESOLVE_HOST:
    throw std::runtime_error(std::string("Couldn't connect to server: ") + curl_easy_strerror(res));
  default:
    throw std::runtime_error(std::string("Request failed: ") + curl_easy_strerror(res));
  }
  if (http_code < 200 || http_code >= 300)
    throw std::runtime_error("HTTP request failed with status code " + std::to_string(http_code) + ": " + response);

  return response;
}

/**
 * \brief Perform a GET request and return the response body.
 */
std::string FreedomNames::http_get(const std::string& url)
{
  CURL* curl = curl_easy_init();
  if (!curl)
    throw std::runtime_error("Could not init curl");
  return perform(curl, url);
}

/**
 * \brief Perform a POST request with a raw body and return the response body.
 */
std::string FreedomNames::http_post(const std::string& url, const std::string& body)
{
  CURL* curl = curl_easy_init();
  if (!curl)
    throw std::runtime_error("Could not init curl");
  // body outlives perform(), so curl may reference it directly
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
  return perform(curl, url);
}

/**
 * \brief Minimal percent-encoding for query parameter values.
 */
std::string FreedomNames::url_encode(const std::string& value)
{
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;
  for (unsigned char c : value)
  {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
    {
      escaped << c;
    }
    else
    {
      escaped << '%' << std::uppercase;
      escaped.width(2);
      escaped << static_cast<int>(c);
      escaped << std::nouppercase;
      escaped.width(0);
    }
  }
  return escaped.str();
}

/**
 * \brief Parse an IPFS-style timeout string ("6s", "5m", "120s") into seconds.
 * Falls back to 120s when the value can't be parsed.
 */
long FreedomNames::parse_timeout_seconds(const std::string& timeout)
{
  if (timeout.empty())
    return 120;
  try
  {
    size_t pos = 0;
    long value = std::stol(timeout, &pos);
    char unit = (pos < timeout.size()) ? timeout[pos] : 's';
    switch (unit)
    {
    case 'm':
      return value * 60;
    case 'h':
      return value * 3600;
    case 's':
    default:
      return value;
    }
  }
  catch (const std::exception&)
  {
    return 120;
  }
}
