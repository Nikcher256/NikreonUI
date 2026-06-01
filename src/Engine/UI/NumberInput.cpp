#include "Engine/UI/NumberInput.hpp"

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/UI/UIStyle.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

namespace Engine {

NumberInput::NumberInput(std::string id, const float value, const float minValue, const float maxValue)
    : Widget(std::move(id))
    , m_value(value)
    , m_minValue(minValue)
    , m_maxValue(maxValue)
{
    setValue(value);
}

// Scrubs the value horizontally while the control owns the mouse drag.
void NumberInput::update(UIContext& context)
{
    Widget::update(context);
    if (m_interaction.held && !m_wasHeld) {
        m_dragStartValue = m_value;
        m_dragStartMouseX = context.mousePosition().x;
    }

    if (m_interaction.held) {
        setValue(m_dragStartValue + (context.mousePosition().x - m_dragStartMouseX) * m_sensitivity);
    }

    m_wasHeld = m_interaction.held;
}

// Draws a compact Unreal-like scrub field with a left accent edge.
void NumberInput::render(Renderer2D& renderer2D, const UIStyle& style) const
{
    if (!m_visible) {
        return;
    }

    const UINumberInputStyle& inputStyle = m_styleOverride ? *m_styleOverride : style.resolveNumberInput(m_styleClass, m_id);
    const glm::vec4 fill = m_interaction.hovered || m_interaction.held ? inputStyle.hovered : inputStyle.box.fill;
    renderer2D.drawSdfRect(
        m_position,
        m_size,
        inputStyle.box.borderRadius,
        fill,
        inputStyle.box.border,
        inputStyle.box.borderWidth);
    renderer2D.drawSdfRect(
        m_position,
        {std::min(3.0f, m_size.x), m_size.y},
        inputStyle.box.borderRadius,
        inputStyle.accent,
        inputStyle.accent,
        0.0f);
}

void NumberInput::setValue(const float value)
{
    const float oldValue = m_value;
    m_value = std::clamp(value, m_minValue, m_maxValue);
    if (std::abs(oldValue - m_value) > 0.0001f && m_onValueChanged) {
        m_onValueChanged(m_value);
    }
}

void NumberInput::setRange(const float minValue, const float maxValue)
{
    m_minValue = minValue;
    m_maxValue = maxValue;
    setValue(m_value);
}

void NumberInput::setSensitivity(const float sensitivity)
{
    m_sensitivity = std::max(sensitivity, 0.0f);
}

void NumberInput::setPrecision(const int precision)
{
    m_precision = std::clamp(precision, 0, 6);
}

void NumberInput::setStyle(const UINumberInputStyle& style)
{
    m_styleOverride = style;
}

void NumberInput::clearStyleOverride()
{
    m_styleOverride.reset();
}

void NumberInput::setOnValueChanged(std::function<void(float)> callback)
{
    m_onValueChanged = std::move(callback);
}

float NumberInput::value() const
{
    return m_value;
}

std::string NumberInput::formattedValue() const
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(m_precision) << m_value;
    return stream.str();
}

} // namespace Engine
