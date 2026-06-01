#include "Engine/UI/Slider.hpp"

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/UI/UIStyle.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Engine {

Slider::Slider(std::string id, const float value, const float minValue, const float maxValue)
    : Widget(std::move(id))
    , m_value(value)
    , m_minValue(minValue)
    , m_maxValue(maxValue)
{
    setValue(value);
}

// Updates the slider value while the user drags the track.
void Slider::update(UIContext& context)
{
    Widget::update(context);
    if (!m_interaction.held || m_maxValue <= m_minValue) {
        return;
    }

    const float t = std::clamp((context.mousePosition().x - m_position.x) / std::max(m_size.x, 1.0f), 0.0f, 1.0f);
    setValue(m_minValue + (m_maxValue - m_minValue) * t);
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

// Sets the slider value and fires the change callback when it meaningfully changes.
void Slider::setValue(const float value)
{
    const float oldValue = m_value;
    m_value = std::clamp(value, m_minValue, m_maxValue);
    if (std::abs(oldValue - m_value) > 0.0001f && m_onValueChanged) {
        m_onValueChanged(m_value);
    }
}

// Changes the valid slider range and reclamps the current value.
void Slider::setRange(const float minValue, const float maxValue)
{
    m_minValue = minValue;
    m_maxValue = maxValue;
    setValue(m_value);
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

} // namespace Engine
