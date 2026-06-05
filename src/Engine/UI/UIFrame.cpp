#include "Engine/UI/UIFrame.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

#include "Engine/UI/UIContext.hpp"

namespace Engine {

UIFrame::UIFrame(UIContext& input, Renderer2D& shapes, TextRenderer& text, const UIStyle& style, const UISurface surface)
    : m_input(&input), m_shapes(&shapes), m_text(&text), m_style(&style), m_surface(surface)
{
}

UIContext& UIFrame::input() const { return *m_input; }
Renderer2D& UIFrame::shapes() const { return *m_shapes; }
TextRenderer& UIFrame::text() const { return *m_text; }
const UIStyle& UIFrame::style() const { return *m_style; }
const UISurface& UIFrame::surface() const { return m_surface; }
glm::vec2 UIFrame::toScreen(const glm::vec2& localPosition) const { return m_surface.origin + localPosition; }
UIClipRect UIFrame::toScreen(const UIClipRect& localRect) const { return {toScreen(localRect.position), localRect.size}; }

void UIFrame::drawBox(const UIClipRect& bounds, const UIBoxStyle& box) const
{
    m_shapes->drawSdfRect(toScreen(bounds.position), bounds.size, box.borderRadius, box.fill, box.border, box.borderWidth);
}

namespace {

float lineHeight(TextRenderer& renderer, const std::string& font, const float scale)
{
    return std::max(renderer.measureText("Mg", font, scale).y, 1.0f);
}

std::string ellipsize(
    TextRenderer& renderer,
    const std::string_view value,
    const std::string& font,
    const float scale,
    const float width,
    bool& truncated)
{
    std::string displayText{value};
    truncated = false;
    if (renderer.measureText(displayText, font, scale).x <= width) {
        return displayText;
    }

    constexpr std::string_view ellipsis = "...";
    displayText.assign(ellipsis);
    if (renderer.measureText(displayText, font, scale).x > width) {
        truncated = true;
        return {};
    }

    std::string candidate{value};
    while (!candidate.empty()) {
        candidate.pop_back();
        displayText = candidate + std::string(ellipsis);
        if (renderer.measureText(displayText, font, scale).x <= width) {
            truncated = true;
            return displayText;
        }
    }

    truncated = true;
    return std::string{ellipsis};
}

std::vector<std::string> wrapWords(
    TextRenderer& renderer,
    const std::string_view value,
    const std::string& font,
    const float scale,
    const float width)
{
    std::vector<std::string> lines;
    std::istringstream stream{std::string{value}};
    std::string word;
    std::string current;

    while (stream >> word) {
        const std::string candidate = current.empty() ? word : current + " " + word;
        if (current.empty() || renderer.measureText(candidate, font, scale).x <= width) {
            current = candidate;
            continue;
        }

        lines.push_back(current);
        current = word;
    }

    if (!current.empty() || lines.empty()) {
        lines.push_back(current);
    }

    return lines;
}

} // namespace

bool UIFrame::drawText(const std::string_view value, const UIClipRect& bounds, const UITextStyle& textStyle) const
{
    if (textStyle.scale <= 0.0f || bounds.size.x <= 0.0f || bounds.size.y <= 0.0f) {
        return false;
    }

    const float availableWidth = std::max(bounds.size.x - std::abs(textStyle.offset.x), 0.0f);
    const int maxLines = textStyle.maxLines <= 0 ? 1024 : textStyle.maxLines;
    bool truncated = false;
    std::vector<std::string> lines = textStyle.wrap == UITextWrap::Word
        ? wrapWords(*m_text, value, textStyle.font, textStyle.scale, availableWidth)
        : std::vector<std::string>{std::string{value}};

    if (static_cast<int>(lines.size()) > maxLines) {
        lines.resize(static_cast<std::size_t>(maxLines));
        truncated = true;
    }

    if (textStyle.overflow == UIOverflow::Ellipsis && !lines.empty()) {
        if (textStyle.wrap == UITextWrap::None) {
            bool lineTruncated = false;
            lines.front() = ellipsize(*m_text, lines.front(), textStyle.font, textStyle.scale, availableWidth, lineTruncated);
            truncated = truncated || lineTruncated;
        } else if (truncated) {
            bool lineTruncated = false;
            lines.back() = ellipsize(*m_text, lines.back() + "...", textStyle.font, textStyle.scale, availableWidth, lineTruncated);
            truncated = truncated || lineTruncated;
        }
    }

    const float measuredLineHeight = lineHeight(*m_text, textStyle.font, textStyle.scale);
    const float totalHeight = measuredLineHeight * static_cast<float>(lines.size());
    glm::vec2 position = toScreen(bounds.position);

    if (textStyle.verticalAlignment == UITextVerticalAlignment::Center) {
        position.y += (bounds.size.y - totalHeight) * 0.5f;
    } else if (textStyle.verticalAlignment == UITextVerticalAlignment::Bottom) {
        position.y += bounds.size.y - totalHeight;
    }

    glm::vec4 color = textStyle.color;
    color.a *= std::clamp(textStyle.opacity, 0.0f, 1.0f);

    if (textStyle.overflow == UIOverflow::Clip || textStyle.overflow == UIOverflow::Ellipsis) {
        m_text->pushClipRect(toScreen(bounds));
    }

    for (std::size_t index = 0; index < lines.size(); ++index) {
        const glm::vec2 measured = m_text->measureText(lines[index], textStyle.font, textStyle.scale);
        glm::vec2 linePosition = position;
        if (textStyle.horizontalAlignment == UITextHorizontalAlignment::Center) {
            linePosition.x += (bounds.size.x - measured.x) * 0.5f;
        } else if (textStyle.horizontalAlignment == UITextHorizontalAlignment::Right) {
            linePosition.x += bounds.size.x - measured.x;
        }
        linePosition.y += static_cast<float>(index) * measuredLineHeight;
        m_text->drawText(lines[index], linePosition + textStyle.offset, color, textStyle.font, textStyle.scale);
    }

    if (textStyle.overflow == UIOverflow::Clip || textStyle.overflow == UIOverflow::Ellipsis) {
        m_text->popClipRect();
    }

    return truncated;
}

void UIFrame::pushClip(const UIClipRect& bounds) const
{
    const UIClipRect screenBounds = toScreen(bounds);
    m_input->pushClipRect(screenBounds);
    m_shapes->pushClipRect(screenBounds);
    m_text->pushClipRect(screenBounds);
}

void UIFrame::popClip() const
{
    m_text->popClipRect();
    m_shapes->popClipRect();
    m_input->popClipRect();
}

} // namespace Engine
