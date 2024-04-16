#pragma once

#include "3ml_cleaner.h"
#include "battery.h"
#include "button.h"
#include "display.h"
#include "state.h"
#include "usb_classes.h"
#include <Arduino.h>

#define STATUS_BAR_HEIGHT 20 // Pixels
#define ACCENT_COLOR color_rgb(247, 176, 91)
#define SECONDARY_COLOR color_rgb(204, 88, 3)
#define TEXT_COLOR color_rgb(255, 255, 255)
#define BACKGROUND_COLOR color_rgb(31, 19, 0)

namespace threeml {

class Renderer {
  private:
    struct selectable_node_t {
        DOMNode *node;
        std::size_t top;
        std::size_t bottom;
        selectable_node_t(DOMNode *node) : node(node), top(0), bottom(0) {}

        bool is_visible(std::size_t scroll_height, std::size_t display_height) {
            return top >= scroll_height &&
                   bottom <= scroll_height + display_height;
        }
    };

    TFT_Parallel *m_display;
    DOM *m_dom;
    std::size_t m_scroll_height;
    std::vector<selectable_node_t> m_selectable_nodes;
    std::size_t m_current_selected;
    long m_scroll_to; // signed to allow for simpler clamping code
    Button m_up_button;
    Button m_down_button;
    bool m_dom_rendered;
    std::size_t m_total_height;

    void clamp_scroll_to() {
        if (m_scroll_to < 0) {
            m_scroll_to = 0;
        } else if (m_scroll_to >
                   m_total_height - m_display->height() + STATUS_BAR_HEIGHT) {
            m_scroll_to =
                m_total_height - m_display->height() + STATUS_BAR_HEIGHT;
        }
    }

    void explore_dom(DOMNode *root) {
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

    void refresh_selectable_nodes() {
        m_selectable_nodes.clear();
        for (const auto node : m_dom->top_level_nodes) {
            explore_dom(node);
        }
    }

    void select_next() {
        if (m_selectable_nodes.empty() ||
            m_current_selected >= m_selectable_nodes.size() - 1) {
            m_scroll_to += 40;
            clamp_scroll_to();
            return;
        }
        m_current_selected++;
        auto node = m_selectable_nodes[m_current_selected];
        if (node.is_visible(m_scroll_height,
                            m_display->height() - STATUS_BAR_HEIGHT)) {
            return;
        } else {
            m_scroll_to = node.top;
            clamp_scroll_to();
        }
    }

    void select_prev() {
        if (m_selectable_nodes.empty() || m_current_selected == 0) {
            m_scroll_to -= 40;
            clamp_scroll_to();
            return;
        }
        m_current_selected--;
        if (m_selectable_nodes[m_current_selected].is_visible(
                m_scroll_height, m_display->height() - STATUS_BAR_HEIGHT)) {
            return;
        } else {
            m_scroll_to = m_selectable_nodes[m_current_selected].top;
        }
    }

    void draw_status_bar() {
        m_display->fillRect(0, 0, m_display->width(), STATUS_BAR_HEIGHT,
                            ACCENT_COLOR);
        m_display->setTextColor(TEXT_COLOR, ACCENT_COLOR);
        m_display->setTextSize(2); // 12x16 pixels
        m_display->setCursor(2, 2);
        m_display->print("Battery: ");
        m_display->print(battery::get_level());
        m_display->print("%");
    }

    void render_plaintext(const std::vector<std::string> &plaintext_data,
                          std::size_t &lowest_scroll_height) {
        m_display->setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
        m_display->setTextSize(2); // 12x16 pixels
        for (const auto &line : plaintext_data) {
            m_display->setCursor(2, STATUS_BAR_HEIGHT + lowest_scroll_height -
                                        m_scroll_height);
            lowest_scroll_height +=
                18; // 16 pixels of text + 2 pixels of padding
            m_display->print(line.c_str());
        }
    }

    void render_link(const DOMNode *node,
                     const std::vector<std::string> &plaintext_data,
                     std::size_t &lowest_scroll_height,
                     int &last_selectable_node) {
        last_selectable_node++;
        m_selectable_nodes[last_selectable_node].top = lowest_scroll_height;
        if (!m_selectable_nodes.empty() &&
            m_selectable_nodes[m_current_selected].node == node) {
            m_display->setTextColor(TEXT_COLOR, ACCENT_COLOR);
        } else {
            m_display->setTextColor(ACCENT_COLOR, BACKGROUND_COLOR);
        }
        m_display->setTextSize(2); // 12x16 pixels
        for (const auto &line : plaintext_data) {
            m_display->setCursor(2, STATUS_BAR_HEIGHT + lowest_scroll_height -
                                        m_scroll_height);
            lowest_scroll_height +=
                18; // 16 pixels of text + 2 pixels of padding
            m_display->print(line.c_str());
        }
        m_selectable_nodes[last_selectable_node].bottom = lowest_scroll_height;
    }

    void render_h1(const std::vector<std::string> &plaintext_data,
                   std::size_t &lowest_scroll_height) {
        m_display->setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
        m_display->setTextSize(3); // 18x24 pixels
        for (const auto &line : plaintext_data) {
            m_display->setCursor(2, STATUS_BAR_HEIGHT + lowest_scroll_height -
                                        m_scroll_height);
            lowest_scroll_height +=
                26; // 24 pixels of text + 2 pixels of padding
            m_display->print(line.c_str());
        }
    }

    void render_node(const DOMNode *node, std::size_t &lowest_scroll_height,
                     int &last_selectable_node) {
        auto bottom_of_display =
            m_scroll_height + m_display->height() - STATUS_BAR_HEIGHT;
        if (m_dom_rendered && lowest_scroll_height > bottom_of_display) {
            return;
        }

        const DOMNode *child = nullptr; // used for display of certain tags

        switch (node->type) {
        case NodeType::PLAINTEXT:
            lowest_scroll_height += 4;
            render_plaintext(node->plaintext_data, lowest_scroll_height);
            break;
        case NodeType::H1:
            child = node->children.front();
            lowest_scroll_height += 4;
            render_h1(child->plaintext_data, lowest_scroll_height);
            break;
        case NodeType::A:
            child = node->children.front();
            lowest_scroll_height += 4;
            render_link(node, child->plaintext_data, lowest_scroll_height,
                        last_selectable_node);
            break;
        }

        if (node->type == NodeType::DIV || node->type == NodeType::BODY) {
            for (const auto child : node->children) {
                render_node(child, lowest_scroll_height, last_selectable_node);
            }
        }
    }

  public:
    Renderer(TFT_Parallel *display, DOM *dom)
        : m_display(display), m_dom(dom), m_scroll_height(0),
          m_current_selected(0), m_up_button(0), m_down_button(14),
          m_dom_rendered(false) {
        if (dom == nullptr) {
            return;
        }
        for (const auto node : m_dom->top_level_nodes) {
            explore_dom(node);
        }
    }
    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;
    Renderer(Renderer &&) = default;
    Renderer &operator=(Renderer &&) = default;
    ~Renderer() = default;

    bool init() {
        m_up_button.on(Button::Event::CLICK, [this]() { select_prev(); });
        m_down_button.on(Button::Event::CLICK, [this]() { select_next(); });
        m_up_button.attach();
        m_down_button.attach();
        return true;
    }

    void render() {
        while (!m_display->done_refreshing())
            ;
        m_display->fillScreen(BACKGROUND_COLOR);

        if (m_dom == nullptr) {
            draw_status_bar();
            m_display->refresh();
            return;
        }

        if (m_dom_rendered) {
            clamp_scroll_to();
            m_scroll_height = (m_scroll_height * 3 + m_scroll_to) / 4;
        }
        std::size_t lowest_scroll_height = 0;
        int last_selectable_node = -1;
        for (const auto node : m_dom->top_level_nodes) {
            render_node(node, lowest_scroll_height, last_selectable_node);
        }
        m_total_height = (lowest_scroll_height > m_total_height)
                             ? lowest_scroll_height
                             : m_total_height;
        m_dom_rendered = true;
        draw_status_bar();

        m_display->refresh();
    }

    void set_dom(DOM *dom) {
        m_dom = dom;
        m_dom_rendered = false;
        m_total_height = 0;
        refresh_selectable_nodes();
    }
};
} // namespace threeml