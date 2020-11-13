#include "markdown-render.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <cmark-gfm-core-extensions.h>
#include <string.h>
#include <time.h> 

#include "node.h"
#include "syntax_extension.h"

#define OUT(s, wrap, escaping) renderer->out(renderer, node, s, wrap, escaping)
#define LIT(s) renderer->out(renderer, node, s, false, LITERAL)
#define CR() renderer->cr(renderer)

static inline void outc(cmark_renderer *renderer, cmark_node *node, 
                        cmark_escaping escape,
                        int32_t c, unsigned char nextc) {
    cmark_render_code_point(renderer, c);
}

MarkdownRender::MarkdownRender()
{
    exePath = QCoreApplication::applicationDirPath();
}

QString const MarkdownRender::render()
{
   QString filePath = exePath + QDir::separator() + "../../test.md";

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "error opening file: " << file.error();
    }
    char *message = parseAndRender(filePath);

    QString output = QString::fromUtf8(message);
    free(message);
    return output;
}

/**
 * This is a function that will make enabling extensions easier
 */
void MarkdownRender::addMarkdownExtension(cmark_parser *parser, const char *extName) {
  cmark_syntax_extension *ext = cmark_find_syntax_extension(extName);
  if ( ext )
    cmark_parser_attach_syntax_extension(parser, ext);
}

char *MarkdownRender::parseAndRender(const QString& filePath)
{
    int options = CMARK_OPT_DEFAULT; // You can also use CMARK_OPT_STRIKETHROUGH_DOUBLE_TILDE to enforce double tilde.

    cmark_gfm_core_extensions_ensure_registered();

    // Modified version of cmark_parse_document in blocks.c
    cmark_parser *parser = cmark_parser_new(options);

    // Add extensions
    addMarkdownExtension(parser, "strikethrough");
    addMarkdownExtension(parser, "table");
    
    // Start measurement
    clock_t t; 
    t = clock(); 

    // Parse to AST with cmark
    cmark_node *root_node;
    FILE * file;
    file = fopen(filePath.toStdString().c_str(), "r");
    // TODO: Copy/paste cmark_parse_file() content, allowing me to add extensions to the parser.
    root_node = cmark_parse_file(file, options);
    fclose(file);

    cmark_parser_free(parser);

    cmark_node_mem(root_node);

    // Render
    char *output = renderToLayout(root_node, options, 0, NULL);
    // Stop measurement
    t = clock() - t; 
    double timeDuration = (((double)t)/CLOCKS_PER_SEC) * 1000; // ms

    char *html = cmark_render_html(root_node, options, NULL);

    printf("HTML render: %s", html);
    printf("My render: %s", output);
    qDebug() << "Content loaded, parsed & rendered time:" << timeDuration << "ms" << endl;

    cmark_node_free(root_node);

    return output;
}

int MarkdownRender::renderNode(cmark_renderer *renderer, cmark_node *node,
                         cmark_event_type ev_type, int options)
{
    bool entering = (ev_type == CMARK_EVENT_ENTER);

    switch (node->type) {
    case CMARK_NODE_DOCUMENT:
        qDebug() << "Document" << endl;
        break;

    case CMARK_NODE_BLOCK_QUOTE:
        break;

    case CMARK_NODE_LIST:
        qDebug() << "List" << endl;
        break;

    case CMARK_NODE_ITEM:
        qDebug() << "Item" << endl;
        break;

    case CMARK_NODE_HEADING:
        qDebug() << "Heading" << endl;
        break;

    case CMARK_NODE_CODE_BLOCK:
        break;

    case CMARK_NODE_HTML_BLOCK:
        break;

    case CMARK_NODE_CUSTOM_BLOCK:
        break;

    case CMARK_NODE_THEMATIC_BREAK:
        break;

    case CMARK_NODE_PARAGRAPH:
        qDebug() << "Paragraph" << endl;
        break;

    case CMARK_NODE_TEXT:
        qDebug() << "Text" << endl;

        // False = no wrap, we didn't specify a width
        OUT(cmark_node_get_literal(node), false, NORMAL);
        break;

    case CMARK_NODE_LINEBREAK:
        break;

    case CMARK_NODE_SOFTBREAK:
        break;

    case CMARK_NODE_CODE:
        break;

    case CMARK_NODE_HTML_INLINE:
        break;

    case CMARK_NODE_CUSTOM_INLINE:
        break;

    case CMARK_NODE_STRONG:
        qDebug() << "Bold" << endl;
        if (entering) {
            LIT("[b]");
        } else {
            LIT("[/b]");
        }
        break;

    case CMARK_NODE_EMPH:
        qDebug() << "Italic" << endl;
        if (entering) {
            LIT("_");
        } else {
            LIT("_");
        }
        break;

    case CMARK_NODE_LINK:
        break;

    case CMARK_NODE_IMAGE:
        break;

    case CMARK_NODE_FOOTNOTE_REFERENCE:
        break;

    case CMARK_NODE_FOOTNOTE_DEFINITION:
        break;
    default:
        assert(false);
        break;
    }

    return 1;
}

char *MarkdownRender::renderToLayout(cmark_node *root, int options, int width, cmark_llist *extensions)
{
    return cmark_render(cmark_node_mem(root), root, options, width, outc, renderNode);
}
