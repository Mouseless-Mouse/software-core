#pragma once

#include "3ml_cleaner.h"
#include "duktape.h"
#include "meta.h"
#include <FFat.h>

namespace threeml {

meta::optional_t<DOMNode *> get_element_by_id(DOM *dom, std::string id);

static duk_ret_t _js_get_element_by_id(duk_context *ctx);
static duk_ret_t _js_set_inner_html(duk_context *ctx);

void construct_element(DOM *dom, DOMNode *node, duk_context *ctx);

void create_js_bindings(duk_context *ctx, DOM *dom);

} // namespace threeml