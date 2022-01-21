#include "md-parser.h"

#include <cmark-gfm-core-extensions.h>
#include <filesystem>
#include <node.h>
#include <stdexcept>
#include <syntax_extension.h>

static const int OPTIONS = CMARK_OPT_STRIKETHROUGH_DOUBLE_TILDE;

/// Meyers Singleton
Parser::Parser() = default;
/// Destructor
Parser::~Parser() = default;

/**
 * \brief Get singleton instance
 * \return Helper reference (singleton)
 */
Parser& Parser::getInstance()
{
  static Parser instance;
  return instance;
}

/**
 * \brief Parse markdown file from string content.
 * Note: Do not forgot to execute: cmark_node_free(document); when you are done with the doc.
 * \param content Content as string
 * \return AST structure (of type cmark_node)
 */
cmark_node* Parser::parseContent(const Glib::ustring& content)
{
  const char* data = content.c_str();

  cmark_gfm_core_extensions_ensure_registered();

  // Modified version of cmark_parse_document() in blocks.c
  cmark_parser* parser = cmark_parser_new(OPTIONS);
  cmark_node* document;
  // Add extensions
  addMarkdownExtension(parser, "strikethrough");
  addMarkdownExtension(parser, "highlight");
  addMarkdownExtension(parser, "superscript");
  addMarkdownExtension(parser, "subscript");
  // addMarkdownExtension(parser, "table");

  cmark_parser_feed(parser, data, strlen(data));
  document = cmark_parser_finish(parser);
  cmark_parser_free(parser);
  return document;
}

/**
 * \brief Built-in cmark parser to HTML
 * \return HTML as string
 */
Glib::ustring Parser::renderHTML(cmark_node* node)
{
  char* tmp = cmark_render_html(node, OPTIONS, NULL);
  Glib::ustring output = Glib::ustring(tmp);
  free(tmp);
  return output;
}

/**
 * \brief Built-in cmark parser to markdown (again)
 * \return return markdown as string
 */
Glib::ustring Parser::renderMarkdown(cmark_node* node)
{
  char* tmp = cmark_render_commonmark(node, OPTIONS, 600);
  Glib::ustring output = Glib::ustring(tmp);
  free(tmp);
  return output;
}

/**
 * This is a function that will make enabling extensions easier
 */
void Parser::addMarkdownExtension(cmark_parser* parser, const char* extName)
{
  cmark_syntax_extension* ext = cmark_find_syntax_extension(extName);
  if (ext)
    cmark_parser_attach_syntax_extension(parser, ext);
}
