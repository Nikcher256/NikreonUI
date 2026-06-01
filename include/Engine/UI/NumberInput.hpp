#pragma once

#include "Engine/UI/UIStyle.hpp"
#include "Engine/UI/Widget.hpp"

#include <functional>
#include <optional>
#include <string>

namespace Engine {

class NumberInput final : public Widget {
public:
    NumberInput(std::string id, float value = 0.0f, float minValue = 0.0f, float maxValue = 1.0f);

    void update(UIContext& context) override;
    void render(Renderer2D& renderer2D, const UIStyle& style) const override;

    void setValue(float value);
    void setRange(float minValue, float maxValue);
    void setSensitivity(float sensitivity);
    void setPrecision(int precision);
    void setStyle(const UINumberInputStyle& style);
    void clearStyleOverride();
    void setOnValueChanged(std::function<void(float)> callback);

    [[nodiscard]] float value() const;
    [[nodiscard]] std::string formattedValue() const;

private:
    float m_value{0.0f};
    float m_minValue{0.0f};
    float m_maxValue{1.0f};
    float m_sensitivity{0.01f};
    float m_dragStartValue{0.0f};
    float m_dragStartMouseX{0.0f};
    int m_precision{2};
    bool m_wasHeld{false};
    std::optional<UINumberInputStyle> m_styleOverride;
    std::function<void(float)> m_onValueChanged;
};

} // namespace Engine
