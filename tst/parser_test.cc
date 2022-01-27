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
    uint16_t expectedDocument = CMARK_NODE_DOCUMENT;

    // When
    cmark_node* doc = Parser::parseContent(markdown);

    // Then
    ASSERT_EQ(doc->type, expectedDocument);
    cmark_node_free(doc);
  }

  TEST(LibreWebTest, TestHTMLRender)
  {
    // Given
    std::string markdown = "_Italic_ **BOLD** ~~strike~~";
    std::string expectedHtml = "<p><em>Italic</em> <strong>BOLD</strong> <del>strike</del></p>\n";

    // When
    cmark_node* doc = Parser::parseContent(markdown);
    std::string html = Parser::renderHTML(doc);
    cmark_node_free(doc);

    // Then
    ASSERT_EQ(html, expectedHtml);
  }

  TEST(LibreWebTest, TestMarkdownRender)
  {
    // Given
    std::string markdown = "**HOLA**";

    // When
    cmark_node* doc = Parser::parseContent(markdown);
    std::string markdownAgain = Parser::renderMarkdown(doc);
    cmark_node_free(doc);

    // Then
    ASSERT_EQ(markdownAgain, markdown + "\n");
  }
} // namespace