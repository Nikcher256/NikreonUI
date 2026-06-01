#include "Engine/UI/Checkbox.hpp"

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/UI/UIStyle.hpp"

#include <algorithm>
#include <utility>

namespace Engine {

Checkbox::Checkbox(std::string id, const bool checked)
    : Widget(std::move(id))
    , m_checked(checked)
{
}

// Updates checkbox interaction and toggles the stored value on click.
void Checkbox::update(UIContext& context)
{
    Widget::update(context);
    if (!m_interaction.pressed) {
        return;
    }

    m_checked = !m_checked;
    if (m_onValueChanged) {
        m_onValueChanged(m_checked);
    }
}

// Draws the checkbox box and inner mark when checked.
void Checkbox::render(Renderer2D& renderer2D, const UIStyle& style) const
{
    if (!m_visible) {
        return;
    }

    const UICheckboxStyle& checkboxStyle = m_styleOverride ? *m_styleOverride : style.resolveCheckbox(m_styleClass, m_id);
    const glm::vec4 fill = m_interaction.hovered ? checkboxStyle.hovered : checkboxStyle.box.fill;
    renderer2D.drawSdfRect(
        m_position,
        m_size,
        checkboxStyle.box.borderRadius,
        fill,
        checkboxStyle.box.border,
        checkboxStyle.box.borderWidth);

    if (m_checked) {
        renderer2D.drawSdfRect(
            {m_position.x + 5.0f, m_position.y + 5.0f},
            {m_size.x - 10.0f, m_size.y - 10.0f},
            std::max(0.0f, checkboxStyle.box.borderRadius - 2.0f),
            checkboxStyle.check,
            checkboxStyle.check,
            0.0f);
    }
}

// Updates the checked state without firing callbacks.
void Checkbox::setChecked(const bool checked)
{
    m_checked = checked;
}

// Overrides the resolved checkbox style for this specific checkbox instance.
void Checkbox::setStyle(const UICheckboxStyle& style)
{
    m_styleOverride = style;
}

// Removes the local style override so class/default style resolution is used.
void Checkbox::clearStyleOverride()
{
    m_styleOverride.reset();
}

// Registers work to perform when the checked state changes.
void Checkbox::setOnValueChanged(std::function<void(bool)> callback)
{
    m_onValueChanged = std::move(callback);
}

// Returns whether the checkbox is currently checked.
bool Checkbox::checked() const
{
    return m_checked;
}

} // namespace Engine
