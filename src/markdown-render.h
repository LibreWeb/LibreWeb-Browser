#ifndef MARKDOWN_RENDER_H
#define MARKDOWN_RENDER_H

#include <cmark-gfm.h>

class MarkdownRender
{
public:
    MarkdownRender();

private:
    void addMarkdownExtension(cmark_parser *parser, char *extName);
    char * to_html(const char *markdown_string);
};
#endif // MARKDOWN_RENDER_H
