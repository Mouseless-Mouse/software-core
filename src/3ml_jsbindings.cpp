#include "3ml_jsbindings.h"
#include "3ml_cleaner.h"
#include "duktape.h"
#include "meta.h"
#include <FFat.h>
#include <queue>

meta::optional_t<threeml::DOMNode *> get_element_by_id(threeml::DOM *dom,
                                                       std::string id) {
    // Breadth-first search! Because I'm not writing a recursive helper
    // function.
    std::queue<threeml::DOMNode *> q;
    for (auto node : dom->top_level_nodes) {
        q.push(node);
    }
    while (!q.empty()) {
        auto node = q.front();
        q.pop();
        if (node->id == id) {
            return meta::optional_t<threeml::DOMNode *>(node);
        }
        for (auto child : node->children) {
            q.push(child);
        }
    }
    return meta::optional_t<threeml::DOMNode *>();
}

duk_ret_t threeml::_js_get_element_by_id(duk_context *ctx) {
    // Get the DOM object
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "_dom");
    threeml::DOM *dom = static_cast<threeml::DOM *>(duk_get_pointer(ctx, -1));
    duk_pop(ctx);
    const char *id = duk_require_string(ctx, 0);
    auto node = get_element_by_id(dom, id);
    if (node.has_value) {
        duk_push_pointer(ctx, node.value);
    } else {
        duk_push_undefined(ctx);
    }
    return 1;
}

duk_ret_t threeml::_js_set_inner_html(duk_context *ctx) {
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "_node");
    DOMNode *node = static_cast<DOMNode *>(duk_get_pointer(ctx, -1));
    duk_pop(ctx);
    const char *html = duk_require_string(ctx, 0);
    auto uncleaned = threeml::parse_string(html);
    node->children.clear();
    for (const auto &top_level : uncleaned.top_level_nodes) {
        node->children.push_back(threeml::clean_node(top_level, node));
    }
    return 0;
}

void threeml::construct_element(DOM *dom, DOMNode *node, duk_context *ctx) {
    duk_push_object(ctx);
    duk_push_pointer(ctx, node);
    duk_put_prop_string(ctx, -2, "_node");
    for (auto &attr : node->unique_attributes) {
        duk_push_string(ctx, attr.second.c_str());
        duk_put_prop_string(ctx, -2, attr.first.c_str());
    }
    duk_push_string(ctx, "innerHTML");
    duk_push_c_function(ctx, _js_set_inner_html, 1);
    duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_SETTER);
}

void threeml::create_js_bindings(duk_context *ctx, threeml::DOM *dom) {
    duk_push_global_object(ctx);

    // Create the document object
    duk_push_object(ctx);
    duk_push_pointer(ctx, dom);
    duk_put_prop_string(ctx, -2, "_dom");
    duk_push_c_function(ctx, _js_get_element_by_id, 1);
    duk_put_prop_string(ctx, -2, "getElementById");
    duk_put_prop_string(ctx, -2, "document");
}
