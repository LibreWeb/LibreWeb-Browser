/**
 * Cmark Subscript inline extension by Melroy van den Berg
 * Usage: ~Subscript~
 * TODO: Is conflicting with highlight, so I need to merge to 2 features into 1 solution.
 * We have the same matching char (~) for both subscript: 1x~ and highlight 2x~
 */
#ifndef CMARK_GFM_SUBSCRIPT_H
#define CMARK_GFM_SUBSCRIPT_H

#include "cmark-gfm-core-extensions.h"

extern cmark_node_type CMARK_NODE_SUBSCRIPT;
cmark_syntax_extension *create_subscript_extension(void);

#endif
