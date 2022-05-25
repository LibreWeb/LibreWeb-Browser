#include "middleware-i.h"
#include "gmock/gmock.h"
#include <string>

struct cmark_node;

class MockMiddleware : public MiddlewareInterface
{
public:
  MOCK_METHOD(void,
              do_request,
              (const std::string& path, bool is_set_address_bar, bool is_history_request, bool is_disable_editor, bool is_parse_content),
              (override));
  MOCK_METHOD(std::string, do_add, (const std::string& path), (override));
  MOCK_METHOD(void, do_write, (const std::string& path, bool is_set_address_and_title), (override));
  MOCK_METHOD(void, set_content, (const Glib::ustring& content), (override));
  MOCK_METHOD(Glib::ustring, get_content, (), (const, override));
  MOCK_METHOD(cmark_node*, parse_content, (), (const, override));
  MOCK_METHOD(void, reset_content_and_path, (), (override));
  MOCK_METHOD(std::size_t, get_ipfs_number_of_peers, (), (const, override));
  MOCK_METHOD(int, get_ipfs_repo_size, (), (const, override));
  MOCK_METHOD(std::string, get_ipfs_repo_path, (), (const, override));
  MOCK_METHOD(std::string, get_ipfs_incoming_rate, (), (const, override));
  MOCK_METHOD(std::string, get_ipfs_outgoing_rate, (), (const, override));
  MOCK_METHOD(std::string, get_ipfs_version, (), (const, override));
  MOCK_METHOD(std::string, get_ipfs_client_id, (), (const, override));
  MOCK_METHOD(std::string, get_ipfs_client_public_key, (), (const, override));
};
