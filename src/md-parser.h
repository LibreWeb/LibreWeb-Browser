#ifndef MD_PARSER_H
#define MD_PARSER_H

#include <string>
#include <cmark-gfm.h>
#include <render.h>

/**
 * Parser class will parse the content to a AST model
 */
class Parser
{
public:
    Parser();
    cmark_node * parseFile(const std::string &filePath);
    std::string const renderHTML(cmark_node *node);

private:
    int options;

    void addMarkdownExtension(cmark_parser *parser, const char *extName);
};
#endif
