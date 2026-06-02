#include "Engine/UI/ColorPicker.hpp"

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/Renderer/TextRenderer.hpp"
#include "Engine/UI/UIFrame.hpp"
#include "Engine/UI/UIStyle.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

#include <glm/common.hpp>

namespace Engine {

namespace {

constexpr float ButtonHeight = 28.0f;
constexpr glm::vec2 PopupSize{228.0f, 310.0f};
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
constexpr float ChannelTrackWidth = 204.0f;
constexpr glm::vec2 HexInputOffset{12.0f, 276.0f};
constexpr glm::vec2 HexInputSize{204.0f, 22.0f};
constexpr int SaturationValueSteps = 24;
constexpr int HueSteps = 24;
constexpr int ChannelGradientSteps = 24;

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

std::string colorToHex(const glm::vec4& color)
{
    std::ostringstream stream;
    stream << '#' << std::uppercase << std::hex << std::setfill('0')
           << std::setw(2) << static_cast<int>(std::round(color.r * 255.0f))
           << std::setw(2) << static_cast<int>(std::round(color.g * 255.0f))
           << std::setw(2) << static_cast<int>(std::round(color.b * 255.0f));
    return stream.str();
}

bool parseHexColor(const std::string_view text, glm::vec4& color)
{
    const std::string value{text.starts_with('#') ? text.substr(1) : text};
    if (value.size() != 6) return false;
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 16);
    if (end == nullptr || *end != '\0') return false;
    color.r = static_cast<float>((parsed >> 16U) & 0xFFU) / 255.0f;
    color.g = static_cast<float>((parsed >> 8U) & 0xFFU) / 255.0f;
    color.b = static_cast<float>(parsed & 0xFFU) / 255.0f;
    return true;
}

} // namespace

ColorPicker::ColorPicker(std::string id, const glm::vec4& color)
    : Widget(std::move(id))
    , m_redInput(m_id + ".red-input", color.r * 255.0f, 0.0f, 255.0f)
    , m_greenInput(m_id + ".green-input", color.g * 255.0f, 0.0f, 255.0f)
    , m_blueInput(m_id + ".blue-input", color.b * 255.0f, 0.0f, 255.0f)
    , m_hexInput(m_id + ".hex-input", colorToHex(color))
{
    m_redInput.setPrecision(0);
    m_greenInput.setPrecision(0);
    m_blueInput.setPrecision(0);
    m_redInput.setSensitivity(255.0f / ChannelTrackWidth);
    m_greenInput.setSensitivity(255.0f / ChannelTrackWidth);
    m_blueInput.setSensitivity(255.0f / ChannelTrackWidth);
    m_redInput.setOnValueChanged([this](const float value) { if (!m_syncingInputs) { m_color.r = value / 255.0f; updateHsvFromColor(); syncHexInput(); notifyColorChanged(); } });
    m_greenInput.setOnValueChanged([this](const float value) { if (!m_syncingInputs) { m_color.g = value / 255.0f; updateHsvFromColor(); syncHexInput(); notifyColorChanged(); } });
    m_blueInput.setOnValueChanged([this](const float value) { if (!m_syncingInputs) { m_color.b = value / 255.0f; updateHsvFromColor(); syncHexInput(); notifyColorChanged(); } });
    m_hexInput.setOnValueChanged([this](const std::string_view value) {
        glm::vec4 color = m_color;
        if (parseHexColor(value, color)) {
            m_color = color;
            updateHsvFromColor();
            syncChannelInputs();
            notifyColorChanged();
        }
    });
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

void ColorPicker::updatePopup(UIContext& context, const TextRenderer& textRenderer, const UIStyle& style)
{
    updatePopup(context);
    if (!m_visible || !m_popupOpen) return;
    layoutChannelInputs();
    const UITextStyle& textStyle = style.resolveText("input-value");
    m_redInput.update(context, textRenderer, style, textStyle.font, textStyle.scale);
    m_greenInput.update(context, textRenderer, style, textStyle.font, textStyle.scale);
    m_blueInput.update(context, textRenderer, style, textStyle.font, textStyle.scale);
    m_hexInput.update(context, textRenderer, style, textStyle.font, textStyle.scale);
}

void ColorPicker::render(Renderer2D& renderer2D, const UIStyle& style) const
{
    if (!m_visible) return;

    const glm::vec4 border = style.field.border;
    renderer2D.drawSdfRect(m_position, {m_size.x, ButtonHeight}, 4.0f, style.field.fill, border, 1.0f);
    const glm::vec2 swatchPosition = m_position + glm::vec2{4.0f};
    const glm::vec2 swatchSize{std::max(m_size.x - 8.0f, 0.0f), ButtonHeight - 8.0f};
    renderer2D.drawSdfRect(swatchPosition, swatchSize, 3.0f, m_color, border, 1.0f);
}

void ColorPicker::renderPopup(UIContext& context, Renderer2D& renderer2D, TextRenderer& textRenderer, const UIStyle& style) const
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

    const float values[] = {m_color.r, m_color.g, m_color.b};
    for (int index = 0; index < 3; ++index) {
        const glm::vec2 trackPosition = popup + glm::vec2{12.0f, ChannelTrackY + ChannelTrackSpacing * index};
        const float segmentWidth = ChannelTrackWidth / static_cast<float>(ChannelGradientSteps);
        for (int step = 0; step < ChannelGradientSteps; ++step) {
            glm::vec4 segmentColor = m_color;
            segmentColor[index] = static_cast<float>(step) / static_cast<float>(ChannelGradientSteps - 1);
            renderer2D.drawQuad(trackPosition + glm::vec2{segmentWidth * step, 0.0f}, {segmentWidth + 0.5f, ChannelTrackHeight}, segmentColor);
        }
        renderer2D.drawRect(trackPosition, {ChannelTrackWidth, ChannelTrackHeight}, style.field.border, 1.0f);
        renderer2D.drawRect(trackPosition + glm::vec2{ChannelTrackWidth * values[index] - 2.0f, -2.0f}, {4.0f, ChannelTrackHeight + 4.0f}, glm::vec4{1.0f}, 1.0f);
    }
    UIFrame frame{context, renderer2D, textRenderer, style};
    m_redInput.renderTextOnly(frame);
    m_greenInput.renderTextOnly(frame);
    m_blueInput.renderTextOnly(frame);
    m_hexInput.render(frame);
}

void ColorPicker::setColor(const glm::vec4& color)
{
    const glm::vec4 clampedColor = glm::clamp(color, glm::vec4{0.0f}, glm::vec4{1.0f});
    if (clampedColor == m_color) return;
    m_color = clampedColor;
    updateHsvFromColor();
    syncChannelInputs();
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
    syncChannelInputs();
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
    syncChannelInputs();
    notifyColorChanged();
}

void ColorPicker::updateChannel(UIContext& context, const std::string_view suffix, const float y, float& value)
{
    const glm::vec2 position = popupPosition() + glm::vec2{12.0f, y};
    if (!context.interact(m_id + std::string{suffix}, position, {ChannelTrackWidth, ChannelTrackHeight}).held) return;
    value = std::clamp((context.mousePosition().x - position.x) / ChannelTrackWidth, 0.0f, 1.0f);
    updateHsvFromColor();
    syncChannelInputs();
    notifyColorChanged();
}

void ColorPicker::layoutChannelInputs()
{
    const glm::vec2 popup = popupPosition();
    m_redInput.setBounds(popup + glm::vec2{12.0f, ChannelTrackY - 4.0f}, {ChannelTrackWidth, 22.0f});
    m_greenInput.setBounds(popup + glm::vec2{12.0f, ChannelTrackY + ChannelTrackSpacing - 4.0f}, {ChannelTrackWidth, 22.0f});
    m_blueInput.setBounds(popup + glm::vec2{12.0f, ChannelTrackY + ChannelTrackSpacing * 2.0f - 4.0f}, {ChannelTrackWidth, 22.0f});
    m_hexInput.setBounds(popup + HexInputOffset, HexInputSize);
}

void ColorPicker::syncChannelInputs()
{
    m_syncingInputs = true;
    m_redInput.setValue(m_color.r * 255.0f);
    m_greenInput.setValue(m_color.g * 255.0f);
    m_blueInput.setValue(m_color.b * 255.0f);
    m_syncingInputs = false;
    syncHexInput();
}

void ColorPicker::syncHexInput()
{
    if (!m_hexInput.focused()) {
        m_hexInput.setValue(colorToHex(m_color));
    }
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
