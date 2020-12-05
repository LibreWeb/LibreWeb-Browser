#include "md-parser.h"

#include <string>
#include <iostream>
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

cmark_node * Parser::parseStream(const std::stringstream &stream)
{
    // Parse to AST with cmark
    cmark_parser *parser = cmark_parser_new(options);

    // Add extensions
    //addMarkdownExtension(parser, "strikethrough");
    //addMarkdownExtension(parser, "table");

    //const char buffer[4096];
    size_t bytes;
    cmark_node *document;


    // Print to console
    std::cout << stream.str() << std::endl;
    

    // stream.read() ..? 
    /*
    while ((bytes = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        bool eof = bytes < sizeof(buffer);
        
        cmark_parser_feed(parser, buffer, bytes);
        if (eof) {
            break;
        }
    }*/
    document = cmark_parser_finish(parser);
    cmark_parser_free(parser);

    return document;
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
