#pragma once

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
    Enter,
    Escape,
};

struct UIInputState {
    glm::vec2 mousePosition{0.0f, 0.0f};
    glm::vec2 scrollDelta{0.0f, 0.0f};
    bool primaryMouseDown{false};
    std::vector<char32_t> typedCharacters;
    std::vector<UIKey> pressedKeys;
};

struct UIInteraction {
    bool hovered{false};
    bool held{false};
    bool pressed{false};
};

class UIContext {
public:
    void beginFrame(const UIInputState& input);
    void endFrame();

    [[nodiscard]] UIInteraction interact(std::string_view id, const glm::vec2& position, const glm::vec2& size);
    [[nodiscard]] bool button(std::string_view id, const glm::vec2& position, const glm::vec2& size);
    bool checkbox(std::string_view id, const glm::vec2& position, const glm::vec2& size, bool& value);
    bool slider(std::string_view id, const glm::vec2& position, const glm::vec2& size, float& value, float minValue, float maxValue);

    [[nodiscard]] glm::vec2 mousePosition() const;
    [[nodiscard]] glm::vec2 scrollDelta() const;
    [[nodiscard]] const std::vector<char32_t>& typedCharacters() const;
    [[nodiscard]] const std::vector<UIKey>& pressedKeys() const;
    [[nodiscard]] bool primaryMousePressed() const;
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

    glm::vec2 m_mousePosition{0.0f, 0.0f};
    glm::vec2 m_scrollDelta{0.0f, 0.0f};
    std::vector<char32_t> m_typedCharacters;
    std::vector<UIKey> m_pressedKeys;
    std::vector<UIClipRect> m_clipStack;
    std::string m_hotId;
    std::string m_activeId;
    std::string m_focusedId;
    bool m_mouseDown{false};
    bool m_previousMouseDown{false};
    bool m_mousePressed{false};
    bool m_mouseReleased{false};
};

} // namespace Engine
