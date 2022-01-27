#include "middleware-i.h"
#include "gmock/gmock.h"
#include <string>

struct cmark_node;

class MockMiddleware : public MiddlewareInterface
{
public:
  MOCK_METHOD(void,
              doRequest,
              (const std::string& path, bool isSetAddressBar, bool isHistoryRequest, bool isDisableEditor, bool isParseContent),
              (override));
  MOCK_METHOD(std::string, doAdd, (const std::string& path), (override));
  MOCK_METHOD(void, doWrite, (const std::string& path, bool isSetAddressAndTitle), (override));
  MOCK_METHOD(void, setContent, (const Glib::ustring& content), (override));
  MOCK_METHOD(Glib::ustring, getContent, (), (const, override));
  MOCK_METHOD(cmark_node*, parseContent, (), (const, override));
  MOCK_METHOD(void, resetContentAndPath, (), (override));
  MOCK_METHOD(std::size_t, getIPFSNumberOfPeers, (), (const, override));
  MOCK_METHOD(int, getIPFSRepoSize, (), (const, override));
  MOCK_METHOD(std::string, getIPFSRepoPath, (), (const, override));
  MOCK_METHOD(std::string, getIPFSIncomingRate, (), (const, override));
  MOCK_METHOD(std::string, getIPFSOutcomingRate, (), (const, override));
  MOCK_METHOD(std::string, getIPFSVersion, (), (const, override));
  MOCK_METHOD(std::string, getIPFSClientId, (), (const, override));
  MOCK_METHOD(std::string, getIPFSClientPublicKey, (), (const, override));
};
