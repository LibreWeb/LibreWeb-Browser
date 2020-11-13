#include "markdown-render.h"

#include <cmark-gfm-core-extensions.h>

#include <string.h>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QTextStream>

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

    char *html = to_html(LineCStr);

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
char * MarkdownRender::to_html(const char *markdown_string)
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
    // qDebug() << "AST" << doc->content.mem << endl;

    // Render
    char *html = cmark_render_html(doc, options, NULL);

    cmark_node_free(doc);

    return html;
}

