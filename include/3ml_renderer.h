#pragma once

#include "3ml_cleaner.h"
#include "battery.h"
#include "display.h"
#include "state.h"
#include "usb_classes.h"
#include <Arduino.h>

#define STATUS_BAR_HEIGHT 20                    // Pixels
#define ACCENT_COLOR color_rgb(0, 251, 0)       // 5-6-5 Color (bright green)
#define SECONDARY_COLOR color_rgb(0, 127, 0)    // 5-6-5 Color (muted green)
#define TEXT_COLOR color_rgb(255, 255, 255)     // 5-6-5 Color (white)

namespace threeml {
class Renderer {
private:
  TFT_Parallel *m_display;
  DOM *m_dom;
  DOMNode *m_selected_node;
  std::size_t m_scroll_height;

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

  void render_node(const DOMNode *node, std::size_t &lowest_scroll_height) {
    if (lowest_scroll_height >
        m_scroll_height + (m_display->height() - STATUS_BAR_HEIGHT)) {
      return;
    }

    const DOMNode *child = nullptr; // used for display of certain tags

    switch (node->type) {
    case NodeType::PLAINTEXT:
      lowest_scroll_height += 4;
      m_display->setTextColor(TEXT_COLOR, 0);
      m_display->setTextSize(2); // 12x16 pixels
      for (const auto &line : node->plaintext_data) {
        m_display->setCursor(2, STATUS_BAR_HEIGHT + lowest_scroll_height -
                                    m_scroll_height);
        lowest_scroll_height += 18; // 16 pixels of text + 2 pixels of padding
        m_display->print(line.c_str());
      }
      break;
    case NodeType::H1:
      child = node->children.front();
      lowest_scroll_height += 4;
      m_display->setTextColor(TEXT_COLOR, 0);
      m_display->setTextSize(3); // 18x24 pixels
      for (const auto &line : child->plaintext_data) {
        m_display->setCursor(2, STATUS_BAR_HEIGHT + lowest_scroll_height -
                                    m_scroll_height);
        lowest_scroll_height += 26; // 24 pixels of text + 2 pixels of padding
        m_display->print(line.c_str());
      }
      break;
    case NodeType::A:
      child = node->children.front();
      if (node == m_selected_node) {
        m_display->setTextColor(TEXT_COLOR, ACCENT_COLOR);
      } else {
        m_display->setTextColor(SECONDARY_COLOR, 0);
      }
      lowest_scroll_height += 4;
      m_display->setTextSize(2); // 12x16 pixels
      for (const auto &line : child->plaintext_data) {
        m_display->setCursor(2, STATUS_BAR_HEIGHT + lowest_scroll_height -
                                    m_scroll_height);
        lowest_scroll_height += 18; // 16 pixels of text + 2 pixels of padding
        m_display->print(line.c_str());
      }
      break;
    }

    if (node->type == NodeType::DIV || node->type == NodeType::BODY) {
      for (const auto child : node->children) {
        render_node(child, lowest_scroll_height);
      }
    }
  }

public:
  Renderer(TFT_Parallel *display, DOM *dom)
      : m_display(display), m_dom(dom), m_scroll_height(0),
        m_selected_node(nullptr) {}
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;
  Renderer(Renderer &&) = default;
  Renderer &operator=(Renderer &&) = default;
  ~Renderer() = default;

  void render() {
    while (!m_display->done_refreshing())
      ;
    m_display->clear();

    draw_status_bar();
    if (m_dom == nullptr) {
      m_display->refresh();
      return;
    }

    std::size_t iliterallydonotcare = 0;
    for (const auto node : m_dom->top_level_nodes) {
      render_node(node, iliterallydonotcare);
    }

    m_display->refresh();
  }

  void set_dom(DOM *dom) { m_dom = dom; }
};
} // namespace threeml