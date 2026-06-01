#include "Engine/UI/Button.hpp"

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/UI/UIStyle.hpp"

#include <utility>

namespace Engine {

Button::Button(std::string id)
    : Widget(std::move(id))
{
}

// Updates button interaction and runs the click callback on release.
void Button::update(UIContext& context)
{
    Widget::update(context);
    if (m_interaction.pressed && m_onClick) {
        m_onClick();
    }
}

// Draws the button using the current theme and interaction state.
void Button::render(Renderer2D& renderer2D, const UIStyle& style) const
{
    if (!m_visible) {
        return;
    }

    const UIButtonStyle& buttonStyle = m_styleOverride ? *m_styleOverride : style.resolveButton(m_styleClass, m_id);
    const glm::vec4 fill = m_selected
        ? buttonStyle.selected
        : m_interaction.held
            ? buttonStyle.pressed
            : m_interaction.hovered
                ? buttonStyle.hovered
                : buttonStyle.normal.fill;
    const glm::vec4 border = m_selected ? buttonStyle.selectedBorder : buttonStyle.normal.border;

    renderer2D.drawSdfRect(
        m_position,
        m_size,
        buttonStyle.normal.borderRadius,
        fill,
        border,
        buttonStyle.normal.borderWidth);

    if (m_selected) {
        // renderer2D.drawSdfRect(
        //     {m_position.x, m_position.y + m_size.y - 4.0f},
        //     {m_size.x, 4.0f},
        //     0.0f,
        //     buttonStyle.accent,
        //     buttonStyle.accent,
        //     0.0f);
    }
}

// Sets whether the button should render as selected.
void Button::setSelected(const bool selected)
{
    m_selected = selected;
}

// Overrides the resolved button style for this specific button instance.
void Button::setStyle(const UIButtonStyle& style)
{
    m_styleOverride = style;
}

// Removes the local style override so class/default style resolution is used.
void Button::clearStyleOverride()
{
    m_styleOverride.reset();
}

// Registers work to perform when the button is clicked.
void Button::setOnClick(std::function<void()> callback)
{
    m_onClick = std::move(callback);
}

// Reports whether this button is currently selected.
bool Button::selected() const
{
    return m_selected;
}

} // namespace Engine
