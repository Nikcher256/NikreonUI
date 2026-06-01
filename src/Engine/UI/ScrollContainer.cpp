#include "Engine/UI/ScrollContainer.hpp"

#include "Engine/Renderer/TextRenderer.hpp"
#include "Engine/UI/UIContext.hpp"

#include <algorithm>

namespace Engine {

void ScrollContainer::setBounds(const UIClipRect& bounds)
{
    m_bounds = bounds;
    clampOffset();
}

void ScrollContainer::setContentHeight(const float contentHeight)
{
    m_contentHeight = std::max(contentHeight, 0.0f);
    clampOffset();
}

void ScrollContainer::setWheelStep(const float wheelStep)
{
    m_wheelStep = std::max(wheelStep, 0.0f);
}

void ScrollContainer::update(const UIContext& context)
{
    if (context.isMouseInside(m_bounds.position, m_bounds.size)) {
        m_offset -= context.scrollDelta().y * m_wheelStep;
        clampOffset();
    }
}

void ScrollContainer::pushClip(UIContext& context) const
{
    context.pushClipRect(m_bounds);
}

void ScrollContainer::popClip(UIContext& context) const
{
    context.popClipRect();
}

void ScrollContainer::pushClip(Renderer2D& renderer2D) const
{
    renderer2D.pushClipRect(m_bounds);
}

void ScrollContainer::popClip(Renderer2D& renderer2D) const
{
    renderer2D.popClipRect();
}

void ScrollContainer::pushClip(TextRenderer& textRenderer) const
{
    textRenderer.pushClipRect(m_bounds);
}

void ScrollContainer::popClip(TextRenderer& textRenderer) const
{
    textRenderer.popClipRect();
}

void ScrollContainer::renderScrollbar(Renderer2D& renderer2D) const
{
    if (maxOffset() <= 0.0f || m_bounds.size.y <= 0.0f) {
        return;
    }

    constexpr float width = 4.0f;
    const float thumbHeight = std::max(m_bounds.size.y * (m_bounds.size.y / m_contentHeight), 18.0f);
    const float travel = std::max(m_bounds.size.y - thumbHeight, 0.0f);
    const float thumbY = m_bounds.position.y + travel * (m_offset / maxOffset());
    renderer2D.drawSdfRect(
        {m_bounds.position.x + m_bounds.size.x - width, thumbY},
        {width, thumbHeight},
        width * 0.5f,
        {0.42f, 0.52f, 0.68f, 0.9f},
        {0.42f, 0.52f, 0.68f, 0.9f},
        0.0f);
}

float ScrollContainer::offset() const
{
    return m_offset;
}

float ScrollContainer::maxOffset() const
{
    return std::max(m_contentHeight - m_bounds.size.y, 0.0f);
}

void ScrollContainer::clampOffset()
{
    m_offset = std::clamp(m_offset, 0.0f, maxOffset());
}

} // namespace Engine
