#include "Engine/UI/ScrollContainer.hpp"

#include "Engine/Renderer/TextRenderer.hpp"
#include "Engine/UI/UIContext.hpp"

#include <algorithm>
#include <utility>

namespace Engine {

ScrollContainer::ScrollContainer(std::string id)
    : m_id(std::move(id))
{
}

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

void ScrollContainer::update(UIContext& context)
{
    if (context.isMouseInside(m_bounds.position, m_bounds.size)) {
        m_offset -= context.scrollDelta().y * m_wheelStep;
        clampOffset();
    }

    const UIClipRect thumb = thumbBounds();
    const UIInteraction interaction = maxOffset() > 0.0f
        ? context.interact(m_id + ".thumb", thumb.position, thumb.size)
        : UIInteraction{};
    if (interaction.held && !m_thumbWasHeld) {
        m_dragStartOffset = m_offset;
        m_dragStartMouseY = context.mousePosition().y;
    }
    if (interaction.held) {
        const float travel = std::max(m_bounds.size.y - thumb.size.y, 0.0f);
        if (travel > 0.0f) {
            m_offset = m_dragStartOffset + (context.mousePosition().y - m_dragStartMouseY) * (maxOffset() / travel);
            clampOffset();
        }
    }
    m_thumbWasHeld = interaction.held;
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

    const UIClipRect thumb = thumbBounds();
    renderer2D.drawSdfRect(
        thumb.position,
        thumb.size,
        thumb.size.x * 0.5f,
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

UIClipRect ScrollContainer::thumbBounds() const
{
    constexpr float width = 6.0f;
    const float thumbHeight = maxOffset() > 0.0f && m_contentHeight > 0.0f
        ? std::max(m_bounds.size.y * (m_bounds.size.y / m_contentHeight), 18.0f)
        : m_bounds.size.y;
    const float travel = std::max(m_bounds.size.y - thumbHeight, 0.0f);
    const float thumbY = maxOffset() > 0.0f
        ? m_bounds.position.y + travel * (m_offset / maxOffset())
        : m_bounds.position.y;
    return {{m_bounds.position.x + m_bounds.size.x - width, thumbY}, {width, thumbHeight}};
}

} // namespace Engine
