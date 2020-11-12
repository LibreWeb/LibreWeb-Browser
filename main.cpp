#include "mainwindow.h"

#include <string.h>
#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QTextStream>

// This is a function that will make enabling extensions easier later on.
void addMarkdownExtension(cmark_parser *parser, char *extName) {
  cmark_syntax_extension *ext = cmark_find_syntax_extension(extName);
  if ( ext )
    cmark_parser_attach_syntax_extension(parser, ext);
}

// A function to convert HTML to markdown
char *to_html(const char *markdown_string)
{
    int options = CMARK_OPT_DEFAULT; // You can also use CMARK_OPT_STRIKETHROUGH_DOUBLE_TILDE to enforce double tilde.

    cmark_gfm_core_extensions_ensure_registered();

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
    qDebug() << "AST" << doc << endl;

    // Render
    char *html = cmark_render_html(doc, options, NULL);
    cmark_node_free(doc);

    return html;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    QString exePath = QCoreApplication::applicationDirPath();
    QString filePath = exePath + QDir::separator() + "../test.md";

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "error opening file: " << file.error();
        return -1;
    }

    QTextStream instream(&file);
    QString line = instream.readLine();
    const char *LineCStr = line.toStdString().c_str();
    file.close();

    char *html = to_html(LineCStr);

    printf("%s", html);
    free(html);

    w.show();
    return a.exec();
}
