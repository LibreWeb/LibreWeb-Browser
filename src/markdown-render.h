#ifndef MARKDOWN_RENDER_H
#define MARKDOWN_RENDER_H

#include <cmark-gfm.h>
#include "render.h"

class MarkdownRender
{
public:
    MarkdownRender();

private:
    void addMarkdownExtension(cmark_parser *parser, const char *extName);
    char * toHTML(const char *markdown_string);
    static int S_render_node(cmark_renderer *renderer, cmark_node *node,
                         cmark_event_type ev_type, int options);
    char * renderWithMem(cmark_node *root, int options, int width, cmark_mem *mem);
};
#endif // MARKDOWN_RENDER_H
