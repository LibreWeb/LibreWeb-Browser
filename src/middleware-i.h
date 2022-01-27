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
  virtual void doRequest(const std::string& path = std::string(),
                         bool isSetAddressBar = true,
                         bool isHistoryRequest = false,
                         bool isDisableEditor = true,
                         bool isParseContent = true) = 0;
  virtual std::string doAdd(const std::string& path) = 0;
  virtual void doWrite(const std::string& path, bool isSetAddressAndTitle = true) = 0;
  virtual void setContent(const Glib::ustring& content) = 0;
  virtual Glib::ustring getContent() const = 0;
  virtual cmark_node* parseContent() const = 0;
  virtual void resetContentAndPath() = 0;
  virtual std::size_t getIPFSNumberOfPeers() const = 0;
  virtual int getIPFSRepoSize() const = 0;
  virtual std::string getIPFSRepoPath() const = 0;
  virtual std::string getIPFSIncomingRate() const = 0;
  virtual std::string getIPFSOutcomingRate() const = 0;
  virtual std::string getIPFSVersion() const = 0;
  virtual std::string getIPFSClientId() const = 0;
  virtual std::string getIPFSClientPublicKey() const = 0;
};

#endif