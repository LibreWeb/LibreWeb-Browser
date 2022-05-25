#ifndef MIDDLEWARE_INTERFACE_H
#define MIDDLEWARE_INTERFACE_H

#include <glibmm/ustring.h>
#include <string>

/* Forward declarations */
struct cmark_node;

/**
 * \class MiddlewareInterface
 * \brief Pure Middleware interface
 */
class MiddlewareInterface
{
public:
  virtual ~MiddlewareInterface()
  {
  }
  virtual void do_request(const std::string& path = std::string(),
                          bool isSetAddressBar = true,
                          bool isHistoryRequest = false,
                          bool isDisableEditor = true,
                          bool isParseContent = true) = 0;
  virtual std::string do_add(const std::string& path) = 0;
  virtual void do_write(const std::string& path, bool isSetAddressAndTitle = true) = 0;
  virtual void set_content(const Glib::ustring& content) = 0;
  virtual Glib::ustring get_content() const = 0;
  virtual cmark_node* parse_content() const = 0;
  virtual void reset_content_and_path() = 0;
  virtual std::size_t get_ipfs_number_of_peers() const = 0;
  virtual int get_ipfs_repo_size() const = 0;
  virtual std::string get_ipfs_repo_path() const = 0;
  virtual std::string get_ipfs_incoming_rate() const = 0;
  virtual std::string get_ipfs_outgoing_rate() const = 0;
  virtual std::string get_ipfs_version() const = 0;
  virtual std::string get_ipfs_client_id() const = 0;
  virtual std::string get_ipfs_client_public_key() const = 0;
};

#endif