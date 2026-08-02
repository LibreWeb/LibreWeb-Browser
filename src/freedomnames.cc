#include "freedomnames.h"

#include <algorithm>
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

  // Parse a node response body, translating nlohmann's exceptions into the
  // std::runtime_error every method here documents. nlohmann::json::parse_error
  // and type_error derive from std::exception, *not* std::runtime_error, so
  // without this a malformed but 2xx body escapes every caller's catch and
  // terminates the browser.
  nlohmann::json parse_response(const std::string& body, const std::string& endpoint)
  {
    try
    {
      return nlohmann::json::parse(body);
    }
    catch (const nlohmann::json::exception& error)
    {
      throw std::runtime_error("Malformed JSON from the Freedom Names node at " + endpoint + ": " + error.what());
    }
  }

  // Read an array-of-strings status field, skipping anything that isn't a
  // string. These fields only feed the status pop-over, so one odd entry should
  // never cost the caller the rest of the update.
  std::vector<std::string> string_array(const nlohmann::json& json, const char* key)
  {
    std::vector<std::string> values;
    const auto field = json.find(key);
    if (field == json.end() || !field->is_array())
      return values;
    for (const auto& entry : *field)
    {
      if (entry.is_string())
        values.push_back(entry.get<std::string>());
    }
    return values;
  }

  bool valid_port(const std::string& value)
  {
    if (value.empty() || !std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c); }))
      return false;
    try
    {
      const unsigned long port = std::stoul(value);
      return port > 0 && port <= 65535;
    }
    catch (const std::exception&)
    {
      return false;
    }
  }

  bool valid_ipv4_loopback(const std::string& host)
  {
    if (host.rfind("127.", 0) != 0)
      return false;
    int dots = 0;
    std::size_t start = 0;
    while (start < host.size())
    {
      const std::size_t end = host.find('.', start);
      const std::string part = host.substr(start, end == std::string::npos ? std::string::npos : end - start);
      if (part.empty() || part.size() > 3 || !std::all_of(part.begin(), part.end(), [](unsigned char c) { return std::isdigit(c); }))
        return false;
      if (std::stoul(part) > 255)
        return false;
      if (end == std::string::npos)
        break;
      dots++;
      start = end + 1;
    }
    return dots == 3;
  }
} // namespace

bool FreedomHealth::has_capability(const std::string& capability) const
{
  return std::find(capabilities.begin(), capabilities.end(), capability) != capabilities.end();
}

/**
 * \brief FreedomNames constructor.
 * \param host node host (eg. 127.0.0.1)
 * \param port node HTTP API port (default 8420)
 * \param timeout time-out string like "6s" or "5m" (matched to the IPFS client contract)
 */
FreedomNames::FreedomNames(const std::string& host, int port, const std::string& timeout)
    : host_(host),
      port_(port),
      timeout_seconds_(parse_timeout_seconds(timeout)),
      abort_(false)
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
  auto json = parse_response(body, "/resolve");
  if (json.contains("records") && json["records"].is_array())
  {
    for (const auto& rr : json["records"])
    {
      if (!rr.is_object())
        continue; // skip a malformed entry rather than failing the whole lookup
      FreedomRecord record;
      try
      {
        record.type = rr.value("type", "");
        record.value = rr.value("value", "");
        record.ttl = rr.value("ttl", 0u);
      }
      catch (const nlohmann::json::exception&)
      {
        continue; // a field with an unexpected type: drop just this record
      }
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
  auto json = parse_response(body, "/content");
  try
  {
    return json.value("hash", "");
  }
  catch (const nlohmann::json::exception& error)
  {
    throw std::runtime_error(std::string("Unexpected /content payload from the Freedom Names node: ") + error.what());
  }
}

/**
 * \brief List owner names managed by the local node.
 */
std::vector<FreedomName> FreedomNames::list_names(const std::string& authoring_api)
{
  if (!is_loopback_http_origin(authoring_api))
    throw std::runtime_error("The Freedom Names node did not advertise a safe local authoring API.");

  const auto json = parse_response(http_get(authoring_api + "/authoring/names"), "/authoring/names");
  const auto names = json.find("names");
  if (names == json.end() || !names->is_array())
    throw std::runtime_error("Unexpected /authoring/names payload from the Freedom Names node.");

  std::vector<FreedomName> result;
  try
  {
    for (const auto& entry : *names)
    {
      if (!entry.is_object() || !entry.contains("label") || !entry["label"].is_string() || !entry.contains("name") || !entry["name"].is_string())
        throw std::runtime_error("Unexpected name entry from the Freedom Names node.");
      result.push_back({entry["label"].get<std::string>(), entry["name"].get<std::string>()});
    }
  }
  catch (const nlohmann::json::exception& error)
  {
    throw std::runtime_error(std::string("Unexpected /authoring/names payload from the Freedom Names node: ") + error.what());
  }
  return result;
}

/**
 * \brief Create an owner key for label and return its full name.
 */
std::string FreedomNames::create_name(const std::string& authoring_api, const std::string& label)
{
  if (!is_loopback_http_origin(authoring_api))
    throw std::runtime_error("The Freedom Names node did not advertise a safe local authoring API.");

  const nlohmann::json request = {{"label", label}};
  const auto response = parse_response(http_post_json(authoring_api + "/authoring/names", request.dump()), "/authoring/names");
  try
  {
    const std::string name = response.value("name", "");
    if (name.empty())
      throw std::runtime_error("The Freedom Names node returned an empty name after creating the owner key.");
    return name;
  }
  catch (const nlohmann::json::exception& error)
  {
    throw std::runtime_error(std::string("Unexpected /authoring/names payload from the Freedom Names node: ") + error.what());
  }
}

/**
 * \brief Point an owned name at content_hash and return the full published name.
 */
std::string FreedomNames::publish_name(const std::string& authoring_api, const std::string& label, const std::string& content_hash)
{
  if (!is_loopback_http_origin(authoring_api))
    throw std::runtime_error("The Freedom Names node did not advertise a safe local authoring API.");

  const nlohmann::json request = {{"records", nlohmann::json::array({{{"type", "CONTENT"}, {"value", content_hash}, {"ttl", 300}}})}};
  const std::string endpoint = "/authoring/names/" + url_encode(label) + "/publish";
  const auto response = parse_response(http_post_json(authoring_api + endpoint, request.dump()), endpoint);
  try
  {
    const std::string name = response.value("published", "");
    if (name.empty())
      throw std::runtime_error("The Freedom Names node returned an empty name after publishing.");
    return name;
  }
  catch (const nlohmann::json::exception& error)
  {
    throw std::runtime_error(std::string("Unexpected ") + endpoint + " payload from the Freedom Names node: " + error.what());
  }
}

/**
 * \brief Get node status from GET /info.
 * \return FreedomInfo with the fields the phase-1 node exposes
 * \throw std::runtime_error on transport/HTTP/parse error
 */
FreedomInfo FreedomNames::get_info()
{
  std::string body = http_get(base_url() + "/info");
  auto json = parse_response(body, "/info");

  FreedomInfo info;
  try
  {
    info.mode = json.value("mode", "");
    info.node_id = json.value("peerID", "");
    info.peers = json.value("hostsConnected", 0);
    info.network_size = json.value("networkSize", -1);
    info.version = json.value("version", "");
    info.routing_table = string_array(json, "peers");
    info.listen_addresses = string_array(json, "listenAddresses");
    info.protocols = string_array(json, "protocols");
  }
  catch (const nlohmann::json::exception& error)
  {
    throw std::runtime_error(std::string("Unexpected /info payload from the Freedom Names node: ") + error.what());
  }
  return info;
}

/**
 * \brief Get the node's liveness + version handshake from GET /health.
 * \throw std::runtime_error on transport/HTTP/parse error
 */
FreedomHealth FreedomNames::get_health()
{
  std::string body = http_get(base_url() + "/health");
  auto json = parse_response(body, "/health");

  FreedomHealth health;
  try
  {
    health.version = json.value("version", "");
    health.ready = json.value("ready", false);
    health.role = json.value("role", ""); // empty on nodes older than 0.8.4
    health.capabilities = string_array(json, "capabilities");
    const std::string authoring_api = json.value("authoringAPI", "");
    if (health.has_capability("authoring") && is_loopback_http_origin(authoring_api))
      health.authoring_api = authoring_api;
  }
  catch (const nlohmann::json::exception& error)
  {
    // A key present with an unexpected type, eg. {"ready": "yes"}.
    throw std::runtime_error(std::string("Unexpected /health payload from the Freedom Names node: ") + error.what());
  }
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
 * \brief Drop the node's resolver cache (DELETE /clear_cache).
 *
 * Every subsequent lookup goes back to the DHT rather than answering from a
 * cached record, which is what you want after a name's owner republishes it and
 * the old record has not expired yet.
 *
 * \throw std::runtime_error on transport/HTTP error
 */
void FreedomNames::clear_cache()
{
  // The handler answers with an empty 200 body; there is nothing to parse.
  http_delete(base_url() + "/clear_cache");
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
 * \brief Perform a POST request carrying a JSON object.
 */
std::string FreedomNames::http_post_json(const std::string& url, const std::string& body)
{
  CURL* curl = curl_easy_init();
  if (!curl)
    throw std::runtime_error("Could not init curl");
  curl_slist* headers = curl_slist_append(nullptr, "Content-Type: application/json");
  if (!headers)
  {
    curl_easy_cleanup(curl);
    throw std::runtime_error("Could not allocate HTTP request headers");
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
  try
  {
    const std::string response = perform(curl, url);
    curl_slist_free_all(headers);
    return response;
  }
  catch (...)
  {
    curl_slist_free_all(headers);
    throw;
  }
}

/**
 * \brief Perform a DELETE request and return the response body.
 */
std::string FreedomNames::http_delete(const std::string& url)
{
  CURL* curl = curl_easy_init();
  if (!curl)
    throw std::runtime_error("Could not init curl");
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
  return perform(curl, url);
}

/**
 * \brief Minimal percent-encoding for query parameter values.
 */
std::string FreedomNames::url_encode(const std::string& value)
{
  std::ostringstream escaped;
  static_cast<void>(escaped.fill('0'));
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
 * \brief Accept only a bare HTTP origin on numeric 127/8 or ::1 loopback.
 */
bool FreedomNames::is_loopback_http_origin(const std::string& url)
{
  static const std::string scheme = "http://";
  if (url.rfind(scheme, 0) != 0)
    return false;
  const std::string authority = url.substr(scheme.size());
  if (authority.empty() || authority.find_first_of("/?#@") != std::string::npos)
    return false;

  std::string host;
  std::string port;
  if (authority.front() == '[')
  {
    const std::size_t close = authority.find(']');
    if (close == std::string::npos || close + 1 >= authority.size() || authority.at(close + 1) != ':')
      return false;
    host = authority.substr(1, close - 1);
    port = authority.substr(close + 2);
    if (host != "::1")
      return false;
  }
  else
  {
    const std::size_t colon = authority.rfind(':');
    if (colon == std::string::npos)
      return false;
    host = authority.substr(0, colon);
    port = authority.substr(colon + 1);
    if (!valid_ipv4_loopback(host))
      return false;
  }
  return valid_port(port);
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
