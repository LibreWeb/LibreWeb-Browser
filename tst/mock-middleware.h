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
  MOCK_METHOD(std::size_t, get_freedom_number_of_peers, (), (const, override));
  MOCK_METHOD(std::string, get_freedom_node_id, (), (const, override));
  MOCK_METHOD(std::string, get_freedom_mode, (), (const, override));
  MOCK_METHOD(int, get_freedom_network_size, (), (const, override));
  MOCK_METHOD(std::string, get_freedom_version, (), (const, override));
};
