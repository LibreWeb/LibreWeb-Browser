#include "md-parser.h"

#include <cmark-gfm-core-extensions.h>
#include <filesystem>
#include <node.h>
#include <stdexcept>
#include <syntax_extension.h>

static const int Options = CMARK_OPT_STRIKETHROUGH_DOUBLE_TILDE;

/// Meyers Singleton
Parser::Parser() = default;
/// Destructor
Parser::~Parser() = default;

/**
 * \brief Get singleton instance
 * \return Helper reference (singleton)
 */
Parser& Parser::get_instance()
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
cmark_node* Parser::parse_content(const Glib::ustring& content)
{
  const char* data = content.c_str();

  cmark_gfm_core_extensions_ensure_registered();

  // Modified version of cmark_parse_document() in blocks.c
  cmark_parser* parser = cmark_parser_new(Options);
  cmark_node* document;
  // Add extensions
  add_markdown_extension(parser, "strikethrough");
  add_markdown_extension(parser, "highlight");
  add_markdown_extension(parser, "superscript");
  add_markdown_extension(parser, "subscript");
  // add_markdown_extension(parser, "table");

  cmark_parser_feed(parser, data, strlen(data));
  document = cmark_parser_finish(parser);
  cmark_parser_free(parser);
  return document;
}

/**
 * \brief Built-in cmark parser to HTML
 * \return HTML as string
 */
Glib::ustring Parser::render_html(cmark_node* node)
{
  char* tmp = cmark_render_html(node, Options, NULL);
  Glib::ustring output = Glib::ustring(tmp);
  free(tmp);
  return output;
}

/**
 * \brief Built-in cmark parser to markdown (again)
 * \return return markdown as string
 */
Glib::ustring Parser::render_markdown(cmark_node* node)
{
  char* tmp = cmark_render_commonmark(node, Options, 600);
  Glib::ustring output = Glib::ustring(tmp);
  free(tmp);
  return output;
}

/**
 * This is a function that will make enabling extensions easier
 */
void Parser::add_markdown_extension(cmark_parser* parser, const char* ext_name)
{
  cmark_syntax_extension* ext = cmark_find_syntax_extension(ext_name);
  if (ext)
    cmark_parser_attach_syntax_extension(parser, ext);
}
