#pragma once

#include "3ml_cleaner.h"
#include "3ml_jsbindings.h"
#include "battery.h"
#include "button.h"
#include "display.h"
#include "duktape.h"
#include "meta.h"
#include "state.h"
#include "usb_classes.h"
#include <Arduino.h>
#include <stack>

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

        /// @brief Determines if the node is visible on the screen.
        /// @param scroll_height The current scroll position on the page.
        /// @param display_height The height of the viewport.
        /// @return A bool indicating whether the node is visible on the screen.
        bool is_visible(std::size_t scroll_height, std::size_t display_height);
    };

    TFT_Parallel *m_display;
    DOM *m_dom;
    SemaphoreHandle_t m_dom_mutex;
    std::size_t m_scroll_height;
    std::vector<selectable_node_t> m_selectable_nodes;
    std::size_t m_current_selected;
    long m_scroll_target; // signed to allow for simpler clamping code
    Button m_up_button;
    Button m_down_button;
    bool m_dom_rendered;
    bool m_initialized;
    std::stack<std::string> m_file_stack;
    std::size_t m_total_height;
    std::string m_title;
    duk_context *m_js_ctx;
    bool m_must_reload;
    std::string m_current_file;
    bool m_going_back;
    bool m_callback_to_run;
    std::string m_pending_callback;

    /// @brief Clamps the value of m_scroll_target so that the screen doesn't
    /// show anything outside of the document, if possible.
    void clamp_scroll_target();

    /// @brief Traverse the DOM to find all selectable nodes.
    /// @param root The root node of the subtree being examined.
    void explore_dom(DOMNode *root);

    /// @brief Clears the list of selectable nodes and repopulates it.
    void refresh_selectable_nodes();

    /// @brief Selects the next selectable node and scrolls until it is visible
    /// or just scrolls if there isn't any next node to select.
    void select_next();

    /// @brief Selects the previous selectable node and scrolls until it is
    /// visible or just scrolls if there isn't any previous node to select.
    void select_prev();

    /// @brief Interacts with the currently selected node, if there is one.
    void interact();

    void go_back();

    /// @brief Renders the status bar on the screen. Status bar is always at the
    /// top, is STATUS_BAR_HEIGHT pixels tall, and its background is
    /// ACCENT_COLOR.
    void draw_status_bar();

    /// @brief Renders plaintext data to the screen. Background color is
    /// BACKGROUND_COLOR, text color is TEXT_COLOR. Font is 12x16 pixels, and
    /// there is 2 pixels of padding after each line.
    /// @param plaintext_data The plaintext data to render.
    /// @param position The current scroll position on the page. Updated after
    /// the call to reflect the bottom of the rendered node.
    void render_plaintext(const std::vector<std::string> &plaintext_data,
                          std::size_t &position);

    /// @brief Renders a link to the screen. Rendering is the same as plaintext,
    /// with the exception of colors. If the link is currently selected,
    /// background color is ACCENT_COLOR; otherwise, it is BACKGROUND_COLOR.
    /// Additionally, if selected, the text color is TEXT_COLOR, but if not, it
    /// is ACCENT_COLOR.
    /// @param node The link node.
    /// @param position The current scroll position on the page. Updated after
    /// the call to reflect the bottom of the rendered node.
    /// @param index The index in the selectable nodes list of this node.
    void render_link(const DOMNode *node, std::size_t &position,
                     std::size_t index);

    /// @brief Renders an H1 tag to the screen. Rendering is the same as
    /// plaintext, except the font is 24x32 pixels. Padding is unchanged.
    /// @param plaintext_data The underlying plaintext data to render.
    /// @param position The current scroll position on the page. Updated after
    /// the call to reflect the bottom of the rendered node.
    void render_h1(const std::vector<std::string> &plaintext_data,
                   std::size_t &position);

    /// @brief Renders a button to the screen. Buttons are rendered in a
    /// rectangle with width (text_width + 10) pixels and height (text_height +
    /// 2 * text_lines + 10) pixels. If not selected, the border of the
    /// rectangle is TEXT_COLOR; if selected, the border is ACCENT_COLOR. The
    /// text color is always TEXT_COLOR. The text is offset from the top-left
    /// corner of the rectangle by 5px each direction.
    /// @param node The button node.
    /// @param position The current scroll position on the page. Updated after
    /// the call to reflect the bottom of the rendered node.
    /// @param index The index in the selectable nodes list of this node.
    void render_button(const DOMNode *node, std::size_t &position,
                       std::size_t index);

    /// @brief Renders a DOM node to the screen. Recursively draws nodes in a
    /// pre-order fashion (i.e. parent, then children). Invisible nodes (like
    /// metadata) are not rendered.
    /// @param node The node to render.
    /// @param position The current scroll position on the page. Updated after
    /// the call to reflect the bottom of the rendered node.
    /// @param selectable_index The index in the selectable nodes list of the
    /// next selectable node (whether it is this node or a child node).
    void render_node(const DOMNode *node, std::size_t &position,
                     std::size_t &selectable_index);

  public:
    Renderer(TFT_Parallel *display)
        : m_display(display), m_dom(nullptr), m_scroll_height(0),
          m_current_selected(0), m_up_button(0), m_down_button(14),
          m_dom_rendered(false), m_initialized(false), m_js_ctx(nullptr),
          m_title("3ML"), m_total_height(0), m_scroll_target(0), m_file_stack(),
          m_selectable_nodes(), m_dom_mutex(nullptr) {
        m_dom_mutex = xSemaphoreCreateMutex();
    }
    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;
    Renderer(Renderer &&) = default;
    Renderer &operator=(Renderer &&) = default;

    // Calls the onbeforeunload event on the current DOM, if there is one.
    ~Renderer();

    /// @brief Initializes the renderer by setting up the display, buttons and
    /// Javascript context. Can be called multiple times without issue.
    /// @return A boolean indicating if initialization was successful.
    bool init();

    /// @brief Draws the current state of the DOM to the screen. If there is no
    /// loaded DOM, refreshes the screen and just draws a blank status bar.
    void render();

    /// @brief Loads a 3ML file into the renderer. Clears and frees
    /// the old DOM if loading the new file was successful.
    /// @param path The path to the file to load.
    /// @param add_to_stack A boolean indicating if the file should be added to
    /// the file stack. Defaults to true.
    /// @return A boolean indicating if loading the file was successful.
    bool load_file(const char *path, bool add_to_stack = true);

    /// @brief Loads a new DOM into the renderer, clearing the old one. Does not
    /// free the old DOM, in case this method is used to reload the DOM after
    /// modifying it.
    /// @param dom The new DOM to load.
    void load_dom(DOM *dom);
};
} // namespace threeml