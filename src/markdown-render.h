#ifndef MARKDOWN_RENDER_H
#define MARKDOWN_RENDER_H

#include <string>
#include <cmark-gfm.h>
#include <render.h>

class MarkdownRender
{
public:
    MarkdownRender();
    std::string const render();

private:
    std::string exePath;

    void addMarkdownExtension(cmark_parser *parser, const char *extName);
    std::string parseRenderFromFile(const std::string &filePath);
    static int renderNode(cmark_renderer *renderer, cmark_node *node,
                         cmark_event_type ev_type, int options);
    char * renderToLayout(cmark_node *root, int options, int width, cmark_llist *extensions);
};
#endif // MARKDOWN_RENDER_H
