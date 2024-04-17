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
    if (m_scroll_target < 0) {
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

void threeml::Renderer::draw_status_bar() {
    m_display->fillRect(0, 0, m_display->width(), STATUS_BAR_HEIGHT,
                        ACCENT_COLOR);
    m_display->setTextColor(TEXT_COLOR, ACCENT_COLOR);
    m_display->setTextSize(2); // 12x16 pixels
    m_display->setCursor(2, 2);
    m_display->print("Battery: ");
    m_display->print(battery::get_level());
    m_display->print("%");
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
    }

    if (node->type == threeml::NodeType::DIV ||
        node->type == threeml::NodeType::BODY) {
        for (const auto child : node->children) {
            render_node(child, position, selectable_index);
        }
    }
}

bool threeml::Renderer::init() {
    if (m_initialized) {
        return true;
    }
    m_up_button.on(Button::Event::CLICK, [this]() { select_prev(); });
    m_down_button.on(Button::Event::CLICK, [this]() { select_next(); });
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
    draw_status_bar();

    m_display->refresh();
}

bool threeml::Renderer::load_file(const char *path) {
    fs::File f = FFat.open(path);
    if (!f) {
        return false;
    }
    char *buffer = new char[f.size() + 1];
    f.readBytes(buffer, f.size());
    buffer[f.size()] = '\0';
    f.close();
    load_dom(threeml::clean_dom(threeml::parse_string(buffer)));
    return true;
}

void threeml::Renderer::load_dom(threeml::DOM *dom) {
    m_dom = dom;
    m_dom_rendered = false;
    m_total_height = 0;
    refresh_selectable_nodes();
}
