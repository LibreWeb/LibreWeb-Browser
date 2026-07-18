#include "middleware-i.h"
#include "gmock/gmock.h"
#include <string>

struct cmark_node;

class MockMiddleware : public MiddlewareInterface
{
public:
  // Alias, since the comma inside the std::function type confuses the MOCK_METHOD macro
  using ImageCallback = std::function<void(const std::string& data)>;

  MOCK_METHOD(void,
              do_request,
              (const std::string& path, bool is_set_address_bar, bool is_history_request, bool is_disable_editor, bool is_parse_content),
              (override));
  MOCK_METHOD(std::string, do_add, (const std::string& path), (override));
  MOCK_METHOD(std::string, do_add_file, (const std::string& path), (override));
  MOCK_METHOD(void, fetch_image, (const std::string& path, const ImageCallback& callback), (override));
  MOCK_METHOD(void, do_write, (const std::string& path, bool is_set_address_and_title), (override));
  MOCK_METHOD(void, set_content, (const Glib::ustring& content), (override));
  MOCK_METHOD(Glib::ustring, get_content, (), (const, override));
  MOCK_METHOD(cmark_node*, parse_content, (), (const, override));
  MOCK_METHOD(void, reset_content_and_path, (), (override));
};
