#include "bluetooth.h"

bool bluetooth::Mouse::start()
{
    m_mouse.begin();
    return true;
}

bool bluetooth::Mouse::enabled()
{
    return m_enabled;
}

void bluetooth::Mouse::toggle_enabled()
{
    m_enabled = !m_enabled;
}

bluetooth::Mouse::Mode bluetooth::Mouse::movement_mode()
{
    return m_movement_mode;
}

void bluetooth::Mouse::set_movement_mode(bluetooth::Mouse::Mode mode)
{
    m_movement_mode = mode;
}

bool bluetooth::Mouse::scrolling()
{
    return m_scrolling;
}

void bluetooth::Mouse::toggle_scrolling()
{
    m_scrolling = !m_scrolling;
}

void bluetooth::Mouse::update(Orientation updated)
{
    m_current_orientation = updated;
    if (!m_enabled)
    {
        return;
    }
    else if (m_last_updated > 0.0 && (millis() - m_last_updated) < M_UPDATE_RATE)
    {
        return;
    }

    m_last_updated = millis();

    if (!m_scrolling)
    {
        switch (m_movement_mode)
        {
        case bluetooth::Mouse::Mode::ROLLING:
            m_mouse.move(m_current_orientation.roll, m_current_orientation.pitch);
            break;
        case bluetooth::Mouse::Mode::SWERVING:
            m_mouse.move(m_current_orientation.yaw, m_current_orientation.pitch);
            break;
        default:
            return;
        }
    }
    else
    {
        m_mouse.move(0, 0, m_current_orientation.pitch);
    }
}

void bluetooth::Mouse::click(ClickType click_type)
{
    if (!m_enabled)
    {
        return;
    }

    switch (click_type)
    {
    case bluetooth::Mouse::ClickType::LEFT:
        m_mouse.click(MOUSE_LEFT);
        break;
    case bluetooth::Mouse::ClickType::MIDDLE:
        m_mouse.click(MOUSE_MIDDLE);
        break;
    case bluetooth::Mouse::ClickType::RIGHT:
        m_mouse.click(MOUSE_RIGHT);
        break;
    default:
        break;
    }
}
