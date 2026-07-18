#ifndef BOOKMARKS_STORAGE_H
#define BOOKMARKS_STORAGE_H

#include <cstddef>
#include <string>
#include <vector>

/**
 * \struct Bookmark
 * \brief Single bookmark entry, an user-friendly name together with the address
 */
struct Bookmark
{
  std::string name;
  std::string address;
};

/**
 * \class BookmarksStorage
 * \brief Manage bookmarks, persisted as a JSON file inside the user config directory
 */
class BookmarksStorage
{
public:
  BookmarksStorage();
  virtual ~BookmarksStorage();

  const std::vector<Bookmark>& get_bookmarks() const;
  bool add(const std::string& name, const std::string& address);
  void update(std::size_t index, const std::string& name, const std::string& address);
  void remove(std::size_t index);
  std::size_t import_from(const std::string& path);
  void export_to(const std::string& path) const;

private:
  std::vector<Bookmark> bookmarks_;
  std::string bookmarks_file_path_;

  void load();
  void save() const;
  std::string serialize() const;
};
#endif
