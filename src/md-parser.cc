#include "md-parser.h"

#include <string>
#include <cmark-gfm-core-extensions.h>

#include <node.h>
#include <syntax_extension.h>

Parser::Parser(): options(CMARK_OPT_DEFAULT) {}

/**
 * Parse markdown by file path
 * @return AST structure (of type cmark_node)
 */
cmark_node * Parser::parseFile(const std::string &filePath)
{
    // Modified version of cmark_parse_document in blocks.c
    cmark_parser *parser = cmark_parser_new(options);

    // Add extensions
    addMarkdownExtension(parser, "strikethrough");
    addMarkdownExtension(parser, "table");
  
    cmark_parser_free(parser);

    // Parse to AST with cmark
    FILE *file;
    if( ( file = fopen(filePath.c_str(), "r" ) ) != NULL ) 
    {
        cmark_node *root_node;

        // TODO: Copy/paste cmark_parse_file() content to here, allowing me to add extensions to the parser.
        root_node = cmark_parse_file(file, options);
        fclose(file);

        return root_node;
    }
    return NULL;    
}

std::string const Parser::renderHTML(cmark_node *node)
{
    char *tmp = cmark_render_html(node, options, NULL);
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
