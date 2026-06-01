#pragma once

#include "Engine/UI/UIStyle.hpp"
#include "Engine/UI/Widget.hpp"

#include <functional>
#include <optional>

namespace Engine {

class Slider final : public Widget {
public:
    Slider(std::string id, float value = 0.0f, float minValue = 0.0f, float maxValue = 1.0f);

    void update(UIContext& context) override;
    void render(Renderer2D& renderer2D, const UIStyle& style) const override;

    void setValue(float value);
    void setRange(float minValue, float maxValue);
    void setStyle(const UISliderStyle& style);
    void clearStyleOverride();
    void setOnValueChanged(std::function<void(float)> callback);

    [[nodiscard]] float value() const;
    [[nodiscard]] float normalizedValue() const;

private:
    float m_value{0.0f};
    float m_minValue{0.0f};
    float m_maxValue{1.0f};
    std::optional<UISliderStyle> m_styleOverride;
    std::function<void(float)> m_onValueChanged;
};

} // namespace Engine
