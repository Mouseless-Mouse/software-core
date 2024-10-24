#include "3ml_renderer.h"
#include "3ml_cleaner.h"
#include "battery.h"
#include "button.h"
#include "display.h"
#include "state.h"
#include "usb_classes.h"
#include <Arduino.h>
#include <FFat.h>

bool threeml::Renderer::selectable_node_t::is_visible(
    std::size_t scroll_height, std::size_t display_height) {
    return top >= scroll_height && bottom <= scroll_height + display_height;
}

void threeml::Renderer::clamp_scroll_target() {
    if (m_scroll_target < 0 ||
        m_total_height <= m_display->height() - STATUS_BAR_HEIGHT) {
        m_scroll_target = 0;
    } else if (m_scroll_target >
               m_total_height - m_display->height() + STATUS_BAR_HEIGHT) {
        m_scroll_target =
            m_total_height - m_display->height() + STATUS_BAR_HEIGHT;
    }
}

void threeml::Renderer::explore_dom(threeml::DOMNode *root) {
    if (root == nullptr) {
        return;
    }
    if (root->selectable) {
        m_selectable_nodes.push_back(selectable_node_t(root));
    }
    for (const auto child : root->children) {
        explore_dom(child);
    }
}

void threeml::Renderer::refresh_selectable_nodes() {
    m_selectable_nodes.clear();
    for (const auto node : m_dom->top_level_nodes) {
        explore_dom(node);
    }
}

void threeml::Renderer::select_next() {
    // If there are no selectable nodes or we are at the end of the list, just
    // scroll downwards a smidge.
    constexpr std::size_t SCROLL_AMOUNT = 40;
    if (m_selectable_nodes.empty() ||
        m_current_selected >= m_selectable_nodes.size() - 1) {
        m_scroll_target += SCROLL_AMOUNT;
        clamp_scroll_target();
        return;
    }
    // Otherwise, select the next node.
    m_current_selected++;
    auto node = m_selectable_nodes[m_current_selected];
    if (node.is_visible(m_scroll_height,
                        m_display->height() - STATUS_BAR_HEIGHT)) {
        return;
    } else {
        // If the current node isn't visible, we need to scroll to it.
        m_scroll_target = node.top;
        clamp_scroll_target();
    }
}

void threeml::Renderer::select_prev() {
    // If there are no selectable nodes or we are at the beginning of the list,
    // just scroll upwards a smidge.
    constexpr std::size_t SCROLL_AMOUNT = 40;
    if (m_selectable_nodes.empty() || m_current_selected == 0) {
        m_scroll_target -= SCROLL_AMOUNT;
        clamp_scroll_target();
        return;
    }
    // Otherwise, select the previous node.
    m_current_selected--;
    auto node = m_selectable_nodes[m_current_selected];
    if (node.is_visible(m_scroll_height,
                        m_display->height() - STATUS_BAR_HEIGHT)) {
        return;
    } else {
        // If the current node isn't visible, we need to scroll to it.
        m_scroll_target = node.top;
    }
}

void threeml::Renderer::interact() {
    if (m_selectable_nodes.empty()) {
        return;
    }
    auto node = m_selectable_nodes[m_current_selected].node;
    switch (node->type) {
    case threeml::NodeType::A:
        m_must_reload = true;
        m_current_file = node->unique_attributes["href"];
        break;
    case threeml::NodeType::BUTTON:
        m_callback_to_run = true;
        m_pending_callback = node->unique_attributes["onclick"];
        break;
    }
}

void threeml::Renderer::go_back() {
    if (m_file_stack.size() < 2) {
        return;
    }
    m_file_stack.pop();
    m_must_reload = true;
    m_going_back = true;
    m_current_file = m_file_stack.top();
}

void threeml::Renderer::draw_status_bar() {
    m_display->fillRect(0, 0, m_display->width(), STATUS_BAR_HEIGHT,
                        ACCENT_COLOR);
    m_display->setTextColor(TEXT_COLOR, ACCENT_COLOR);
    m_display->setTextSize(2); // 12x16 pixels
    m_display->setCursor(2, 2);
    m_display->print(m_title.c_str());
}

void threeml::Renderer::render_plaintext(
    const std::vector<std::string> &plaintext_data, std::size_t &position) {
    m_display->setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
    m_display->setTextSize(2); // 12x16 pixels
    for (const auto &line : plaintext_data) {
        m_display->setCursor(2, STATUS_BAR_HEIGHT + position - m_scroll_height);
        position += 18; // 16 pixels of text + 2 pixels of padding
        m_display->print(line.c_str());
    }
}

void threeml::Renderer::render_link(const threeml::DOMNode *node,
                                    std::size_t &position, std::size_t index) {
    auto plaintext_data = node->children.front()->plaintext_data;
    // Update the positioning information for this node.
    m_selectable_nodes[index].top = position;
    // Are we selected? If so, invert the text color to highlight it.
    if (!m_selectable_nodes.empty() &&
        m_selectable_nodes[m_current_selected].node == node) {
        m_display->setTextColor(TEXT_COLOR, ACCENT_COLOR);
    } else {
        m_display->setTextColor(ACCENT_COLOR, BACKGROUND_COLOR);
    }
    m_display->setTextSize(2); // 12x16 pixels
    for (const auto &line : plaintext_data) {
        m_display->setCursor(2, STATUS_BAR_HEIGHT + position - m_scroll_height);
        position += 18; // 16 pixels of text + 2 pixels of padding
        m_display->print(line.c_str());
    }
    // Final update to the positioning information for this node.
    m_selectable_nodes[index].bottom = position;
}

void threeml::Renderer::render_h1(
    const std::vector<std::string> &plaintext_data, std::size_t &position) {
    m_display->setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
    m_display->setTextSize(3); // 18x24 pixels
    for (const auto &line : plaintext_data) {
        m_display->setCursor(2, STATUS_BAR_HEIGHT + position - m_scroll_height);
        position += 26; // 24 pixels of text + 2 pixels of padding
        m_display->print(line.c_str());
    }
}

void threeml::Renderer::render_button(const DOMNode *node,
                                      std::size_t &position,
                                      std::size_t index) {
    auto plaintext_data = node->children.front()->plaintext_data;
    // Update the positioning information for this node.
    m_selectable_nodes[index].top = position;
    // Calculate the width and height of the bounding box
    std::size_t width = 0;
    std::size_t height = 0;
    for (const auto &line : plaintext_data) {
        width = (line.size() > width) ? line.size() : width;
        height++;
    }
    width = width * 12 + 10;  // 12 pixels per character, plus 10 for padding
    height = height * 18 + 8; // Minus 2 for padding on the last line, plus 10
                              // for padding on the bottom
    auto border_color = TEXT_COLOR;
    if (!m_selectable_nodes.empty() &&
        m_selectable_nodes[m_current_selected].node == node) {
        border_color = ACCENT_COLOR;
    }
    auto screen_position =
        STATUS_BAR_HEIGHT + position -
        m_scroll_height; // Only used for drawing the box; the way text is drawn
                         // immediately invalidates this value, so we need to
                         // keep recalculating it.
    m_display->drawFastHLine(2, screen_position, width, border_color);
    m_display->drawFastHLine(2, screen_position + height, width, border_color);
    m_display->drawFastVLine(2, screen_position, height, border_color);
    m_display->drawFastVLine(2 + width, screen_position, height, border_color);
    m_display->setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
    m_display->setTextSize(2); // 12x16 pixels
    position += 5;             // 5 pixels of padding
    if (!plaintext_data.empty()) {
        m_display->setCursor(7, STATUS_BAR_HEIGHT + position - m_scroll_height);
        m_display->print(plaintext_data.front().c_str());
        position += 16; // 16 pixels of text
    }
    for (auto it = plaintext_data.begin() + 1; it != plaintext_data.end();
         it++) {
        position += 2; // 2 pixels of padding
        m_display->setCursor(7, STATUS_BAR_HEIGHT + position - m_scroll_height);
        m_display->print(it->c_str());
        position += 16; // 16 pixels of text
    }
    // Final update to the positioning information for this node.
    position = m_selectable_nodes[index].top + height + 2;
    m_selectable_nodes[index].bottom = position;
}

void threeml::Renderer::render_node(const threeml::DOMNode *node,
                                    std::size_t &position,
                                    std::size_t &selectable_index) {
    auto bottom_of_display =
        m_scroll_height + m_display->height() - STATUS_BAR_HEIGHT;
    if (m_dom_rendered && position > bottom_of_display) {
        return;
    }

    const threeml::DOMNode *child = nullptr; // used for display of certain tags

    switch (node->type) {
    case threeml::NodeType::PLAINTEXT:
        position += 4;
        render_plaintext(node->plaintext_data, position);
        break;
    case threeml::NodeType::H1:
        child = node->children.front();
        position += 4;
        render_h1(child->plaintext_data, position);
        break;
    case threeml::NodeType::A:
        child = node->children.front();
        position += 4;
        render_link(node, position, selectable_index);
        selectable_index++;
        break;
    case threeml::NodeType::BUTTON:
        child = node->children.front();
        position += 4;
        render_button(node, position, selectable_index);
        selectable_index++;
    }

    if (node->type == threeml::NodeType::DIV ||
        node->type == threeml::NodeType::BODY) {
        for (const auto child : node->children) {
            render_node(child, position, selectable_index);
        }
    }
}

threeml::Renderer::~Renderer() {
    vSemaphoreDelete(m_dom_mutex);
    if (m_dom == nullptr) {
        return;
    }
    delete m_dom;
}

bool threeml::Renderer::init() {
    if (m_initialized) {
        return true;
    }
    m_up_button.on(Button::Event::CLICK, [this]() { select_prev(); });
    m_down_button.on(Button::Event::CLICK, [this]() { select_next(); });
    m_down_button.on(Button::Event::HOLD, [this]() { interact(); });
    m_up_button.on(Button::Event::HOLD, [this]() { go_back(); });
    m_up_button.attach();
    m_down_button.attach();
    if (!FFat.begin(true)) {
        return false;
    }
    m_initialized = true;
    return true;
}

void threeml::Renderer::render() {
    while (!m_display->done_refreshing())
        ;
    m_display->fillScreen(BACKGROUND_COLOR);

    if (m_dom == nullptr) {
        // No DOM to render, so just draw the status bar and refresh the
        // display.
        draw_status_bar();
        m_display->refresh();
        return;
    }
    if (m_must_reload) {
        m_must_reload = false;
        load_file(m_current_file.c_str(), !m_going_back);
        m_going_back = false;
    } else if (m_callback_to_run) {
        m_callback_to_run = false;
        TaskLog().println("Running callback");
        if (m_js_ctx != nullptr) {
            duk_peval_string(m_js_ctx, m_pending_callback.c_str());
        }
    }

    xSemaphoreTake(m_dom_mutex, portMAX_DELAY); // Lock the DOM for rendering.
    if (m_dom_rendered) {
        clamp_scroll_target();
        // Smooth scrolling effect. Just uses an alpha filter.
        m_scroll_height = (m_scroll_height * 3 + m_scroll_target) / 4;
    }
    std::size_t position = 0;
    std::size_t selectable_index = 0;
    for (const auto node : m_dom->top_level_nodes) {
        render_node(node, position, selectable_index);
    }
    // I call this the "Christopher Columbus" method for determining the total
    // height of the document. It's expensive to render the entire document just
    // to see how tall it is, so we just keep updating our knowledge of how tall
    // the document is as we render it.
    m_total_height = (position > m_total_height) ? position : m_total_height;
    m_dom_rendered = true;
    xSemaphoreGive(m_dom_mutex); // Unlock the DOM for rendering.
    draw_status_bar();

    m_display->refresh();
}

bool threeml::Renderer::load_file(const char *path, bool add_to_stack) {
    fs::File f = FFat.open(path);
    if (!f) {
        return false;
    }
    char *buffer = new char[f.size() + 1];
    f.readBytes(buffer, f.size());
    buffer[f.size()] = '\0';
    f.close();
    auto tmp = m_dom;
    load_dom(threeml::clean_dom(threeml::parse_string(buffer)));
    if (add_to_stack) {
        m_file_stack.push(path);
    }
    for (const auto node : m_dom->top_level_nodes) {
        if (node->type != threeml::NodeType::BODY) {
            continue;
        }
        auto attrs = node->unique_attributes;
        if (attrs.find("onbeforeunload") != attrs.end()) {
            if (m_js_ctx != nullptr) {
                duk_peval_string(m_js_ctx, attrs["onbeforeunload"].c_str());
            }
            break;
        }
    }
    delete[] buffer;
    delete tmp;
    return true;
}

void threeml::Renderer::load_dom(threeml::DOM *dom) {
    xSemaphoreTake(m_dom_mutex, portMAX_DELAY);
    bool is_a_reload = (m_dom == dom);
    m_dom = dom;
    m_dom_rendered = false;
    m_total_height = 0;
    refresh_selectable_nodes();
    duk_destroy_heap(m_js_ctx);
    m_js_ctx = duk_create_heap_default();
    create_js_bindings(m_js_ctx, m_dom);
    for (const auto node : m_dom->top_level_nodes) {
        if (node->type == threeml::NodeType::BODY) {
            auto attrs = node->unique_attributes;
            if (attrs.find("onload") != attrs.end() && !is_a_reload) {
                if (m_js_ctx != nullptr) {
                    duk_peval_string(m_js_ctx, attrs["onload"].c_str());
                }
            }
            continue;
        }
        for (const auto child : node->children) {
            if (child->type == threeml::NodeType::SCRIPT) {
                if (m_js_ctx != nullptr) {
                    load_js_file(m_js_ctx,
                                 child->unique_attributes["src"].c_str());
                }
            } else if (child->type == threeml::NodeType::TITLE) {
                m_title = child->children.front()->plaintext_data.front();
            }
        }
    }
    xSemaphoreGive(m_dom_mutex);
}
