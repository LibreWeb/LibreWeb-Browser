#ifndef MARKDOWN_RENDER_H
#define MARKDOWN_RENDER_H

#include <string>
#include <cmark-gfm.h>
#include <render.h>

class MarkdownRender
{
public:
    MarkdownRender();
    cmark_node * parseFile(const std::string &filePath);
    std::string const renderHTML(cmark_node *node);
    std::string const renderMyLayout(cmark_node *node);

    std::string const renderDemoFile(); // As an example

private:
    std::string exePath;
    int options;

    void addMarkdownExtension(cmark_parser *parser, const char *extName);
    char * renderLayout(cmark_node *root, int options, int width, cmark_llist *extensions);
    static int renderNode(cmark_renderer *renderer, cmark_node *node,
                         cmark_event_type ev_type, int options);
};
#endif // MARKDOWN_RENDER_H
