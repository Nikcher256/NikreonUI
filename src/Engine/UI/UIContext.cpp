#include "Engine/UI/UIContext.hpp"

#include <algorithm>

#include <glm/common.hpp>

namespace Engine {

namespace {

UIClipRect intersectClipRects(const UIClipRect& left, const UIClipRect& right)
{
    const glm::vec2 minimum = glm::max(left.position, right.position);
    const glm::vec2 maximum = glm::min(left.position + left.size, right.position + right.size);
    return {minimum, glm::max(maximum - minimum, glm::vec2{0.0f, 0.0f})};
}

} // namespace

// Captures mouse input once so every widget sees a consistent frame state.
void UIContext::beginFrame(const UIInputState& input)
{
    m_hotId.clear();
    m_mousePosition = input.mousePosition;
    m_scrollDelta = input.scrollDelta;
    m_clipboardText = input.clipboardText;
    m_setClipboardText = input.setClipboardText;
    m_typedCharacters = input.typedCharacters;
    m_pressedKeys = input.pressedKeys;
    m_clipStack.clear();
    m_layers.clear();
    m_layerStack.clear();
    m_previousMouseDown = m_mouseDown;
    m_mouseDown = input.primaryMouseDown;
    m_shiftDown = input.shiftDown;
    m_mousePressed = m_mouseDown && !m_previousMouseDown;
    m_mouseReleased = !m_mouseDown && m_previousMouseDown;
}

// Clears active capture after release so the next frame can pick a new widget.
void UIContext::endFrame()
{
    if (m_mouseReleased) {
        m_activeId.clear();
    }
}

void UIContext::registerLayer(const std::string_view id, const int zIndex, const UIClipRect& bounds, const bool modal)
{
    const std::string layerId{id};
    for (UIInteractionLayer& layer : m_layers) {
        if (layer.id == layerId) {
            layer.zIndex = zIndex;
            layer.bounds = bounds;
            layer.modal = modal;
            return;
        }
    }

    m_layers.push_back({layerId, bounds, zIndex, modal});
}

void UIContext::unregisterLayer(const std::string_view id)
{
    const std::string layerId{id};
    m_layers.erase(
        std::remove_if(m_layers.begin(), m_layers.end(), [&layerId](const UIInteractionLayer& layer) {
            return layer.id == layerId;
        }),
        m_layers.end());
    m_layerStack.erase(
        std::remove(m_layerStack.begin(), m_layerStack.end(), layerId),
        m_layerStack.end());
}

void UIContext::pushLayer(const std::string_view id)
{
    m_layerStack.emplace_back(id);
}

void UIContext::popLayer()
{
    if (!m_layerStack.empty()) {
        m_layerStack.pop_back();
    }
}

// Computes hover, hold, and click state for a rectangular widget.
UIInteraction UIContext::interact(const std::string_view id, const glm::vec2& position, const glm::vec2& size)
{
    const std::string widgetId{id};
    const std::string_view layerId = m_layerStack.empty() ? std::string_view{} : std::string_view{m_layerStack.back()};
    const bool layerAllowed = layerAllowsInteraction(layerId);
    const bool hovered = layerAllowed && contains(position, size);
    if (hovered) {
        m_hotId = widgetId;
    }

    if (hovered && m_mousePressed) {
        m_activeId = widgetId;
    }

    const bool held = m_mouseDown && m_activeId == widgetId;
    const bool pressed = m_mouseReleased && hovered && m_activeId == widgetId;
    return {hovered, held, pressed};
}

// Returns true on the frame a button is clicked.
bool UIContext::button(const std::string_view id, const glm::vec2& position, const glm::vec2& size)
{
    return interact(id, position, size).pressed;
}

// Toggles a boolean value when the checkbox is clicked.
bool UIContext::checkbox(const std::string_view id, const glm::vec2& position, const glm::vec2& size, bool& value)
{
    if (!button(id, position, size)) {
        return false;
    }

    value = !value;
    return true;
}

// Updates a normalized value while dragging inside the slider track.
bool UIContext::slider(
    const std::string_view id,
    const glm::vec2& position,
    const glm::vec2& size,
    float& value,
    const float minValue,
    const float maxValue)
{
    const UIInteraction state = interact(id, position, size);
    if (!state.held || maxValue <= minValue) {
        return false;
    }

    const float t = std::clamp((m_mousePosition.x - position.x) / std::max(size.x, 1.0f), 0.0f, 1.0f);
    value = minValue + (maxValue - minValue) * t;
    return true;
}

// Returns the mouse position captured for the current UI frame.
glm::vec2 UIContext::mousePosition() const
{
    return m_mousePosition;
}

glm::vec2 UIContext::scrollDelta() const
{
    return m_scrollDelta;
}

const std::vector<char32_t>& UIContext::typedCharacters() const
{
    return m_typedCharacters;
}

const std::vector<UIKey>& UIContext::pressedKeys() const
{
    return m_pressedKeys;
}

bool UIContext::primaryMousePressed() const
{
    return m_mousePressed;
}

bool UIContext::shiftDown() const
{
    return m_shiftDown;
}

std::string_view UIContext::clipboardText() const
{
    return m_clipboardText;
}

void UIContext::setClipboardText(const std::string_view text) const
{
    if (m_setClipboardText) {
        m_setClipboardText(text);
    }
}

bool UIContext::isMouseInside(const glm::vec2& position, const glm::vec2& size) const
{
    return contains(position, size);
}

void UIContext::focus(const std::string_view id)
{
    m_focusedId = id;
}

void UIContext::clearFocus()
{
    m_focusedId.clear();
}

bool UIContext::isFocused(const std::string_view id) const
{
    return m_focusedId == id;
}

void UIContext::pushClipRect(const UIClipRect& clipRect)
{
    m_clipStack.push_back(m_clipStack.empty() ? clipRect : intersectClipRects(m_clipStack.back(), clipRect));
}

void UIContext::popClipRect()
{
    if (!m_clipStack.empty()) {
        m_clipStack.pop_back();
    }
}

// Reports whether a widget is currently under the mouse.
bool UIContext::isHot(const std::string_view id) const
{
    return m_hotId == id;
}

// Reports whether a widget currently owns the mouse drag/click.
bool UIContext::isActive(const std::string_view id) const
{
    return m_activeId == id;
}

bool UIContext::hasHotItem() const
{
    return !m_hotId.empty();
}

bool UIContext::hasActiveItem() const
{
    return !m_activeId.empty();
}

bool UIContext::baseInputBlocked() const
{
    return topInputLayer() != nullptr || hasHotItem() || hasActiveItem();
}

bool UIContext::rawContains(const glm::vec2& position, const glm::vec2& size) const
{
    return m_mousePosition.x >= position.x &&
        m_mousePosition.y >= position.y &&
        m_mousePosition.x <= position.x + size.x &&
        m_mousePosition.y <= position.y + size.y;
}

bool UIContext::layerAllowsInteraction(const std::string_view layerId) const
{
    const UIInteractionLayer* topLayer = topInputLayer();
    if (topLayer == nullptr) {
        return true;
    }

    const UIInteractionLayer* currentLayer = findLayer(layerId);
    const int currentZ = currentLayer != nullptr ? currentLayer->zIndex : 0;
    if (currentZ > topLayer->zIndex) {
        return true;
    }

    return currentLayer != nullptr && currentLayer->id == topLayer->id;
}

const UIInteractionLayer* UIContext::findLayer(const std::string_view id) const
{
    if (id.empty()) {
        return nullptr;
    }

    const auto found = std::find_if(m_layers.begin(), m_layers.end(), [id](const UIInteractionLayer& layer) {
        return layer.id == id;
    });
    return found == m_layers.end() ? nullptr : &*found;
}

const UIInteractionLayer* UIContext::topInputLayer() const
{
    const UIInteractionLayer* topLayer = nullptr;
    for (const UIInteractionLayer& layer : m_layers) {
        if (!layer.modal && !rawContains(layer.bounds.position, layer.bounds.size)) {
            continue;
        }

        if (topLayer == nullptr || layer.zIndex >= topLayer->zIndex) {
            topLayer = &layer;
        }
    }
    return topLayer;
}

// Tests whether the captured mouse position lies inside a rectangle.
bool UIContext::contains(const glm::vec2& position, const glm::vec2& size) const
{
    const bool insideWidget = rawContains(position, size);
    if (!insideWidget || m_clipStack.empty()) {
        return insideWidget;
    }

    const UIClipRect& clipRect = m_clipStack.back();
    return m_mousePosition.x >= clipRect.position.x &&
        m_mousePosition.y >= clipRect.position.y &&
        m_mousePosition.x <= clipRect.position.x + clipRect.size.x &&
        m_mousePosition.y <= clipRect.position.y + clipRect.size.y;
}

} // namespace Engine
