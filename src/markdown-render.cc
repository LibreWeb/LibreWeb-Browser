#include "markdown-render.h"

#include <string>
#include <cmark-gfm-core-extensions.h>
#include <time.h> 

#include <node.h>
#include <syntax_extension.h>

#define OUT(s, wrap, escaping) renderer->out(renderer, node, s, wrap, escaping)
#define LIT(s) renderer->out(renderer, node, s, false, LITERAL)
#define CR() renderer->cr(renderer)

static inline void outc(cmark_renderer *renderer, cmark_node *node, 
                        cmark_escaping escape,
                        int32_t c, unsigned char nextc) {
    cmark_render_code_point(renderer, c);
}

MarkdownRender::MarkdownRender():
    options(CMARK_OPT_DEFAULT) {}


/**
 * Parse markdown by file path
 * @return AST structure (of type cmark_node)
 */
cmark_node * MarkdownRender::parseFile(const std::string &filePath)
{
    // Modified version of cmark_parse_document in blocks.c
    cmark_parser *parser = cmark_parser_new(options);

    // Add extensions
    addMarkdownExtension(parser, "strikethrough");
    addMarkdownExtension(parser, "table");
  
    cmark_parser_free(parser);

    // Parse to AST with cmark
    FILE *file;
    if( ( file = fopen(filePath.c_str(), "r" ) ) != NULL ) 
    {
        cmark_node *root_node;

        // TODO: Copy/paste cmark_parse_file() content to here, allowing me to add extensions to the parser.
        root_node = cmark_parse_file(file, options);
        fclose(file);

        return root_node;
    }
    return NULL;    
}

std::string const MarkdownRender::renderHTML(cmark_node *node)
{
    char *tmp = cmark_render_html(node, options, NULL);
    std::string output = std::string(tmp);
    free(tmp);
    return output;
}

std::string const MarkdownRender::renderMyLayout(cmark_node *node)
{
    char *tmp = renderLayout(node, options, 0, NULL);
    std::string output = std::string(tmp);
    free(tmp);
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

char *MarkdownRender::renderLayout(cmark_node *root, int options, int width, cmark_llist *extensions)
{
    return cmark_render(cmark_node_mem(root), root, options, width, outc, renderNode);
}

int MarkdownRender::renderNode(cmark_renderer *renderer, cmark_node *node,
                         cmark_event_type ev_type, int options)
{
    bool entering = (ev_type == CMARK_EVENT_ENTER);

    switch (node->type) {
    case CMARK_NODE_DOCUMENT:
        printf("Document\n");
        break;

    case CMARK_NODE_BLOCK_QUOTE:
        break;

    case CMARK_NODE_LIST:
        printf("List\n");
        break;

    case CMARK_NODE_ITEM:
        printf("Item\n");
        break;

    case CMARK_NODE_HEADING:
        printf("Heading\n");
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
        printf("Paragraph\n");
        break;

    case CMARK_NODE_TEXT:
        printf("Text\n");

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
        printf("Bold\n");
        if (entering) {
            LIT("[b]");
        } else {
            LIT("[/b]");
        }
        break;

    case CMARK_NODE_EMPH:
        printf("Italic\n");
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

