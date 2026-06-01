#include "Engine/UI/Layout.hpp"

#include "Engine/UI/Widget.hpp"

#include <algorithm>

#include <glm/common.hpp>

namespace Engine {

namespace {

UIRect inset(const UIRect& bounds, const UIEdgeInsets& insets)
{
    return {
        {bounds.position.x + insets.left, bounds.position.y + insets.top},
        {
            std::max(0.0f, bounds.size.x - insets.left - insets.right),
            std::max(0.0f, bounds.size.y - insets.top - insets.bottom),
        },
    };
}

float major(const glm::vec2& value, const UILayoutAxis axis)
{
    return axis == UILayoutAxis::Horizontal ? value.x : value.y;
}

float cross(const glm::vec2& value, const UILayoutAxis axis)
{
    return axis == UILayoutAxis::Horizontal ? value.y : value.x;
}

float leading(const UIEdgeInsets& insets, const UILayoutAxis axis)
{
    return axis == UILayoutAxis::Horizontal ? insets.left : insets.top;
}

float trailing(const UIEdgeInsets& insets, const UILayoutAxis axis)
{
    return axis == UILayoutAxis::Horizontal ? insets.right : insets.bottom;
}

float crossLeading(const UIEdgeInsets& insets, const UILayoutAxis axis)
{
    return axis == UILayoutAxis::Horizontal ? insets.top : insets.left;
}

float crossTrailing(const UIEdgeInsets& insets, const UILayoutAxis axis)
{
    return axis == UILayoutAxis::Horizontal ? insets.bottom : insets.right;
}

glm::vec2 fromAxes(const float majorValue, const float crossValue, const UILayoutAxis axis)
{
    return axis == UILayoutAxis::Horizontal
        ? glm::vec2{majorValue, crossValue}
        : glm::vec2{crossValue, majorValue};
}

} // namespace

UIEdgeInsets UIEdgeInsets::all(const float value)
{
    return {value, value, value, value};
}

UIEdgeInsets UIEdgeInsets::symmetric(const float horizontal, const float vertical)
{
    return {horizontal, vertical, horizontal, vertical};
}

UIAnchors UIAnchors::fill()
{
    return {{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}};
}

UIAnchors UIAnchors::fixed(const glm::vec2& anchor, const glm::vec2& offset, const glm::vec2& size)
{
    return {anchor, anchor, offset, offset + size};
}

UIAnchors UIAnchors::horizontalStretch(const float top, const float height)
{
    return {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, top}, {0.0f, top + height}};
}

UILayoutTarget::UILayoutTarget(UIRect& rect)
    : m_rect(&rect)
{
}

UILayoutTarget::UILayoutTarget(Widget& widget)
    : m_widget(&widget)
{
}

void UILayoutTarget::setBounds(const UIRect& bounds) const
{
    if (m_rect != nullptr) {
        *m_rect = bounds;
    }

    if (m_widget != nullptr) {
        m_widget->setBounds(bounds.position, bounds.size);
    }
}

UILinearLayout::UILinearLayout(const UILayoutAxis axis)
    : m_axis(axis)
{
}

void UILinearLayout::setBounds(const UIRect& bounds)
{
    m_bounds = bounds;
}

void UILinearLayout::setPadding(const UIEdgeInsets& padding)
{
    m_padding = padding;
}

void UILinearLayout::setGap(const float gap)
{
    m_gap = std::max(0.0f, gap);
}

void UILinearLayout::setAlignment(const UIAlignment alignment)
{
    m_alignment = alignment;
}

void UILinearLayout::add(
    const UILayoutTarget& target,
    const glm::vec2& preferredSize,
    const float grow,
    const UIEdgeInsets& margin,
    const std::optional<UIAlignment> alignment)
{
    m_items.push_back({target, preferredSize, std::max(0.0f, grow), margin, alignment});
}

void UILinearLayout::clear()
{
    m_items.clear();
}

void UILinearLayout::layout() const
{
    if (m_items.empty()) {
        return;
    }

    const UIRect content = inset(m_bounds, m_padding);
    const float totalGap = m_gap * static_cast<float>(m_items.size() - 1);
    float occupiedMajor = totalGap;
    float totalGrow = 0.0f;

    for (const Item& item : m_items) {
        occupiedMajor += std::max(0.0f, major(item.preferredSize, m_axis));
        occupiedMajor += leading(item.margin, m_axis) + trailing(item.margin, m_axis);
        totalGrow += item.grow;
    }

    const float availableExtra = std::max(0.0f, major(content.size, m_axis) - occupiedMajor);
    float cursor = major(content.position, m_axis);
    const float contentCrossPosition = cross(content.position, m_axis);
    const float contentCrossSize = cross(content.size, m_axis);

    for (const Item& item : m_items) {
        cursor += leading(item.margin, m_axis);

        const float itemMajor = std::max(0.0f, major(item.preferredSize, m_axis)) +
            (totalGrow > 0.0f ? availableExtra * item.grow / totalGrow : 0.0f);
        const float crossAvailable = std::max(
            0.0f,
            contentCrossSize - crossLeading(item.margin, m_axis) - crossTrailing(item.margin, m_axis));
        const UIAlignment alignment = item.alignment.value_or(m_alignment);
        const float requestedCross = std::max(0.0f, cross(item.preferredSize, m_axis));
        const float itemCross = alignment == UIAlignment::Stretch
            ? crossAvailable
            : std::min(requestedCross, crossAvailable);

        float crossOffset = crossLeading(item.margin, m_axis);
        if (alignment == UIAlignment::Center) {
            crossOffset += (crossAvailable - itemCross) * 0.5f;
        } else if (alignment == UIAlignment::End) {
            crossOffset += crossAvailable - itemCross;
        }

        item.target.setBounds({
            fromAxes(cursor, contentCrossPosition + crossOffset, m_axis),
            fromAxes(itemMajor, itemCross, m_axis),
        });

        cursor += itemMajor + trailing(item.margin, m_axis) + m_gap;
    }
}

void UIDockLayout::setBounds(const UIRect& bounds)
{
    m_bounds = bounds;
}

void UIDockLayout::setPadding(const UIEdgeInsets& padding)
{
    m_padding = padding;
}

void UIDockLayout::setGap(const float gap)
{
    m_gap = std::max(0.0f, gap);
}

void UIDockLayout::add(const UILayoutTarget& target, const UIDock dock, const float extent, const UIEdgeInsets& margin)
{
    m_items.push_back({target, dock, std::max(0.0f, extent), margin});
}

void UIDockLayout::clear()
{
    m_items.clear();
}

void UIDockLayout::layout() const
{
    UIRect remaining = inset(m_bounds, m_padding);

    for (const Item& item : m_items) {
        UIRect allocated = remaining;

        switch (item.dock) {
        case UIDock::Left: {
            allocated.size.x = std::min(item.extent, remaining.size.x);
            const float consumed = std::min(remaining.size.x, allocated.size.x + m_gap);
            remaining.position.x += consumed;
            remaining.size.x -= consumed;
            break;
        }
        case UIDock::Top: {
            allocated.size.y = std::min(item.extent, remaining.size.y);
            const float consumed = std::min(remaining.size.y, allocated.size.y + m_gap);
            remaining.position.y += consumed;
            remaining.size.y -= consumed;
            break;
        }
        case UIDock::Right: {
            allocated.size.x = std::min(item.extent, remaining.size.x);
            allocated.position.x = remaining.position.x + remaining.size.x - allocated.size.x;
            const float consumed = std::min(remaining.size.x, allocated.size.x + m_gap);
            remaining.size.x -= consumed;
            break;
        }
        case UIDock::Bottom: {
            allocated.size.y = std::min(item.extent, remaining.size.y);
            allocated.position.y = remaining.position.y + remaining.size.y - allocated.size.y;
            const float consumed = std::min(remaining.size.y, allocated.size.y + m_gap);
            remaining.size.y -= consumed;
            break;
        }
        case UIDock::Fill:
            break;
        }

        item.target.setBounds(inset(allocated, item.margin));
    }
}

void UIStackLayout::setBounds(const UIRect& bounds)
{
    m_bounds = bounds;
}

void UIStackLayout::setPadding(const UIEdgeInsets& padding)
{
    m_padding = padding;
}

void UIStackLayout::add(const UILayoutTarget& target, const UIAnchors& anchors, const UIEdgeInsets& margin)
{
    m_items.push_back({target, anchors, margin});
}

void UIStackLayout::clear()
{
    m_items.clear();
}

void UIStackLayout::layout() const
{
    const UIRect content = inset(m_bounds, m_padding);

    for (const Item& item : m_items) {
        const glm::vec2 minimum = content.position + content.size * item.anchors.minimum + item.anchors.offsetMinimum;
        const glm::vec2 maximum = content.position + content.size * item.anchors.maximum + item.anchors.offsetMaximum;
        item.target.setBounds(inset({minimum, glm::max(maximum - minimum, glm::vec2{0.0f, 0.0f})}, item.margin));
    }
}

} // namespace Engine
