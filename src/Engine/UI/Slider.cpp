#include "Engine/UI/Slider.hpp"

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/Renderer/TextRenderer.hpp"
#include "Engine/UI/UIStyle.hpp"
#include "Engine/UI/UIFrame.hpp"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

namespace Engine {

Slider::Slider(std::string id, const float value, const float minValue, const float maxValue)
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

// Updates the slider value while the user drags the track.
void Slider::update(UIContext& context)
{
    updateDragging(context);
}

void Slider::update(
    UIContext& context,
    const TextRenderer& textRenderer,
    const UIStyle& style,
    const std::string_view fontName,
    const float scale)
{
    m_textEditor.setBounds(m_position, m_size);
    if (!m_editing) {
        updateDragging(context);
        if (m_interaction.pressed && !m_dragged) {
            beginEditing(context);
        }
    }

    if (m_editing) {
        const UISliderStyle& sliderStyle = m_styleOverride ? *m_styleOverride : style.resolveSlider(m_styleClass, m_id);
        UITextInputStyle textInputStyle;
        textInputStyle.box = sliderStyle.track;
        textInputStyle.hovered = sliderStyle.hovered;
        textInputStyle.focused = sliderStyle.track.fill;
        textInputStyle.focusedBorder = sliderStyle.track.border;
        m_textEditor.setStyle(textInputStyle);
        m_textEditor.update(context, textRenderer, style, fontName, scale);
        m_interaction = m_textEditor.interaction();
        if (!m_textEditor.focused()) {
            m_editing = false;
            m_textEditor.setValue(formattedValue());
        }
    }
}

void Slider::updateDragging(UIContext& context)
{
    Widget::update(context);
    if (m_interaction.held && !m_wasHeld) {
        m_dragStartValue = m_value;
        m_dragStartMouseX = context.mousePosition().x;
        m_dragged = false;
    }
    if (!m_interaction.held || m_maxValue <= m_minValue) {
        m_wasHeld = m_interaction.held;
        return;
    }

    m_dragged = m_dragged || std::abs(context.mousePosition().x - m_dragStartMouseX) >= 3.0f;
    if (m_dragged) {
        const float t = std::clamp((context.mousePosition().x - m_position.x) / std::max(m_size.x, 1.0f), 0.0f, 1.0f);
        setValue(m_minValue + (m_maxValue - m_minValue) * t);
    }
    m_wasHeld = m_interaction.held;
}

void Slider::beginEditing(UIContext& context)
{
    m_editing = true;
    m_textEditor.setValue(formattedValue());
    m_textEditor.selectAll();
    context.focus(m_textEditor.id());
}

// Draws the slider track, fill, and knob.
void Slider::render(Renderer2D& renderer2D, const UIStyle& style) const
{
    if (!m_visible) {
        return;
    }

    const UISliderStyle& sliderStyle = m_styleOverride ? *m_styleOverride : style.resolveSlider(m_styleClass, m_id);
    const float t = normalizedValue();
    const glm::vec4 track = m_interaction.hovered || m_interaction.held ? sliderStyle.hovered : sliderStyle.track.fill;
    renderer2D.drawSdfRect(
        m_position,
        m_size,
        sliderStyle.track.borderRadius,
        track,
        sliderStyle.track.border,
        sliderStyle.track.borderWidth);
    renderer2D.drawSdfRect(
        m_position,
        {m_size.x * t, m_size.y},
        sliderStyle.track.borderRadius,
        sliderStyle.fill,
        sliderStyle.fill,
        0.0f);

    constexpr float knobWidth = 8.0f;
    const float knobX = m_position.x + std::max(0.0f, m_size.x * t - knobWidth * 0.5f);
    renderer2D.drawSdfRect({knobX, m_position.y - 2.0f}, {knobWidth, m_size.y + 4.0f}, 4.0f, sliderStyle.knob, sliderStyle.knob, 0.0f);
}

void Slider::render(const UIFrame& frame) const
{
    if (!m_visible) return;
    const UISliderStyle& sliderStyle = m_styleOverride ? *m_styleOverride : frame.style().resolveSlider(m_styleClass, m_id);
    const float t = normalizedValue();
    const glm::vec4 track = m_interaction.hovered || m_interaction.held ? sliderStyle.hovered : sliderStyle.track.fill;
    frame.shapes().drawSdfRect(frame.toScreen(m_position), m_size, sliderStyle.track.borderRadius, track, sliderStyle.track.border, sliderStyle.track.borderWidth);
    frame.shapes().drawSdfRect(frame.toScreen(m_position), {m_size.x * t, m_size.y}, sliderStyle.track.borderRadius, sliderStyle.fill, sliderStyle.fill, 0.0f);
    constexpr float knobWidth = 8.0f;
    frame.shapes().drawSdfRect(frame.toScreen({m_position.x + std::max(0.0f, m_size.x * t - knobWidth * 0.5f), m_position.y - 2.0f}), {knobWidth, m_size.y + 4.0f}, 4.0f, sliderStyle.knob, sliderStyle.knob, 0.0f);
    if (m_editing) {
        const UITextStyle& textStyle = frame.style().resolveText("control-value");
        m_textEditor.renderText(
            frame.text(),
            frame.style(),
            textStyle.font,
            textStyle.scale,
            frame.surface().origin,
            &textStyle);
    } else {
        frame.drawText(
            formattedValue(),
            {m_position, m_size},
            frame.style().resolveText("control-value"));
    }
}

// Sets the slider value and fires the change callback when it meaningfully changes.
void Slider::setValue(const float value)
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

// Changes the valid slider range and reclamps the current value.
void Slider::setRange(const float minValue, const float maxValue)
{
    m_minValue = minValue;
    m_maxValue = maxValue;
    setValue(m_value);
}

void Slider::setPrecision(const int precision)
{
    m_precision = std::clamp(precision, 0, 6);
    if (!m_editing) {
        m_textEditor.setValue(formattedValue());
    }
}

// Overrides the resolved slider style for this specific slider instance.
void Slider::setStyle(const UISliderStyle& style)
{
    m_styleOverride = style;
}

// Removes the local style override so class/default style resolution is used.
void Slider::clearStyleOverride()
{
    m_styleOverride.reset();
}

// Registers work to perform when the slider value changes.
void Slider::setOnValueChanged(std::function<void(float)> callback)
{
    m_onValueChanged = std::move(callback);
}

// Returns the current slider value.
float Slider::value() const
{
    return m_value;
}

// Returns the slider value normalized to 0..1.
float Slider::normalizedValue() const
{
    if (m_maxValue <= m_minValue) {
        return 0.0f;
    }

    return std::clamp((m_value - m_minValue) / (m_maxValue - m_minValue), 0.0f, 1.0f);
}

std::string Slider::formattedValue() const
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(m_precision) << m_value;
    return stream.str();
}

bool Slider::editing() const
{
    return m_editing;
}

} // namespace Engine
