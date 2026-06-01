#pragma once

#include "Engine/UI/UIStyle.hpp"
#include "Engine/UI/TextInput.hpp"
#include "Engine/UI/Widget.hpp"

#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace Engine {

class TextRenderer;

class NumberInput final : public Widget {
public:
    NumberInput(std::string id, float value = 0.0f, float minValue = 0.0f, float maxValue = 1.0f);

    void update(UIContext& context) override;
    void update(UIContext& context, const TextRenderer& textRenderer, const UIStyle& style, std::string_view fontName = "default", float scale = 1.0f);
    void render(Renderer2D& renderer2D, const UIStyle& style) const override;

    void setValue(float value);
    void setRange(float minValue, float maxValue);
    void setSensitivity(float sensitivity);
    void setPrecision(int precision);
    void setStyle(const UINumberInputStyle& style);
    void clearStyleOverride();
    void setOnValueChanged(std::function<void(float)> callback);

    [[nodiscard]] float value() const;
    [[nodiscard]] float normalizedValue() const;
    [[nodiscard]] std::string formattedValue() const;
    [[nodiscard]] bool editing() const;
    [[nodiscard]] const TextInput& textEditor() const;

private:
    void updateScrubbing(UIContext& context);
    void beginEditing(UIContext& context);

    float m_value{0.0f};
    float m_minValue{0.0f};
    float m_maxValue{1.0f};
    float m_sensitivity{0.01f};
    float m_dragStartValue{0.0f};
    float m_dragStartMouseX{0.0f};
    int m_precision{2};
    bool m_wasHeld{false};
    bool m_dragged{false};
    bool m_editing{false};
    TextInput m_textEditor;
    std::optional<UINumberInputStyle> m_styleOverride;
    std::function<void(float)> m_onValueChanged;
};

} // namespace Engine
