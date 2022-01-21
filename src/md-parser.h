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
  static Parser& getInstance();
  static cmark_node* parseContent(const Glib::ustring& content);
  static Glib::ustring renderHTML(cmark_node* node);
  static Glib::ustring renderMarkdown(cmark_node* node);

private:
  Parser();
  ~Parser();
  Parser(const Parser&) = delete;
  Parser& operator=(const Parser&) = delete;

  static void addMarkdownExtension(cmark_parser* parser, const char* extName);
};
#endif
