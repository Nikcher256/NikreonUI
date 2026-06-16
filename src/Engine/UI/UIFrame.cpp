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

constexpr float TextClipTopPadding = 1.0f;
constexpr float TextClipBottomPadding = 3.0f;

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

    const auto appendWrappedToken = [&](const std::string& token, std::string& current) {
        std::string remaining = token;
        while (!remaining.empty()) {
            const std::string candidate = current.empty() ? remaining : current + " " + remaining;
            if (renderer.measureText(candidate, font, scale).x <= width) {
                current = candidate;
                return;
            }

            if (!current.empty()) {
                lines.push_back(current);
                current.clear();
                continue;
            }

            std::string line;
            while (!remaining.empty()) {
                const std::string next = line + remaining.front();
                if (!line.empty() && renderer.measureText(next, font, scale).x > width) {
                    break;
                }
                line = next;
                remaining.erase(remaining.begin());
            }

            if (line.empty()) {
                line.push_back(remaining.front());
                remaining.erase(remaining.begin());
            }
            lines.push_back(line);
        }
    };

    std::istringstream stream{std::string{value}};
    std::string word;
    std::string current;

    while (stream >> word) {
        appendWrappedToken(word, current);
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

    if (textStyle.overflow == UIOverflow::Clip) {
        truncated = truncated || totalHeight > bounds.size.y;
        for (const std::string& line : lines) {
            truncated = truncated || m_text->measureText(line, textStyle.font, textStyle.scale).x > availableWidth;
        }
    }

    if (textStyle.verticalAlignment == UITextVerticalAlignment::Center) {
        position.y += (bounds.size.y - totalHeight) * 0.5f;
    } else if (textStyle.verticalAlignment == UITextVerticalAlignment::Bottom) {
        position.y += bounds.size.y - totalHeight;
    }

    glm::vec4 color = textStyle.color;
    color.a *= std::clamp(textStyle.opacity, 0.0f, 1.0f);

    const bool clipsText = textStyle.overflow == UIOverflow::Clip;
    if (clipsText) {
        UIClipRect clipBounds = toScreen(bounds);
        clipBounds.position.y = std::max(0.0f, clipBounds.position.y - TextClipTopPadding);
        clipBounds.size.y += TextClipTopPadding + TextClipBottomPadding;
        m_text->pushClipRect(clipBounds);
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

    if (clipsText) {
        m_text->popClipRect();
    }

    return truncated;
}

bool UIFrame::drawTextWithTooltip(
    const std::string_view value,
    const UIClipRect& bounds,
    const UITextStyle& textStyle,
    const std::string_view tooltipText) const
{
    const bool truncated = drawText(value, bounds, textStyle);
    const std::string_view requestedTooltip = tooltipText.empty() ? value : tooltipText;
    if (truncated && !requestedTooltip.empty()) {
        const UIClipRect screenBounds = toScreen(bounds);
        if (m_input->isMouseInside(screenBounds.position, screenBounds.size)) {
            m_input->requestTooltip(requestedTooltip);
        }
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

void UIFrame::beginCompositeRenderItem() const
{
    const std::uint64_t renderOrder = m_shapes->reserveRenderOrder();
    m_shapes->beginCompositeRenderItem(renderOrder);
    m_text->beginCompositeRenderItem(renderOrder);
}

void UIFrame::endCompositeRenderItem() const
{
    m_text->endCompositeRenderItem();
    m_shapes->endCompositeRenderItem();
}

UICompositeRenderScope::UICompositeRenderScope(const UIFrame& frame)
    : m_frame(&frame)
{
    m_frame->beginCompositeRenderItem();
}

UICompositeRenderScope::~UICompositeRenderScope()
{
    m_frame->endCompositeRenderItem();
}

} // namespace Engine
