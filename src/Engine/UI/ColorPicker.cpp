#include "Engine/UI/ColorPicker.hpp"

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/UI/UIStyle.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include <glm/common.hpp>

namespace Engine {

namespace {

constexpr float ButtonHeight = 28.0f;
constexpr glm::vec2 PopupSize{228.0f, 278.0f};
constexpr glm::vec2 SaturationValueOffset{12.0f, 12.0f};
constexpr glm::vec2 SaturationValueSize{174.0f, 150.0f};
constexpr glm::vec2 HueOffset{194.0f, 12.0f};
constexpr glm::vec2 HueSize{22.0f, 150.0f};
constexpr glm::vec2 OldPreviewOffset{12.0f, 176.0f};
constexpr glm::vec2 CurrentPreviewOffset{116.0f, 176.0f};
constexpr glm::vec2 PreviewSize{100.0f, 18.0f};
constexpr float ChannelTrackY = 204.0f;
constexpr float ChannelTrackHeight = 14.0f;
constexpr float ChannelTrackSpacing = 20.0f;
constexpr int SaturationValueSteps = 16;
constexpr int HueSteps = 18;

glm::vec4 hsvToRgb(const float hue, const float saturation, const float value)
{
    const float wrappedHue = hue - std::floor(hue);
    const float scaledHue = wrappedHue * 6.0f;
    const int sector = static_cast<int>(std::floor(scaledHue));
    const float fraction = scaledHue - static_cast<float>(sector);
    const float p = value * (1.0f - saturation);
    const float q = value * (1.0f - fraction * saturation);
    const float t = value * (1.0f - (1.0f - fraction) * saturation);

    switch (sector % 6) {
    case 0: return {value, t, p, 1.0f};
    case 1: return {q, value, p, 1.0f};
    case 2: return {p, value, t, 1.0f};
    case 3: return {p, q, value, 1.0f};
    case 4: return {t, p, value, 1.0f};
    default: return {value, p, q, 1.0f};
    }
}

bool contains(const glm::vec2& point, const glm::vec2& position, const glm::vec2& size)
{
    return point.x >= position.x && point.y >= position.y &&
        point.x <= position.x + size.x && point.y <= position.y + size.y;
}

} // namespace

ColorPicker::ColorPicker(std::string id, const glm::vec4& color)
    : Widget(std::move(id))
{
    setColor(color);
}

void ColorPicker::update(UIContext& context)
{
    if (!m_visible) return;
    m_interaction = context.interact(m_id + ".button", m_position, {m_size.x, ButtonHeight});
    if (m_interaction.pressed) {
        m_popupOpen = !m_popupOpen;
        if (m_popupOpen) {
            m_originalColor = m_color;
        }
    }
}

void ColorPicker::updatePopup(UIContext& context)
{
    if (!m_visible || !m_popupOpen) return;

    const glm::vec2 popup = popupPosition();
    if (context.primaryMousePressed() &&
        !contains(context.mousePosition(), m_position, {m_size.x, ButtonHeight}) &&
        !contains(context.mousePosition(), popup, PopupSize)) {
        m_popupOpen = false;
        return;
    }

    updateSaturationValue(context);
    updateHue(context);
    if (context.interact(m_id + ".old-preview", popup + OldPreviewOffset, PreviewSize).pressed) {
        setColor(m_originalColor);
    }
    updateChannel(context, ".red", ChannelTrackY, m_color.r);
    updateChannel(context, ".green", ChannelTrackY + ChannelTrackSpacing, m_color.g);
    updateChannel(context, ".blue", ChannelTrackY + ChannelTrackSpacing * 2.0f, m_color.b);
}

void ColorPicker::render(Renderer2D& renderer2D, const UIStyle& style) const
{
    if (!m_visible) return;

    const glm::vec4 border = style.field.border;
    renderer2D.drawSdfRect(m_position, {m_size.x, ButtonHeight}, 4.0f, style.field.fill, border, 1.0f);
    renderer2D.drawSdfRect(
        m_position + glm::vec2{4.0f},
        {std::max(m_size.x - 8.0f, 0.0f), ButtonHeight - 8.0f},
        3.0f,
        m_color,
        border,
        1.0f);
}

void ColorPicker::renderPopup(Renderer2D& renderer2D, const UIStyle& style) const
{
    if (!m_visible || !m_popupOpen) return;

    const glm::vec2 popup = popupPosition();
    // Keep the popup shell in the quad batch so it records before the palette
    // quads. SDF rectangles are recorded as a later batch by VulkanRenderer2D.
    renderer2D.drawQuad(popup, PopupSize, style.panel.fill);
    renderer2D.drawRect(popup, PopupSize, style.panel.border, 1.0f);

    const glm::vec2 svPosition = popup + SaturationValueOffset;
    const glm::vec2 cellSize = SaturationValueSize / static_cast<float>(SaturationValueSteps);
    for (int y = 0; y < SaturationValueSteps; ++y) {
        const float value = 1.0f - static_cast<float>(y) / static_cast<float>(SaturationValueSteps - 1);
        for (int x = 0; x < SaturationValueSteps; ++x) {
            const float saturation = static_cast<float>(x) / static_cast<float>(SaturationValueSteps - 1);
            renderer2D.drawQuad(svPosition + glm::vec2{cellSize.x * x, cellSize.y * y}, cellSize + glm::vec2{0.5f}, hsvToRgb(m_hue, saturation, value));
        }
    }

    const float hueCellHeight = HueSize.y / static_cast<float>(HueSteps);
    for (int index = 0; index < HueSteps; ++index) {
        renderer2D.drawQuad(
            popup + HueOffset + glm::vec2{0.0f, hueCellHeight * index},
            {HueSize.x, hueCellHeight + 0.5f},
            hsvToRgb(static_cast<float>(index) / static_cast<float>(HueSteps), 1.0f, 1.0f));
    }

    renderer2D.drawRect(
        svPosition + glm::vec2{m_saturation * SaturationValueSize.x - 4.0f, (1.0f - m_value) * SaturationValueSize.y - 4.0f},
        {8.0f, 8.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
        2.0f);
    renderer2D.drawRect(
        popup + HueOffset + glm::vec2{-2.0f, m_hue * HueSize.y - 2.0f},
        {HueSize.x + 4.0f, 4.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
        1.0f);
    renderer2D.drawQuad(popup + OldPreviewOffset, PreviewSize, m_originalColor);
    renderer2D.drawRect(popup + OldPreviewOffset, PreviewSize, style.field.border, 1.0f);
    renderer2D.drawQuad(popup + CurrentPreviewOffset, PreviewSize, m_color);
    renderer2D.drawRect(popup + CurrentPreviewOffset, PreviewSize, style.field.border, 1.0f);

    const glm::vec4 channels[] = {
        {0.86f, 0.22f, 0.24f, 1.0f},
        {0.24f, 0.72f, 0.38f, 1.0f},
        {0.28f, 0.48f, 0.88f, 1.0f},
    };
    const float values[] = {m_color.r, m_color.g, m_color.b};
    for (int index = 0; index < 3; ++index) {
        const glm::vec2 trackPosition = popup + glm::vec2{12.0f, ChannelTrackY + ChannelTrackSpacing * index};
        renderer2D.drawSdfRect(trackPosition, {204.0f, ChannelTrackHeight}, 3.0f, style.field.fill, style.field.border, 1.0f);
        renderer2D.drawSdfRect(trackPosition, {204.0f * values[index], ChannelTrackHeight}, 3.0f, channels[index], channels[index], 0.0f);
    }
}

void ColorPicker::setColor(const glm::vec4& color)
{
    const glm::vec4 clampedColor = glm::clamp(color, glm::vec4{0.0f}, glm::vec4{1.0f});
    if (clampedColor == m_color) return;
    m_color = clampedColor;
    updateHsvFromColor();
    notifyColorChanged();
}

void ColorPicker::setOnColorChanged(std::function<void(const glm::vec4&)> callback)
{
    m_onColorChanged = std::move(callback);
}

const glm::vec4& ColorPicker::color() const
{
    return m_color;
}

bool ColorPicker::popupOpen() const
{
    return m_popupOpen;
}

void ColorPicker::updateSaturationValue(UIContext& context)
{
    const glm::vec2 position = popupPosition() + SaturationValueOffset;
    if (!context.interact(m_id + ".saturation-value", position, SaturationValueSize).held) return;
    m_saturation = std::clamp((context.mousePosition().x - position.x) / SaturationValueSize.x, 0.0f, 1.0f);
    m_value = 1.0f - std::clamp((context.mousePosition().y - position.y) / SaturationValueSize.y, 0.0f, 1.0f);
    const float alpha = m_color.a;
    m_color = hsvToRgb(m_hue, m_saturation, m_value);
    m_color.a = alpha;
    notifyColorChanged();
}

void ColorPicker::updateHue(UIContext& context)
{
    const glm::vec2 position = popupPosition() + HueOffset;
    if (!context.interact(m_id + ".hue", position, HueSize).held) return;
    m_hue = std::clamp((context.mousePosition().y - position.y) / HueSize.y, 0.0f, 1.0f);
    const float alpha = m_color.a;
    m_color = hsvToRgb(m_hue, m_saturation, m_value);
    m_color.a = alpha;
    notifyColorChanged();
}

void ColorPicker::updateChannel(UIContext& context, const std::string_view suffix, const float y, float& value)
{
    const glm::vec2 position = popupPosition() + glm::vec2{12.0f, y};
    if (!context.interact(m_id + std::string{suffix}, position, {204.0f, ChannelTrackHeight}).held) return;
    value = std::clamp((context.mousePosition().x - position.x) / 204.0f, 0.0f, 1.0f);
    updateHsvFromColor();
    notifyColorChanged();
}

void ColorPicker::notifyColorChanged()
{
    if (m_onColorChanged) m_onColorChanged(m_color);
}

void ColorPicker::updateHsvFromColor()
{
    const float maximum = std::max({m_color.r, m_color.g, m_color.b});
    const float minimum = std::min({m_color.r, m_color.g, m_color.b});
    const float delta = maximum - minimum;
    m_value = maximum;
    m_saturation = maximum <= 0.0f ? 0.0f : delta / maximum;
    if (delta <= 0.0f) return;
    if (maximum == m_color.r) m_hue = (m_color.g - m_color.b) / delta;
    else if (maximum == m_color.g) m_hue = 2.0f + (m_color.b - m_color.r) / delta;
    else m_hue = 4.0f + (m_color.r - m_color.g) / delta;
    m_hue = (m_hue / 6.0f) - std::floor(m_hue / 6.0f);
}

glm::vec2 ColorPicker::popupPosition() const
{
    const float left = m_position.x - PopupSize.x - 8.0f;
    return {left >= 8.0f ? left : m_position.x + m_size.x + 8.0f, m_position.y + ButtonHeight + 6.0f};
}

} // namespace Engine
