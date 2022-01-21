#ifndef MIDDLEWARE_H
#define MIDDLEWARE_H

#include "ipfs.h"
#include <atomic>
#include <glibmm/dispatcher.h>
#include <glibmm/ustring.h>
#include <map>
#include <mutex>
#include <sigc++/connection.h>
#include <string>
#include <thread>
#include <variant>

/* Forward declarations */
struct cmark_node;
class MainWindow;

/**
 * \class Middleware
 * \brief Handles (IPFS) network requests and File IO from disk towards the GUI
 */
class Middleware
{
public:
  explicit Middleware(MainWindow& mainWindow, const std::string& timeout);
  virtual ~Middleware();
  void doRequest(const std::string& path = std::string(),
                 bool isSetAddressBar = true,
                 bool isHistoryRequest = false,
                 bool isDisableEditor = true,
                 bool isParseContent = true);
  std::string doAdd(const std::string& path);
  void doWrite(const std::string& path, bool isSetAddressAndTitle = true);
  void setContent(const Glib::ustring& content);
  Glib::ustring getContent() const;
  cmark_node* parseContent() const;
  void resetContentAndPath();
  std::size_t getIPFSNumberOfPeers() const;
  int getIPFSRepoSize() const;
  std::string getIPFSRepoPath() const;
  std::string getIPFSIncomingRate() const;
  std::string getIPFSOutcomingRate() const;
  std::string getIPFSVersion() const;
  std::string getIPFSClientId() const;
  std::string getIPFSClientPublicKey() const;

private:
  MainWindow& mainWindow;
  Glib::Dispatcher requestStarted_;
  Glib::Dispatcher requestFinished_;
  sigc::connection statusTimerHandler_;
  // Threading:
  std::thread* requestThread_;                    /* Request thread pointer */
  std::thread* statusThread_;                     /* Status thread pointer */
  std::atomic<bool> is_request_thread_done_;      /* Indication when the single request (fetch) is done */
  std::atomic<bool> keep_request_thread_running_; /* Trigger the request thread to stop/continue */
  std::atomic<bool> is_status_thread_done_;       /* Indication when the status calls are done */

  // IPFS:
  std::string ipfsHost_;    /* IPFS host name */
  int ipfsPort_;            /* IPFS port number */
  std::string ipfsTimeout_; /* IPFS time-out setting */
  IPFS ipfs_fetch_;         /* IPFS object for fetch calls */
  IPFS ipfs_status_;        /* IPFS object for status calls, so it doesn't conflict with the fetch request */
  std::size_t ipfsNumberOfPeers_;
  int ipfsRepoSize_;
  std::string ipfsRepoPath_;
  std::string ipfsIncomingRate_;
  std::string ipfsOutcomingRate_;
  std::string ipfsVersion_;
  std::string ipfsClientID_;
  std::string ipfsClientPublicKey_;
  std::mutex status_mutex_; /* IPFS status mutex to protect class members */

  // Request & Response:
  std::string requestPath_;
  std::string finalRequestPath_;
  Glib::ustring currentContent_;
  bool waitPageVisible_;

  void processRequest(const std::string& path, bool isParseContent);
  void fetchFromIPFS(bool isParseContent);
  void openFromDisk(bool isParseContent);
  bool validateUTF8(const Glib::ustring& text) const;
  void doIPFSStatusUpdateOnce();
  bool doIPFSStatusUpdate();
  void processIPFSStatus();
  void abortRequest();
  void abortStatus();
};

#endif