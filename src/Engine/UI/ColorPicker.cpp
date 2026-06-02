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
constexpr glm::vec2 PopupSize{228.0f, 340.0f};
constexpr glm::vec2 SaturationValueOffset{12.0f, 12.0f};
constexpr glm::vec2 SaturationValueSize{174.0f, 150.0f};
constexpr glm::vec2 HueOffset{194.0f, 12.0f};
constexpr glm::vec2 HueSize{22.0f, 150.0f};
constexpr glm::vec2 OldPreviewOffset{12.0f, 176.0f};
constexpr glm::vec2 CurrentPreviewOffset{116.0f, 176.0f};
constexpr glm::vec2 PreviewSize{100.0f, 18.0f};
constexpr float ChannelTrackY = 204.0f;
constexpr float ChannelTrackHeight = 14.0f;
constexpr float ChannelTrackSpacing = 34.0f;
constexpr float ChannelTrackWidth = 204.0f;
constexpr glm::vec2 HexInputOffset{12.0f, 306.0f};
constexpr glm::vec2 HexInputSize{204.0f, 24.0f};
constexpr int HueGradientSegments = 6;

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
    , m_redSlider(m_id + ".red-slider", color.r * 255.0f, 0.0f, 255.0f)
    , m_greenSlider(m_id + ".green-slider", color.g * 255.0f, 0.0f, 255.0f)
    , m_blueSlider(m_id + ".blue-slider", color.b * 255.0f, 0.0f, 255.0f)
    , m_hexInput(m_id + ".hex-input", colorToHex(color))
{
    m_redSlider.setStyleClass("color-red");
    m_greenSlider.setStyleClass("color-green");
    m_blueSlider.setStyleClass("color-blue");
    m_redSlider.setPrecision(0);
    m_greenSlider.setPrecision(0);
    m_blueSlider.setPrecision(0);
    m_redSlider.setOnValueChanged([this](const float value) {
    if (!m_syncingInputs) {
        m_color.r = std::clamp(value / 255.0f, 0.0f, 1.0f);
        updateHsvFromColor();
        syncHexInput();
        notifyColorChanged();
    }
    });

    m_greenSlider.setOnValueChanged([this](const float value) {
        if (!m_syncingInputs) {
            m_color.g = std::clamp(value / 255.0f, 0.0f, 1.0f);
            updateHsvFromColor();
            syncHexInput();
            notifyColorChanged();
        }
    });

    m_blueSlider.setOnValueChanged([this](const float value) {
        if (!m_syncingInputs) {
            m_color.b = std::clamp(value / 255.0f, 0.0f, 1.0f);
            updateHsvFromColor();
            syncHexInput();
            notifyColorChanged();
        }
    });
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
        !contains(context.mousePosition(), m_position, m_size) &&
        !contains(context.mousePosition(), popup, PopupSize)) {
        m_popupOpen = false;
        return;
    }

    updateSaturationValue(context);
    updateHue(context);
    if (context.interact(m_id + ".old-preview", popup + OldPreviewOffset, PreviewSize).pressed) {
        setColor(m_originalColor);
    }
}

void ColorPicker::updatePopup(UIContext& context, const TextRenderer& textRenderer, const UIStyle& style)
{
    updatePopup(context);
    if (!m_visible || !m_popupOpen) return;
    layoutChannelInputs();
    const UITextStyle& textStyle = style.resolveText("input-value");
    m_redSlider.update(context, textRenderer, style, textStyle.font, textStyle.scale);
    m_greenSlider.update(context, textRenderer, style, textStyle.font, textStyle.scale);
    m_blueSlider.update(context, textRenderer, style, textStyle.font, textStyle.scale); 
    m_hexInput.update(context, textRenderer, style, textStyle.font, textStyle.scale);
}

void ColorPicker::render(Renderer2D& renderer2D, const UIStyle& style) const
{
    if (!m_visible) {
        return;
    }

    glm::vec4 swatchColor = m_color;
    swatchColor.a = 1.0f; // important, in case clear color alpha is 0

    const glm::vec4 border = m_interaction.hovered
        ? glm::vec4{0.62f, 0.78f, 1.0f, 1.0f}
        : style.field.border;

    renderer2D.drawSdfRect(
        m_position,
        m_size,
        4.0f,
        swatchColor, // fill color
        border,      // border color
        1.0f);
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
    const glm::vec4 hueColor = hsvToRgb(m_hue, 1.0f, 1.0f);

    // Horizontal gradient: white -> selected hue.
    renderer2D.drawGradientQuad(
        svPosition,
        SaturationValueSize,
        {1.0f, 1.0f, 1.0f, 1.0f},
        hueColor,
        hueColor,
        {1.0f, 1.0f, 1.0f, 1.0f});

    // Vertical black overlay: transparent top -> black bottom.
    renderer2D.drawGradientQuad(
        svPosition,
        SaturationValueSize,
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 0.0f, 1.0f});

    renderer2D.drawRect(svPosition, SaturationValueSize, style.field.border, 1.0f);

    for (int index = 0; index < HueGradientSegments; ++index) {
        const float t0 = static_cast<float>(index) / static_cast<float>(HueGradientSegments);
        const float t1 = static_cast<float>(index + 1) / static_cast<float>(HueGradientSegments);

        const glm::vec4 c0 = hsvToRgb(t0, 1.0f, 1.0f);
        const glm::vec4 c1 = hsvToRgb(t1, 1.0f, 1.0f);

        const float y0 = HueSize.y * t0;
        const float y1 = HueSize.y * t1;

        renderer2D.drawGradientQuad(
            popup + HueOffset + glm::vec2{0.0f, y0},
            {HueSize.x, y1 - y0},
            c0,
            c0,
            c1,
            c1);
    }

    renderer2D.drawRect(popup + HueOffset, HueSize, style.field.border, 1.0f);

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

    UIFrame frame{context, renderer2D, textRenderer, style};
    m_redSlider.render(frame);
    m_greenSlider.render(frame);
    m_blueSlider.render(frame);
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

    m_redSlider.setBounds(
        popup + glm::vec2{12.0f, ChannelTrackY},
        {ChannelTrackWidth, 24.0f});

    m_greenSlider.setBounds(
        popup + glm::vec2{12.0f, ChannelTrackY + ChannelTrackSpacing},
        {ChannelTrackWidth, 24.0f});

    m_blueSlider.setBounds(
        popup + glm::vec2{12.0f, ChannelTrackY + ChannelTrackSpacing * 2.0f},
        {ChannelTrackWidth, 24.0f});

    m_hexInput.setBounds(popup + HexInputOffset, HexInputSize);
}

void ColorPicker::syncChannelInputs()
{
    m_syncingInputs = true;
    m_redSlider.setValue(m_color.r * 255.0f);
    m_greenSlider.setValue(m_color.g * 255.0f);
    m_blueSlider.setValue(m_color.b * 255.0f);
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
