#pragma once

#include "Engine/UI/UIStyle.hpp"
#include "Engine/UI/Widget.hpp"

#include <functional>
#include <optional>
#include <string>

namespace Engine {

class TextInput final : public Widget {
public:
    explicit TextInput(std::string id, std::string value = {});

    void update(UIContext& context) override;
    void render(Renderer2D& renderer2D, const UIStyle& style) const override;

    void setValue(std::string value);
    void setPlaceholder(std::string placeholder);
    void setStyle(const UITextInputStyle& style);
    void clearStyleOverride();
    void setOnValueChanged(std::function<void(std::string_view)> callback);

    [[nodiscard]] const std::string& value() const;
    [[nodiscard]] const std::string& placeholder() const;
    [[nodiscard]] std::size_t caretIndex() const;
    [[nodiscard]] std::size_t selectionStart() const;
    [[nodiscard]] std::size_t selectionEnd() const;
    [[nodiscard]] bool hasSelection() const;
    [[nodiscard]] bool focused() const;

private:
    void notifyValueChanged() const;
    void eraseSelection();
    void moveCaret(std::size_t caretIndex, bool extendSelection);

    std::string m_value;
    std::string m_placeholder;
    std::size_t m_caretIndex{0};
    std::size_t m_selectionAnchor{0};
    bool m_focused{false};
    std::optional<UITextInputStyle> m_styleOverride;
    std::function<void(std::string_view)> m_onValueChanged;
};

} // namespace Engine
