#pragma once

#include "Engine/UI/UIStyle.hpp"
#include "Engine/UI/Widget.hpp"

#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace Engine {

class TextRenderer;
class UIFrame;

class TextInput final : public Widget {
public:
    explicit TextInput(std::string id, std::string value = {});

    void update(UIContext& context) override;
    void update(UIContext& context, const TextRenderer& textRenderer, const UIStyle& style, std::string_view fontName = "default", float scale = 1.0f);
    void render(Renderer2D& renderer2D, const UIStyle& style) const override;
    void render(const UIFrame& frame) const;
    void renderText(
    TextRenderer& textRenderer,
    const UIStyle& style,
    std::string_view fontName = "default",
    float scale = 1.0f,
    const glm::vec2& origin = {},
    const UITextStyle* textStyleOverride = nullptr) const;

    void setValue(std::string value);
    void setPlaceholder(std::string placeholder);
    void setStyle(const UITextInputStyle& style);
    void clearStyleOverride();
    void setOnValueChanged(std::function<void(std::string_view)> callback);
    void selectAll();

    [[nodiscard]] const std::string& value() const;
    [[nodiscard]] const std::string& placeholder() const;
    [[nodiscard]] std::size_t caretIndex() const;
    [[nodiscard]] std::size_t selectionStart() const;
    [[nodiscard]] std::size_t selectionEnd() const;
    [[nodiscard]] bool hasSelection() const;
    [[nodiscard]] bool focused() const;
    [[nodiscard]] float horizontalScrollOffset() const;
    [[nodiscard]] float horizontalScrollRange() const;

private:
    void updateEditing(UIContext& context, const TextRenderer* textRenderer, const UITextInputStyle* inputStyle, std::string_view fontName, float scale);
    void notifyValueChanged() const;
    void eraseSelection();
    void moveCaret(std::size_t caretIndex, bool extendSelection);
    [[nodiscard]] std::size_t caretIndexAt(float mouseX, const TextRenderer& textRenderer, const UITextInputStyle& inputStyle, std::string_view fontName, float scale) const;
    void ensureCaretVisible(const TextRenderer& textRenderer, const UITextInputStyle& inputStyle, std::string_view fontName, float scale);
    void updateHorizontalScrollbar(UIContext& context, const UITextInputStyle& inputStyle);

    std::string m_value;
    std::string m_placeholder;
    std::size_t m_caretIndex{0};
    std::size_t m_selectionAnchor{0};
    bool m_focused{false};
    float m_horizontalScrollOffset{0.0f};
    float m_horizontalScrollRange{0.0f};
    float m_scrollDragStartOffset{0.0f};
    float m_scrollDragStartMouseX{0.0f};
    bool m_scrollThumbWasHeld{false};
    std::optional<UITextInputStyle> m_styleOverride;
    std::function<void(std::string_view)> m_onValueChanged;
};

} // namespace Engine
