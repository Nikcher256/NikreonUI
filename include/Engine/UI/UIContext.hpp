#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <glm/vec2.hpp>

#include "Engine/Renderer/Renderer2D.hpp"

namespace Engine {

enum class UIKey {
    Backspace,
    Delete,
    Left,
    Right,
    Home,
    End,
    SelectAll,
    Copy,
    Paste,
    Enter,
    Escape,
};

struct UIInputState {
    glm::vec2 mousePosition{0.0f, 0.0f};
    glm::vec2 scrollDelta{0.0f, 0.0f};
    bool primaryMouseDown{false};
    bool shiftDown{false};
    std::string clipboardText;
    std::function<void(std::string_view)> setClipboardText;
    std::vector<char32_t> typedCharacters;
    std::vector<UIKey> pressedKeys;
};

struct UIInteraction {
    bool hovered{false};
    bool held{false};
    bool pressed{false};
};

struct UIInteractionLayer {
    std::string id;
    UIClipRect bounds;
    int zIndex{0};
    bool modal{false};
};

class UIContext {
public:
    void beginFrame(const UIInputState& input);
    void endFrame();

    void registerLayer(std::string_view id, int zIndex, const UIClipRect& bounds, bool modal = false);
    void unregisterLayer(std::string_view id);
    void pushLayer(std::string_view id);
    void popLayer();

    [[nodiscard]] UIInteraction interact(std::string_view id, const glm::vec2& position, const glm::vec2& size);
    [[nodiscard]] bool button(std::string_view id, const glm::vec2& position, const glm::vec2& size);
    bool checkbox(std::string_view id, const glm::vec2& position, const glm::vec2& size, bool& value);
    bool slider(std::string_view id, const glm::vec2& position, const glm::vec2& size, float& value, float minValue, float maxValue);

    [[nodiscard]] glm::vec2 mousePosition() const;
    [[nodiscard]] glm::vec2 scrollDelta() const;
    [[nodiscard]] const std::vector<char32_t>& typedCharacters() const;
    [[nodiscard]] const std::vector<UIKey>& pressedKeys() const;
    [[nodiscard]] bool primaryMousePressed() const;
    [[nodiscard]] bool shiftDown() const;
    [[nodiscard]] std::string_view clipboardText() const;
    void setClipboardText(std::string_view text) const;
    [[nodiscard]] bool isMouseInside(const glm::vec2& position, const glm::vec2& size) const;
    void focus(std::string_view id);
    void clearFocus();
    [[nodiscard]] bool isFocused(std::string_view id) const;
    void pushClipRect(const UIClipRect& clipRect);
    void popClipRect();
    [[nodiscard]] bool isHot(std::string_view id) const;
    [[nodiscard]] bool isActive(std::string_view id) const;

private:
    [[nodiscard]] bool contains(const glm::vec2& position, const glm::vec2& size) const;
    [[nodiscard]] bool rawContains(const glm::vec2& position, const glm::vec2& size) const;
    [[nodiscard]] bool layerAllowsInteraction(std::string_view layerId) const;
    [[nodiscard]] const UIInteractionLayer* findLayer(std::string_view id) const;
    [[nodiscard]] const UIInteractionLayer* topInputLayer() const;

    glm::vec2 m_mousePosition{0.0f, 0.0f};
    glm::vec2 m_scrollDelta{0.0f, 0.0f};
    std::vector<char32_t> m_typedCharacters;
    std::vector<UIKey> m_pressedKeys;
    std::string m_clipboardText;
    std::function<void(std::string_view)> m_setClipboardText;
    std::vector<UIClipRect> m_clipStack;
    std::string m_hotId;
    std::string m_activeId;
    std::string m_focusedId;
    std::vector<UIInteractionLayer> m_layers;
    std::vector<std::string> m_layerStack;
    bool m_mouseDown{false};
    bool m_previousMouseDown{false};
    bool m_mousePressed{false};
    bool m_mouseReleased{false};
    bool m_shiftDown{false};
};

} // namespace Engine
