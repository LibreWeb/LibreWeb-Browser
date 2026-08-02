#ifndef FREEDOMNAMES_CLI_H
#define FREEDOMNAMES_CLI_H

#include <string>
#include <vector>

/**
 * \struct FreedomKey
 * \brief One owner keypair found under ~/.freedom/keys.
 *
 * A Freedom name is its key: whoever holds the private key owns the name, and
 * the name itself is derived from the public half. So a key file *is* the unit
 * the publish dialog offers the user.
 */
struct FreedomKey
{
  std::string label;     /* file stem, eg. "blog" */
  std::string full_name; /* "blog.<pubKeyID>.fn", empty when it could not be read */
};

/**
 * \class FreedomNamesCli
 * \brief Drives the `freedom` sub-commands of the bundled freedom-names binary.
 *
 * The node's HTTP API (see FreedomNames) can store content and accept an
 * already-signed record, but it deliberately cannot sign one: that needs the
 * owner's private key, and record building plus Ed25519 signing lives in the
 * node's own `record` package. Rather than reimplement that -- and have a second
 * implementation of a signature format to keep in step forever -- this shells
 * out to the very binary the browser already ships and spawns.
 *
 * Every call blocks until the child exits. `put` in particular publishes into
 * the DHT, which the node bounds at 60 seconds, so callers must run it off the
 * GUI thread.
 */
class FreedomNamesCli
{
public:
  // Owner keys in ~/.freedom/keys, sorted by label. Never throws: an unreadable
  // keys directory simply means the user has no names yet.
  static std::vector<FreedomKey> list_keys();

  // `freedom keygen <label>`; returns the new full "label.<pubKeyID>.fn" name.
  // \throw std::runtime_error when the label is invalid or already taken
  static std::string keygen(const std::string& label);

  // `freedom put <label> <file>`: upload the file's bytes, point the name's
  // CONTENT record at them, sign and publish. Returns the published full name.
  // \throw std::runtime_error when the node is unreachable or publishing fails
  static std::string put(const std::string& label, const std::string& file_path);

  // Whether the bundled binary can be found at all. When false the publish
  // dialog can still offer the content-hash path, which only needs the HTTP API.
  static bool available();

private:
  // Runs the binary with `freedom <args...>`, returning its stdout.
  // \throw std::runtime_error carrying the child's stderr on a non-zero exit
  static std::string run(const std::vector<std::string>& args);
  static std::string keys_dir();
  static std::string first_line(const std::string& text);
};
#endif
