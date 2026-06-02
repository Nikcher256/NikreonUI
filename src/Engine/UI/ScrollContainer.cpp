#include "Engine/UI/ScrollContainer.hpp"

#include "Engine/Renderer/TextRenderer.hpp"
#include "Engine/UI/UIContext.hpp"
#include "Engine/UI/UIStyle.hpp"
#include "Engine/UI/Widget.hpp"

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
    renderScrollbar(renderer2D, UIStyle{});
}

void ScrollContainer::renderScrollbar(Renderer2D& renderer2D, const UIStyle& style) const
{
    if (maxOffset() <= 0.0f || m_bounds.size.y <= 0.0f) {
        return;
    }

    const UIClipRect thumb = thumbBounds(style.scrollbar.width, style.scrollbar.minimumThumbLength);
    renderer2D.drawSdfRect(m_bounds.position + glm::vec2{m_bounds.size.x - style.scrollbar.width, 0.0f}, {style.scrollbar.width, m_bounds.size.y}, style.scrollbar.width * 0.5f, style.scrollbar.track, style.scrollbar.track, 0.0f);
    renderer2D.drawSdfRect(
        thumb.position,
        thumb.size,
        thumb.size.x * 0.5f,
        style.scrollbar.thumb,
        style.scrollbar.thumb,
        0.0f);
}

Widget& ScrollContainer::addChild(std::unique_ptr<Widget> child, const glm::vec2& contentPosition)
{
    Widget& result = *child;
    m_children.push_back({std::move(child), contentPosition});
    return result;
}

void ScrollContainer::clearChildren() { m_children.clear(); }

void ScrollContainer::updateChildren(UIContext& context)
{
    pushClip(context);
    for (Child& child : m_children) {
        child.widget->setBounds(contentOrigin() + child.contentPosition, child.widget->size());
        child.widget->update(context);
    }
    popClip(context);
}

void ScrollContainer::renderChildren(Renderer2D& renderer2D, const UIStyle& style)
{
    pushClip(renderer2D);
    for (Child& child : m_children) child.widget->render(renderer2D, style);
    popClip(renderer2D);
}

glm::vec2 ScrollContainer::contentOrigin() const { return m_bounds.position - glm::vec2{0.0f, m_offset}; }

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
    return thumbBounds(6.0f, 18.0f);
}

UIClipRect ScrollContainer::thumbBounds(const float width, const float minimumThumbLength) const
{
    const float thumbHeight = maxOffset() > 0.0f && m_contentHeight > 0.0f
        ? std::max(m_bounds.size.y * (m_bounds.size.y / m_contentHeight), minimumThumbLength)
        : m_bounds.size.y;
    const float travel = std::max(m_bounds.size.y - thumbHeight, 0.0f);
    const float thumbY = maxOffset() > 0.0f
        ? m_bounds.position.y + travel * (m_offset / maxOffset())
        : m_bounds.position.y;
    return {{m_bounds.position.x + m_bounds.size.x - width, thumbY}, {width, thumbHeight}};
}

} // namespace Engine
