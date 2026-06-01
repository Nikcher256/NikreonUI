#include "Engine/UI/Widget.hpp"

#include <utility>

namespace Engine {

Widget::Widget(std::string id)
    : m_id(std::move(id))
{
}

// Updates basic rectangular interaction shared by all widgets.
void Widget::update(UIContext& context)
{
    if (!m_visible) {
        m_interaction = {};
        return;
    }

    m_interaction = context.interact(m_id, m_position, m_size);
}

// Places the widget in screen-space UI coordinates.
void Widget::setBounds(const glm::vec2& position, const glm::vec2& size)
{
    m_position = position;
    m_size = size;
}

// Controls whether the widget participates in update/render.
void Widget::setVisible(const bool visible)
{
    m_visible = visible;
}

// Assigns a reusable style class resolved by UIStyle during rendering.
void Widget::setStyleClass(std::string styleClass)
{
    m_styleClass = std::move(styleClass);
}

// Returns the stable widget id used for input ownership.
std::string_view Widget::id() const
{
    return m_id;
}

// Returns the optional class name used to look up widget-specific style data.
std::string_view Widget::styleClass() const
{
    return m_styleClass;
}

// Returns the widget's top-left position.
const glm::vec2& Widget::position() const
{
    return m_position;
}

// Returns the widget's current size.
const glm::vec2& Widget::size() const
{
    return m_size;
}

// Returns the interaction state produced during the latest update.
const UIInteraction& Widget::interaction() const
{
    return m_interaction;
}

// Reports whether the widget is active in layout and rendering.
bool Widget::visible() const
{
    return m_visible;
}

} // namespace Engine
