#include "freedomnames-cli.h"

#include "freedomnames-daemon.h"
#include <algorithm>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <glibmm/spawn.h>
#include <stdexcept>

#ifdef LEGACY_CXX
#include <experimental/filesystem>
namespace n_fs = ::std::experimental::filesystem;
#else
#include <filesystem>
namespace n_fs = ::std::filesystem;
#endif

namespace
{
  // Suffix of an owner key file in the keys directory. Staged records live
  // beside them as "<label>.records.json", which must not be read as a label.
  const std::string kKeySuffix = ".key";
} // namespace

/**
 * \brief Whether the bundled binary can be found.
 */
bool FreedomNamesCli::available()
{
  return !FreedomNamesDaemon::locate_binary().empty();
}

/**
 * \brief Directory holding the owner keys, matching the node's own layout.
 */
std::string FreedomNamesCli::keys_dir()
{
  return Glib::build_filename(Glib::get_home_dir(), ".freedom", "keys");
}

/**
 * \brief First line of a child's output, without its trailing newline.
 *
 * The CLI prints a short human-readable report; the line the browser wants is
 * always the first one, and the rest (byte counts, expiry) is for a terminal.
 */
std::string FreedomNamesCli::first_line(const std::string& text)
{
  const std::string::size_type end = text.find('\n');
  std::string line = (end == std::string::npos) ? text : text.substr(0, end);
  while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
    line.pop_back();
  return line;
}

/**
 * \brief List the owner keys the user already has.
 *
 * The full name cannot be derived from the file name -- it embeds a hash of the
 * public key -- so each label costs one `freedom name` call. That is a process
 * spawn per key, which is why this is called when the publish dialog is built
 * rather than on every repaint, and why a key whose name cannot be read is kept
 * in the list with an empty full_name instead of failing the whole listing.
 */
std::vector<FreedomKey> FreedomNamesCli::list_keys()
{
  std::vector<FreedomKey> keys;
  const std::string dir = keys_dir();
  if (!Glib::file_test(dir, Glib::FileTest::FILE_TEST_IS_DIR))
    return keys; // no names yet; not an error

  try
  {
    for (const auto& entry : n_fs::directory_iterator(dir))
    {
      const std::string file_name = entry.path().filename().string();
      if (file_name.size() <= kKeySuffix.size() || file_name.compare(file_name.size() - kKeySuffix.size(), kKeySuffix.size(), kKeySuffix) != 0)
        continue;
      FreedomKey key;
      key.label = file_name.substr(0, file_name.size() - kKeySuffix.size());
      keys.push_back(key);
    }
  }
  catch (const std::exception&)
  {
    return keys; // unreadable directory: report what we have, which may be none
  }

  std::sort(keys.begin(), keys.end(), [](const FreedomKey& a, const FreedomKey& b) { return a.label < b.label; });
  for (FreedomKey& key : keys)
  {
    try
    {
      key.full_name = first_line(run({"name", key.label}));
    }
    catch (const std::runtime_error&)
    {
      // Leave full_name empty; the dialog shows the label on its own.
    }
  }
  return keys;
}

/**
 * \brief Generate a new owner keypair.
 */
std::string FreedomNamesCli::keygen(const std::string& label)
{
  // "Generated key for "x"\nYour name: x.<pubKeyID>.fn" -- the name is what the
  // caller needs, and it is on the second line here rather than the first.
  const std::string output = run({"keygen", label});
  const std::string marker = "Your name:";
  const std::string::size_type at = output.find(marker);
  if (at == std::string::npos)
    throw std::runtime_error("Could not read the new name from the Freedom Names CLI output: " + output);
  std::string name = first_line(output.substr(at + marker.size()));
  while (!name.empty() && name.front() == ' ')
    name.erase(0, 1);
  if (name.empty())
    throw std::runtime_error("The Freedom Names CLI reported an empty name for \"" + label + "\".");
  return name;
}

/**
 * \brief Upload a file and point a name at it, in one CLI call.
 */
std::string FreedomNamesCli::put(const std::string& label, const std::string& file_path)
{
  // "Uploaded <file> (N bytes) -> <hash>\nPublished <name> (seq ..., N record(s))"
  const std::string output = run({"put", label, file_path});
  const std::string marker = "Published ";
  const std::string::size_type at = output.find(marker);
  if (at == std::string::npos)
    throw std::runtime_error("Could not read the published name from the Freedom Names CLI output: " + output);
  std::string rest = first_line(output.substr(at + marker.size()));
  // Drop the trailing " (seq ..., N record(s))" summary.
  const std::string::size_type space = rest.find(' ');
  if (space != std::string::npos)
    rest = rest.substr(0, space);
  if (rest.empty())
    throw std::runtime_error("The Freedom Names CLI reported an empty name for \"" + label + "\".");
  return rest;
}

/**
 * \brief Run `<binary> freedom <args...>` and return its standard output.
 *
 * Uses spawn_sync rather than the shell: a label or path is user input, and
 * passing an argument vector means quoting can never be got wrong.
 */
std::string FreedomNamesCli::run(const std::vector<std::string>& args)
{
  const std::string binary = FreedomNamesDaemon::locate_binary();
  if (binary.empty())
    throw std::runtime_error("The Freedom Names binary could not be found, so names cannot be managed.");

  std::vector<std::string> argv;
  argv.push_back(binary);
  argv.push_back("freedom");
  argv.insert(argv.end(), args.begin(), args.end());

  std::string out;
  std::string err;
  int exit_status = 0;
  try
  {
    Glib::spawn_sync(Glib::get_home_dir(), argv, Glib::SPAWN_DEFAULT, Glib::SlotSpawnChildSetup(), &out, &err, &exit_status);
  }
  catch (const Glib::SpawnError& error)
  {
    throw std::runtime_error(std::string("Could not run the Freedom Names CLI: ") + error.what());
  }

  if (exit_status != 0)
  {
    // The CLI reports failures as "error: ..." on stderr; pass that through
    // rather than an exit code, since it is already written for a human.
    std::string reason = first_line(err.empty() ? out : err);
    if (reason.empty())
      reason = "the command failed without a message";
    throw std::runtime_error(reason);
  }
  return out;
}
