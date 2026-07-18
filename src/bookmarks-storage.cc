#include "bookmarks-storage.h"
#include "file.h"

#include <algorithm>
#include <glib.h>
#include <glibmm/miscutils.h>
#include <iostream>
#include <nlohmann/json.hpp>

BookmarksStorage::BookmarksStorage()
{
  std::string config_directory = Glib::build_filename(Glib::get_user_config_dir(), "libreweb");
  g_mkdir_with_parents(config_directory.c_str(), 0755);
  bookmarks_file_path_ = Glib::build_filename(config_directory, "bookmarks.json");
  load();
}

BookmarksStorage::~BookmarksStorage()
{
}

/**
 * \brief Retrieve all stored bookmarks
 * \return List of bookmarks
 */
const std::vector<Bookmark>& BookmarksStorage::get_bookmarks() const
{
  return bookmarks_;
}

/**
 * \brief Add a new bookmark, or rename the existing bookmark when the address is already present
 * \param name User-friendly bookmark name
 * \param address Address the bookmark points to
 * \return True when a new bookmark was added, false when an existing bookmark was updated
 */
bool BookmarksStorage::add(const std::string& name, const std::string& address)
{
  auto iter = std::find_if(bookmarks_.begin(), bookmarks_.end(), [&address](const Bookmark& bookmark) { return bookmark.address == address; });
  if (iter != bookmarks_.end())
  {
    iter->name = name;
    save();
    return false;
  }
  bookmarks_.push_back({name, address});
  save();
  return true;
}

/**
 * \brief Update the bookmark at the given index
 * \param index Bookmark index
 * \param name New user-friendly bookmark name
 * \param address New address
 */
void BookmarksStorage::update(std::size_t index, const std::string& name, const std::string& address)
{
  if (index < bookmarks_.size())
  {
    bookmarks_[index].name = name;
    bookmarks_[index].address = address;
    save();
  }
}

/**
 * \brief Remove the bookmark at the given index
 * \param index Bookmark index
 */
void BookmarksStorage::remove(std::size_t index)
{
  if (index < bookmarks_.size())
  {
    bookmarks_.erase(bookmarks_.begin() + index);
    save();
  }
}

/**
 * \brief Import bookmarks from a JSON file, skipping addresses that are already bookmarked
 * \param path File path of the JSON file to import
 * \throw std::runtime_error when the file can't be read or isn't valid bookmarks JSON
 * \return Number of newly imported bookmarks
 */
std::size_t BookmarksStorage::import_from(const std::string& path)
{
  nlohmann::json json = nlohmann::json::parse(File::read(path));
  std::size_t imported = 0;
  for (const auto& item : json.at("bookmarks"))
  {
    std::string address = item.at("address").get<std::string>();
    std::string name = item.value("name", address);
    bool exists = std::any_of(bookmarks_.cbegin(), bookmarks_.cend(), [&address](const Bookmark& bookmark) { return bookmark.address == address; });
    if (!exists)
    {
      bookmarks_.push_back({name, address});
      ++imported;
    }
  }
  if (imported > 0)
    save();
  return imported;
}

/**
 * \brief Export all bookmarks to a JSON file
 * \param path Destination file path
 * \throw std::ios_base::failure when the file can't be written to
 */
void BookmarksStorage::export_to(const std::string& path) const
{
  File::write(path, serialize());
}

/**
 * \brief Load the bookmarks from disk, silently starting empty when the file is missing or corrupt
 */
void BookmarksStorage::load()
{
  bookmarks_.clear();
  try
  {
    nlohmann::json json = nlohmann::json::parse(File::read(bookmarks_file_path_));
    for (const auto& item : json.at("bookmarks"))
    {
      std::string address = item.at("address").get<std::string>();
      bookmarks_.push_back({item.value("name", address), address});
    }
  }
  catch (const std::exception& error)
  {
    // Missing file is the normal first-run case; a corrupt file shouldn't prevent start-up
  }
}

/**
 * \brief Write the bookmarks to disk
 */
void BookmarksStorage::save() const
{
  try
  {
    File::write(bookmarks_file_path_, serialize());
  }
  catch (const std::exception& error)
  {
    std::cerr << "ERROR: Could not save bookmarks to: " << bookmarks_file_path_ << std::endl;
  }
}

/**
 * \brief Serialize the bookmarks to a JSON string
 * \return JSON string
 */
std::string BookmarksStorage::serialize() const
{
  nlohmann::json json;
  json["bookmarks"] = nlohmann::json::array();
  for (const Bookmark& bookmark : bookmarks_)
  {
    json["bookmarks"].push_back({{"name", bookmark.name}, {"address", bookmark.address}});
  }
  return json.dump(2);
}
