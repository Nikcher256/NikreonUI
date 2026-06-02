#include "Engine/UI/ColorPicker.hpp"

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/UI/UIStyle.hpp"

#include <algorithm>
#include <utility>

#include <glm/common.hpp>

namespace Engine {

namespace {

constexpr float PreviewWidth = 34.0f;
constexpr float TrackGap = 6.0f;
constexpr float TrackHeight = 14.0f;
constexpr float TrackSpacing = 20.0f;

} // namespace

ColorPicker::ColorPicker(std::string id, const glm::vec4& color)
    : Widget(std::move(id))
{
    setColor(color);
}

void ColorPicker::update(UIContext& context)
{
    if (!m_visible) {
        return;
    }

    Widget::update(context);
    const float trackY = m_position.y + 2.0f;
    updateChannel(context, ".red", trackY, m_color.r);
    updateChannel(context, ".green", trackY + TrackSpacing, m_color.g);
    updateChannel(context, ".blue", trackY + TrackSpacing * 2.0f, m_color.b);
}

void ColorPicker::render(Renderer2D& renderer2D, const UIStyle& style) const
{
    if (!m_visible) {
        return;
    }

    const float trackX = m_position.x + PreviewWidth + TrackGap;
    const float trackWidth = std::max(m_size.x - PreviewWidth - TrackGap, 0.0f);
    const glm::vec4 background = style.field.fill;
    const glm::vec4 border = style.field.border;
    const glm::vec4 channels[] = {
        {0.86f, 0.22f, 0.24f, 1.0f},
        {0.24f, 0.72f, 0.38f, 1.0f},
        {0.28f, 0.48f, 0.88f, 1.0f},
    };
    const float values[] = {m_color.r, m_color.g, m_color.b};

    renderer2D.drawSdfRect(m_position, {PreviewWidth, TrackSpacing * 3.0f - 4.0f}, 4.0f, m_color, border, 1.0f);
    for (int index = 0; index < 3; ++index) {
        const glm::vec2 trackPosition{trackX, m_position.y + 2.0f + TrackSpacing * static_cast<float>(index)};
        renderer2D.drawSdfRect(trackPosition, {trackWidth, TrackHeight}, 3.0f, background, border, 1.0f);
        renderer2D.drawSdfRect(trackPosition, {trackWidth * values[index], TrackHeight}, 3.0f, channels[index], channels[index], 0.0f);
    }
}

void ColorPicker::setColor(const glm::vec4& color)
{
    const glm::vec4 clampedColor = glm::clamp(color, glm::vec4{0.0f}, glm::vec4{1.0f});
    if (clampedColor == m_color) {
        return;
    }

    m_color = clampedColor;
    if (m_onColorChanged) {
        m_onColorChanged(m_color);
    }
}

void ColorPicker::setOnColorChanged(std::function<void(const glm::vec4&)> callback)
{
    m_onColorChanged = std::move(callback);
}

const glm::vec4& ColorPicker::color() const
{
    return m_color;
}

void ColorPicker::updateChannel(UIContext& context, const std::string_view suffix, const float y, float& value)
{
    const float trackX = m_position.x + PreviewWidth + TrackGap;
    const float trackWidth = std::max(m_size.x - PreviewWidth - TrackGap, 0.0f);
    const std::string channelId = m_id + std::string{suffix};
    const UIInteraction interaction = context.interact(channelId, {trackX, y}, {trackWidth, TrackHeight});
    if (!interaction.held || trackWidth <= 0.0f) {
        return;
    }

    const float newValue = std::clamp((context.mousePosition().x - trackX) / trackWidth, 0.0f, 1.0f);
    if (newValue == value) {
        return;
    }

    value = newValue;
    if (m_onColorChanged) {
        m_onColorChanged(m_color);
    }
}

} // namespace Engine
