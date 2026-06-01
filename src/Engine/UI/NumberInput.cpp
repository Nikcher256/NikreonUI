#include "Engine/UI/NumberInput.hpp"

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/Renderer/TextRenderer.hpp"
#include "Engine/UI/UIStyle.hpp"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

namespace Engine {

NumberInput::NumberInput(std::string id, const float value, const float minValue, const float maxValue)
    : Widget(id)
    , m_value(value)
    , m_minValue(minValue)
    , m_maxValue(maxValue)
    , m_textEditor(std::move(id) + ".text-editor")
{
    setValue(value);
    m_textEditor.setOnValueChanged([this](const std::string_view text) {
        const std::string input{text};
        char* end = nullptr;
        const float parsedValue = std::strtof(input.c_str(), &end);
        if (end != nullptr && *end == '\0' && std::isfinite(parsedValue)) {
            setValue(parsedValue);
        }
    });
}

// Scrubs the value horizontally while the control owns the mouse drag.
void NumberInput::update(UIContext& context)
{
    updateScrubbing(context);
}

void NumberInput::update(
    UIContext& context,
    const TextRenderer& textRenderer,
    const UIStyle& style,
    const std::string_view fontName,
    const float scale)
{
    m_textEditor.setBounds(m_position, m_size);
    if (!m_editing) {
        updateScrubbing(context);
        if (m_interaction.pressed && !m_dragged) {
            beginEditing(context);
        }
    }

    if (m_editing) {
        const UINumberInputStyle& inputStyle = m_styleOverride ? *m_styleOverride : style.resolveNumberInput(m_styleClass, m_id);
        UITextInputStyle textInputStyle;
        textInputStyle.box = inputStyle.box;
        textInputStyle.hovered = inputStyle.hovered;
        textInputStyle.focused = inputStyle.box.fill;
        textInputStyle.focusedBorder = inputStyle.box.border;
        m_textEditor.setStyle(textInputStyle);
        m_textEditor.update(context, textRenderer, style, fontName, scale);
        m_interaction = m_textEditor.interaction();
        if (!m_textEditor.focused()) {
            m_editing = false;
            m_textEditor.setValue(formattedValue());
        }
    }
}

void NumberInput::updateScrubbing(UIContext& context)
{
    Widget::update(context);
    if (m_interaction.held && !m_wasHeld) {
        m_dragStartValue = m_value;
        m_dragStartMouseX = context.mousePosition().x;
        m_dragged = false;
    }

    if (m_interaction.held) {
        const float distance = context.mousePosition().x - m_dragStartMouseX;
        m_dragged = m_dragged || std::abs(distance) >= 3.0f;
        if (m_dragged) {
            setValue(m_dragStartValue + distance * m_sensitivity);
        }
    }

    m_wasHeld = m_interaction.held;
}

void NumberInput::beginEditing(UIContext& context)
{
    m_editing = true;
    m_textEditor.setValue(formattedValue());
    m_textEditor.selectAll();
    context.focus(m_textEditor.id());
}

// Draws a compact Unreal-like scrub field with a value-proportional fill.
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
    if (inputStyle.showValueFill) {
        renderer2D.drawSdfRect(
            m_position,
            {m_size.x * normalizedValue(), m_size.y},
            inputStyle.box.borderRadius,
            inputStyle.accent,
            inputStyle.accent,
            0.0f);
    }
}

void NumberInput::setValue(const float value)
{
    const float oldValue = m_value;
    m_value = std::clamp(value, m_minValue, m_maxValue);
    if (std::abs(oldValue - m_value) > 0.0001f && m_onValueChanged) {
        m_onValueChanged(m_value);
    }
    if (!m_editing) {
        m_textEditor.setValue(formattedValue());
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
    if (!m_editing) {
        m_textEditor.setValue(formattedValue());
    }
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

float NumberInput::normalizedValue() const
{
    if (m_maxValue <= m_minValue) {
        return 0.0f;
    }

    return std::clamp((m_value - m_minValue) / (m_maxValue - m_minValue), 0.0f, 1.0f);
}

std::string NumberInput::formattedValue() const
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(m_precision) << m_value;
    return stream.str();
}

bool NumberInput::editing() const
{
    return m_editing;
}

const TextInput& NumberInput::textEditor() const
{
    return m_textEditor;
}

} // namespace Engine
