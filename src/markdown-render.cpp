#include "markdown-render.h"

#include <cmark-gfm-core-extensions.h>
#include <string.h>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QTextStream>

#include "node.h"
#include "syntax_extension.h"

static inline void outc(cmark_renderer *renderer, cmark_node *node, 
                        cmark_escaping escape,
                        int32_t c, unsigned char nextc) {
    cmark_render_code_point(renderer, c);
}

MarkdownRender::MarkdownRender()
{
    QString exePath = QCoreApplication::applicationDirPath();
    QString filePath = exePath + QDir::separator() + "../../test.md";

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "error opening file: " << file.error();
    }

    QTextStream instream(&file);
    QString line = instream.readLine();
    const char *LineCStr = line.toStdString().c_str();
    file.close();

    char *html = toHTML(LineCStr);

    printf("%s", html);
    free(html);
}

/**
 * This is a function that will make enabling extensions easier
 */
void MarkdownRender::addMarkdownExtension(cmark_parser *parser, char *extName) {
  cmark_syntax_extension *ext = cmark_find_syntax_extension(extName);
  if ( ext )
    cmark_parser_attach_syntax_extension(parser, ext);
}

// A function to convert HTML to markdown
char * MarkdownRender::toHTML(const char *markdown_string)
{
    int options = CMARK_OPT_DEFAULT; // You can also use CMARK_OPT_STRIKETHROUGH_DOUBLE_TILDE to enforce double tilde.

    //cmark_gfm_core_extensions_ensure_registered();

    // Modified version of cmark_parse_document in blocks.c
    cmark_parser *parser = cmark_parser_new(options);

    // Add extensions
    addMarkdownExtension(parser, "strikethrough");
    addMarkdownExtension(parser, "table");

    // cmark AST
    cmark_node *doc;
    cmark_parser_feed(parser, markdown_string, strlen(markdown_string));
    doc = cmark_parser_finish(parser);
    cmark_parser_free(parser);

    // no cmake_node_dump() ?

    cmark_node_mem(doc);
    // qDebug() << "AST" << doc->content.mem << endl;

    // Render
    char *html = cmark_render_html(doc, options, NULL);
    char *something = renderWithMem(doc, options, 0, cmark_node_mem(doc));

    cmark_node_free(doc);

    return html;
}

int MarkdownRender::S_render_node(cmark_renderer *renderer, cmark_node *node,
                         cmark_event_type ev_type, int options)
{
    qDebug() << "Type:" << node->type << endl;
}

char * MarkdownRender::renderWithMem(cmark_node *root, int options, int width, cmark_mem *mem)
{
    return cmark_render(mem, root, options, width, outc, S_render_node);
}
