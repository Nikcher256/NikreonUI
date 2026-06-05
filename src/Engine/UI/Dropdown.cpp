#include "Engine/UI/Dropdown.hpp"

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/Renderer/TextRenderer.hpp"
#include "Engine/UI/UIFrame.hpp"
#include "Engine/UI/UIStyle.hpp"

#include <algorithm>
#include <utility>
#include <string>

namespace Engine {

namespace {

bool contains(const glm::vec2& point, const glm::vec2& position, const glm::vec2& size)
{
    return point.x >= position.x && point.y >= position.y &&
        point.x <= position.x + size.x && point.y <= position.y + size.y;
}

} //namespace 

Dropdown::Dropdown(std::string id, std::vector<std::string> items, const std::size_t selectedIndex)
    : Widget(std::move(id))
    , m_items(std::move(items))
{
    setSelectedIndex(selectedIndex);
}

void Dropdown::update(UIContext& context)
{
    if (!m_visible) {
        m_interaction = {};
        return;
    }

    m_interaction = context.interact(m_id + ".button", m_position, m_size);
    if (m_interaction.pressed) {
        m_popupOpen = !m_popupOpen;
    }
}

void Dropdown::render(Renderer2D& renderer2D, const UIStyle& style) const
{
    if (!m_visible) {
        return;
    }

    const UIButtonStyle& buttonStyle = style.resolveButton(m_styleClass, m_id);
    const glm::vec4 fill = m_interaction.held
        ? buttonStyle.pressed
        : m_interaction.hovered
            ? buttonStyle.hovered
            : buttonStyle.normal.fill;

    renderer2D.drawSdfRect(
        m_position,
        m_size,
        buttonStyle.normal.borderRadius,
        fill,
        buttonStyle.normal.border,
        buttonStyle.normal.borderWidth);

    const glm::vec4 chevron = buttonStyle.icon;
    const glm::vec2 center{
        m_position.x + m_size.x - 16.0f,
        m_position.y + m_size.y * 0.5f,
    };
    renderer2D.drawQuad({center.x - 5.0f, center.y - 2.0f}, {4.0f, 4.0f}, chevron);
    renderer2D.drawQuad({center.x - 1.0f, center.y + 2.0f}, {4.0f, 4.0f}, chevron);
    renderer2D.drawQuad({center.x + 3.0f, center.y - 2.0f}, {4.0f, 4.0f}, chevron);
}

void Dropdown::updatePopup(UIContext& context)
{
    m_hoveredIndex = -1;

    if (!m_visible || !m_popupOpen) {
        return;
    }

    const glm::vec2 popup = popupPosition();
    const glm::vec2 size = popupSize();

    if (context.primaryMousePressed() &&
        !contains(context.mousePosition(), m_position, m_size) &&
        !contains(context.mousePosition(), popup, size)) {
        m_popupOpen = false;
        return;
    }

    context.pushLayer(m_id + ".popup-layer");
    for (std::size_t index = 0; index < m_items.size(); ++index) {
        const glm::vec2 rowPosition{popup.x, popup.y + static_cast<float>(index) * m_rowHeight};
        const UIInteraction interaction = context.interact(m_id + ".item." + std::to_string(index), rowPosition, {size.x, m_rowHeight});
        
        if (interaction.hovered) {
            m_hoveredIndex = static_cast<int>(index);
        }

        if (interaction.pressed) {
            setSelectedIndex(index);
            m_popupOpen = false;
            break;
        }
    }
    context.popLayer();
}

void Dropdown::renderPopup(UIContext& context, Renderer2D& renderer2D, TextRenderer& textRenderer, const UIStyle& style) const
{
    if (!m_visible || !m_popupOpen) {
        return;
    }

    const glm::vec2 popup = popupPosition();
    const glm::vec2 size = popupSize();
    const UIButtonStyle& buttonStyle = style.resolveButton(m_styleClass, m_id);
    const UITextStyle& textStyle = style.resolveText("toolbar-toggle");

    renderer2D.drawQuad(popup, size, style.panel.fill);
    renderer2D.drawRect(popup, size, style.panel.border, 1.0f);

    UIFrame frame{context, renderer2D, textRenderer, style};

    for (std::size_t index = 0; index < m_items.size(); ++index) {
        const glm::vec2 rowPosition{popup.x, popup.y + static_cast<float>(index) * m_rowHeight};
        const bool selected = index == m_selectedIndex;
        const bool hovered = static_cast<int>(index) == m_hoveredIndex;
        const glm::vec4 rowFill = selected
            ? buttonStyle.selected
            : hovered
                ? buttonStyle.hovered
                : glm::vec4{0.0f, 0.0f, 0.0f, 0.0f};

        if (rowFill.a > 0.0f) {
            renderer2D.drawQuad(rowPosition, {size.x, m_rowHeight}, rowFill);
        }

        frame.drawText(
            m_items[index],
            {{rowPosition.x + 10.0f, rowPosition.y}, {std::max(size.x - 20.0f, 0.0f), m_rowHeight}},
            textStyle);
    }
}

void Dropdown::registerPopupLayer(UIContext& context, const int zIndex, const bool modal) const
{
    if (!m_visible || !m_popupOpen) {
        return;
    }

    context.registerLayer(m_id + ".popup-layer", zIndex, {popupPosition(), popupSize()}, modal);
}

void Dropdown::setItems(std::vector<std::string> items)
{
    m_items = std::move(items);
    if (m_items.empty()) {
        m_selectedIndex = 0;
        return;
    }

    if (m_selectedIndex >= m_items.size()) {
        m_selectedIndex = m_items.size() - 1U;
    }
}

void Dropdown::setSelectedIndex(const std::size_t selectedIndex)
{
    if (m_items.empty()) {
        m_selectedIndex = 0;
        return;
    }

    const std::size_t clampedIndex = std::min(selectedIndex, m_items.size() - 1U);
    if (clampedIndex == m_selectedIndex) {
        return;
    }

    m_selectedIndex = clampedIndex;
    notifySelectionChanged();
}

void Dropdown::setOnSelectionChanged(std::function<void(std::size_t, std::string_view)> callback)
{
    m_onSelectionChanged = std::move(callback);
}

std::size_t Dropdown::selectedIndex() const
{
    return m_selectedIndex;
}

std::string_view Dropdown::selectedText() const
{
    if (m_items.empty() || m_selectedIndex >= m_items.size()) {
        return {};
    }

    return m_items[m_selectedIndex];
}

bool Dropdown::popupOpen() const
{
    return m_popupOpen;
}

glm::vec2 Dropdown::popupPosition() const
{
    return {m_position.x, m_position.y + m_size.y + 6.0f};
}

glm::vec2 Dropdown::popupSize() const
{
    return {
        std::max(m_size.x, 120.0f),
        m_rowHeight * static_cast<float>(std::max(m_items.size(), std::size_t{1})),
    };
}

void Dropdown::notifySelectionChanged()
{
    if (m_onSelectionChanged) {
        m_onSelectionChanged(m_selectedIndex, selectedText());
    }
}

} //namespace Engine