#pragma once

#include "3ml_cleaner.h"
#include "duktape.h"
#include "meta.h"
#include <FFat.h>

namespace threeml {

DOMNode *get_element_by_id(DOM *dom, std::string id);

static duk_ret_t _js_get_element_by_id(duk_context *ctx);
static duk_ret_t _js_set_inner_html(duk_context *ctx);

void construct_element(DOM *dom, DOMNode *node, duk_context *ctx);

void load_js_file(duk_context *ctx, const char *filename);

void create_js_bindings(duk_context *ctx, DOM *dom);
void switch_dom(DOM *dom, duk_context *ctx);

} // namespace threeml