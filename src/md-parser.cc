#include "md-parser.h"

#include <string>
#include <stdexcept>
#include <cmark-gfm-core-extensions.h>
#include <node.h>
#include <syntax_extension.h>
#include <filesystem>

static const int OPTIONS = CMARK_OPT_DEFAULT;

/// Meyers Singleton
Parser::Parser() = default;
/// Destructor
Parser::~Parser() = default;

/**
 * \brief Get singleton instance
 * \return Helper reference (singleton)
 */
Parser &Parser::getInstance()
{
    static Parser instance;
    return instance;
}

/**
 * Parse markdown file from string content
 * @return AST structure (of type cmark_node)
 */
cmark_node *Parser::parseContent(const std::string &content)
{
    //cmark_node *doc;
    // Parse to AST with cmark
    // mark_parser *parser = cmark_parser_new(OPTIONS);

    // Add extensions
    //addMarkdownExtension(parser, "strikethrough");
    //addMarkdownExtension(parser, "table");

    const char *data = content.c_str();
    // TODO: Copy cmark_parse_document() to be able to add extensions to the parser
    return cmark_parse_document(data, strlen(data), OPTIONS);
}

/**
 * Built-in cmark parser to HTML
 */
std::string const Parser::renderHTML(cmark_node *node)
{
    char *tmp = cmark_render_html(node, OPTIONS, NULL);
    std::string output = std::string(tmp);
    free(tmp);
    return output;
}

/**
 * This is a function that will make enabling extensions easier
 */
void Parser::addMarkdownExtension(cmark_parser *parser, const char *extName)
{
    cmark_syntax_extension *ext = cmark_find_syntax_extension(extName);
    if (ext)
        cmark_parser_attach_syntax_extension(parser, ext);
}
