#include "Engine/UI/Button.hpp"

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/UI/UIStyle.hpp"

#include <algorithm>
#include <utility>

namespace Engine {

namespace {

void drawIconRect(Renderer2D& renderer2D, const glm::vec2& center, const glm::vec2& size, const glm::vec4& color)
{
    renderer2D.drawSdfRect(center - size * 0.5f, size, 1.5f, color, color, 0.0f);
}

} // namespace

void drawUIIcon(Renderer2D& renderer2D, const UIIcon icon, const glm::vec2& center, const float size, const glm::vec4& color)
{
    const float unit = std::max(1.0f, size / 12.0f);

    switch (icon) {
    case UIIcon::Play:
        drawIconRect(renderer2D, center + glm::vec2{-unit * 1.5f, -unit * 3.0f}, {unit * 3.0f, unit * 2.0f}, color);
        drawIconRect(renderer2D, center, {unit * 6.0f, unit * 2.0f}, color);
        drawIconRect(renderer2D, center + glm::vec2{-unit * 1.5f, unit * 3.0f}, {unit * 3.0f, unit * 2.0f}, color);
        break;
    case UIIcon::Pause:
        drawIconRect(renderer2D, center + glm::vec2{-unit * 2.0f, 0.0f}, {unit * 2.0f, unit * 8.0f}, color);
        drawIconRect(renderer2D, center + glm::vec2{unit * 2.0f, 0.0f}, {unit * 2.0f, unit * 8.0f}, color);
        break;
    case UIIcon::Stop:
        drawIconRect(renderer2D, center, {unit * 7.0f, unit * 7.0f}, color);
        break;
    case UIIcon::ChevronDown:
        drawIconRect(renderer2D, center + glm::vec2{-unit * 3.0f, -unit * 1.5f}, {unit * 3.0f, unit * 3.0f}, color);
        drawIconRect(renderer2D, center + glm::vec2{0.0f, unit * 1.5f}, {unit * 3.0f, unit * 3.0f}, color);
        drawIconRect(renderer2D, center + glm::vec2{unit * 3.0f, -unit * 1.5f}, {unit * 3.0f, unit * 3.0f}, color);
        break;
    case UIIcon::ChevronUp:
        drawIconRect(renderer2D, center + glm::vec2{-unit * 3.0f, unit * 1.5f}, {unit * 3.0f, unit * 3.0f}, color);
        drawIconRect(renderer2D, center + glm::vec2{0.0f, -unit * 1.5f}, {unit * 3.0f, unit * 3.0f}, color);
        drawIconRect(renderer2D, center + glm::vec2{unit * 3.0f, unit * 1.5f}, {unit * 3.0f, unit * 3.0f}, color);
        break;
    case UIIcon::None:
        break;
    }
}

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

    const float automaticIconSize = std::clamp(std::min(m_size.x, m_size.y) * 0.55f, 10.0f, 18.0f);
    const float iconSize = buttonStyle.iconSize > 0.0f
        ? std::min(buttonStyle.iconSize, std::min(m_size.x, m_size.y))
        : automaticIconSize;
    const glm::vec2 iconPosition = m_position + (m_size - glm::vec2{iconSize, iconSize}) * 0.5f;
    if (m_iconImage && m_iconImage->texture != 0U) {
        renderer2D.drawImage(
            m_iconImage->texture,
            iconPosition,
            {iconSize, iconSize},
            m_iconImage->uvMinimum,
            m_iconImage->uvMaximum,
            m_iconImage->tint);
    } else if (m_icon != UIIcon::None) {
        drawUIIcon(renderer2D, m_icon, m_position + m_size * 0.5f, iconSize, buttonStyle.icon);
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

// Sets the optional primitive icon drawn in the center of the button.
void Button::setIcon(const UIIcon icon)
{
    m_icon = icon;
}

// Sets an optional texture-backed icon. The primitive icon remains as fallback.
void Button::setIconImage(const UIIconImage& icon)
{
    m_iconImage = icon;
}

// Clears the texture-backed icon so primitive icons can render again.
void Button::clearIconImage()
{
    m_iconImage.reset();
}

// Reports whether this button is currently selected.
bool Button::selected() const
{
    return m_selected;
}

// Reports the currently assigned icon.
UIIcon Button::icon() const
{
    return m_icon;
}

// Reports the currently assigned texture-backed icon, if any.
const std::optional<UIIconImage>& Button::iconImage() const
{
    return m_iconImage;
}

} // namespace Engine
