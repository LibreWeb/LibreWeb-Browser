#ifndef MD_PARSER_H
#define MD_PARSER_H

#include <cmark-gfm.h>
#include <glibmm/ustring.h>
#include <render.h>
#include <sstream>

/**
 * \class Parser
 * \brief Parser Markdown parser class, parse the content to an AST model
 */
class Parser
{
public:
  // Singleton
  static Parser& get_instance();
  static cmark_node* parse_content(const Glib::ustring& content);
  static Glib::ustring render_html(cmark_node* node);
  static Glib::ustring render_markdown(cmark_node* node);

private:
  Parser();
  ~Parser();
  Parser(const Parser&) = delete;
  Parser& operator=(const Parser&) = delete;

  static void add_markdown_extension(cmark_parser* parser, const char* ext_name);
};
#endif
