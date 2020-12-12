#include "md-parser.h"

#include <string>
#include <stdexcept>
#include <cmark-gfm-core-extensions.h>
#include <node.h>
#include <syntax_extension.h>
#include <filesystem>

Parser::Parser(): options(CMARK_OPT_DEFAULT) {}

/**
 * Parse markdown by file path
 * @return AST structure (of type cmark_node)
 */
cmark_node * Parser::parseFile(const std::string &filePath)
{
    // Parse to AST with cmark
    FILE *file;
    if (!std::filesystem::exists(filePath.c_str())) {
        throw std::runtime_error("File not found.");
    }

    if( ( file = fopen(filePath.c_str(), "r" ) ) != NULL ) 
    {
        cmark_node *doc;
        // TODO: Copy/paste cmark_parse_file() content to here, allowing me to add extensions to the parser.
        doc = cmark_parse_file(file, options);
        fclose(file);
        return doc;
    } else {
        throw std::runtime_error("File open failed.");
    }
}

/**
 * Parse markdown file by stringstream
 * @return AST structure (of type cmark_node)
 */
cmark_node * Parser::parseStream(const std::stringstream &stream)
{
    //cmark_node *doc;
    // Parse to AST with cmark
    // mark_parser *parser = cmark_parser_new(options);

    // Add extensions
    //addMarkdownExtension(parser, "strikethrough");
    //addMarkdownExtension(parser, "table");

    const std::string tmp = stream.str();
    const char *data = tmp.c_str();    
    // TODO: Copy cmark_parse_document() to be able to add extensions to the parser
    return cmark_parse_document(data, strlen(data), options);
}

/**
 * Built-in cmark parser to HTML
 */
std::string const Parser::renderHTML(cmark_node *node)
{
    char *tmp = cmark_render_html(node, options, NULL);
    std::string output = std::string(tmp);
    free(tmp);
    return output;
}

/**
 * Get just the plain text
 */
std::string const Parser::getSource(cmark_node *node)
{
    char *tmp = cmark_render_commonmark(node, options, 0);
    std::string output = std::string(tmp);
    free(tmp);
    return output;
}

/**
 * This is a function that will make enabling extensions easier
 */
void Parser::addMarkdownExtension(cmark_parser *parser, const char *extName) {
  cmark_syntax_extension *ext = cmark_find_syntax_extension(extName);
  if ( ext )
    cmark_parser_attach_syntax_extension(parser, ext);
}
