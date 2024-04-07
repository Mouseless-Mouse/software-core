#pragma once

#include <Arduino.h>
#include <BleMouse.h>
#include "sensor.h"

namespace bluetooth
{
    class Mouse
    {
    public:
        /// @brief Movement modes for the mouse.
        enum class Mode
        {
            SWERVING,
            ROLLING
        };
        /// @brief Types of mouse click event.
        enum class ClickType
        {
            LEFT,
            MIDDLE,
            RIGHT
        };

    private:
        BleMouse m_mouse;
        bool m_enabled;
        bool m_scrolling;
        Mode m_movement_mode;
        Orientation m_current_orientation;
        float m_last_updated;
        const float M_UPDATE_RATE;

    public:
        /// @brief Creates the Mouse object.
        /// @param update_rate Time interval in milliseconds between bluetooth updates.
        Mouse(float update_rate)
            : m_mouse(), m_enabled(false), m_movement_mode(Mode::ROLLING), m_last_updated(-1.0), M_UPDATE_RATE(update_rate), m_scrolling(false)
        {
        }
        ~Mouse() = default;
        Mouse(Mouse &) = default;
        Mouse(Mouse &&) = default;
        Mouse &operator=(Mouse &) = default;
        Mouse &operator=(Mouse &&) = default;

        /// @brief Initializes all mouse components and starts operation.
        /// @return A boolean indicating if initialization was successful.
        bool start();

        bool enabled();
        void toggle_enabled();

        Mode movement_mode();
        void set_movement_mode(Mode mode);
        bool scrolling();
        void toggle_scrolling();

        /// @brief Take in new sensor data and move the mouse if applicable.
        /// @param updated The new sensor data.
        void update(Orientation updated);
        /// @brief Click the mouse, if enabled.
        /// @param click_type The type of click to send.
        void click(ClickType click_type);
    };
}