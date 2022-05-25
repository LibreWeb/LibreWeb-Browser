#include "md-parser.h"
#include "gtest/gtest.h"
#include <cmark-gfm.h>
#include <node.h>
#include <string>
namespace
{
  TEST(LibreWebTest, TestContentParser)
  {
    // Given
    std::string markdown = "Jaja";
    uint16_t expected_document = CMARK_NODE_DOCUMENT;

    // When
    cmark_node* doc = Parser::parse_content(markdown);

    // Then
    ASSERT_EQ(doc->type, expected_document);
    cmark_node_free(doc);
  }

  TEST(LibreWebTest, TestHTMLRender)
  {
    // Given
    std::string markdown = "_Italic_ **BOLD** ~~strike~~";
    std::string expected_html = "<p><em>Italic</em> <strong>BOLD</strong> <del>strike</del></p>\n";

    // When
    cmark_node* doc = Parser::parse_content(markdown);
    std::string html = Parser::render_html(doc);
    cmark_node_free(doc);

    // Then
    ASSERT_EQ(html, expected_html);
  }

  TEST(LibreWebTest, TestMarkdownRender)
  {
    // Given
    std::string markdown = "**HOLA**";

    // When
    cmark_node* doc = Parser::parse_content(markdown);
    std::string markdown_again = Parser::render_markdown(doc);
    cmark_node_free(doc);

    // Then
    ASSERT_EQ(markdown_again, markdown + "\n");
  }
} // namespace