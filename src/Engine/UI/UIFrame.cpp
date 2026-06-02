#include "Engine/UI/UIFrame.hpp"

#include <algorithm>

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

void UIFrame::drawText(const std::string_view value, const UIClipRect& bounds, const UITextStyle& textStyle) const
{
    if (textStyle.scale <= 0.0f || bounds.size.x <= 0.0f || bounds.size.y <= 0.0f) {
        return;
    }
    const glm::vec2 measured = m_text->measureText(value, textStyle.font, textStyle.scale);
    glm::vec2 position = toScreen(bounds.position);
    if (textStyle.horizontalAlignment == UITextHorizontalAlignment::Center) position.x += (bounds.size.x - measured.x) * 0.5f;
    else if (textStyle.horizontalAlignment == UITextHorizontalAlignment::Right) position.x += bounds.size.x - measured.x;
    if (textStyle.verticalAlignment == UITextVerticalAlignment::Center) position.y += (bounds.size.y - measured.y) * 0.5f;
    else if (textStyle.verticalAlignment == UITextVerticalAlignment::Bottom) position.y += bounds.size.y - measured.y;
    glm::vec4 color = textStyle.color;
    color.a *= std::clamp(textStyle.opacity, 0.0f, 1.0f);
    m_text->drawText(value, position + textStyle.offset, color, textStyle.font, textStyle.scale);
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
