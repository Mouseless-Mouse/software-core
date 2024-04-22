#include "3ml_jsbindings.h"
#include "3ml_cleaner.h"
#include "duktape.h"
#include "meta.h"
#include "state.h"
#include <FFat.h>
#include <queue>

threeml::DOMNode *threeml::get_element_by_id(threeml::DOM *dom,
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
            return node;
        }
        for (auto child : node->children) {
            q.push(child);
        }
    }
    return nullptr;
}

duk_ret_t threeml::_js_get_element_by_id(duk_context *ctx) {
    // Get the DOM object
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("dom"));
    threeml::DOM *dom = static_cast<threeml::DOM *>(duk_get_pointer(ctx, -1));
    duk_pop(ctx);
    const char *id = duk_get_string(ctx, 0);
    auto node = threeml::get_element_by_id(dom, id);
    if (node == nullptr) {
        duk_push_undefined(ctx);
        return 1;
    }
    construct_element(dom, node, ctx);
    return 1;
}

duk_ret_t threeml::_js_set_inner_html(duk_context *ctx) {
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("node"));
    DOMNode *node = static_cast<DOMNode *>(duk_get_pointer(ctx, -1));
    duk_pop(ctx);
    const char *html = duk_to_string(ctx, 0);
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
    duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("node"));
    for (auto &attr : node->unique_attributes) {
        duk_push_string(ctx, attr.second.c_str());
        duk_put_prop_string(ctx, -2, attr.first.c_str());
    }
    duk_push_string(ctx, "innerHTML");
    duk_push_c_function(ctx, _js_set_inner_html, 1);
    duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_SETTER);
}

void threeml::load_js_file(duk_context *ctx, const char *filename) {
    auto file = FFat.open(filename, FILE_READ);
    if (!file) {
        return;
    }
    char *buffer = new char[file.size() + 1];
    file.readBytes(buffer, file.size());
    buffer[file.size()] = '\0';
    TaskPrint().println(buffer);
    file.close();
    duk_peval_string(ctx, buffer);
}

void threeml::create_js_bindings(duk_context *ctx, threeml::DOM *dom) {
    // Create the document object
    duk_push_global_object(ctx);
    duk_push_object(ctx);
    duk_push_pointer(ctx, dom);
    duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("dom"));
    duk_push_c_function(ctx, _js_get_element_by_id, 1);
    duk_put_prop_string(ctx, -2, "getElementById");
    duk_put_prop_string(ctx, -2, "document");
    // duk_pop(ctx);
}

void threeml::switch_dom(DOM *dom, duk_context *ctx) {
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "document");
    duk_push_pointer(ctx, dom);
    duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("dom"));
    duk_pop(ctx);
}
