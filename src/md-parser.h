#ifndef MD_PARSER_H
#define MD_PARSER_H

#include <string>
#include <cmark-gfm.h>
#include <render.h>

/**
 * \class Parser Markdown parser class, parse the content to an AST model
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
