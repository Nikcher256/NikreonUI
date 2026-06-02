#pragma once

#include "Engine/UI/Widget.hpp"

#include <functional>

#include <glm/vec4.hpp>

namespace Engine {

class ColorPicker final : public Widget {
public:
    ColorPicker(std::string id, const glm::vec4& color = {1.0f, 1.0f, 1.0f, 1.0f});

    void update(UIContext& context) override;
    void render(Renderer2D& renderer2D, const UIStyle& style) const override;
    void updatePopup(UIContext& context);
    void renderPopup(Renderer2D& renderer2D, const UIStyle& style) const;

    void setColor(const glm::vec4& color);
    void setOnColorChanged(std::function<void(const glm::vec4&)> callback);

    [[nodiscard]] const glm::vec4& color() const;
    [[nodiscard]] bool popupOpen() const;

private:
    void updateSaturationValue(UIContext& context);
    void updateHue(UIContext& context);
    void updateChannel(UIContext& context, std::string_view suffix, float y, float& value);
    void notifyColorChanged();
    void updateHsvFromColor();
    [[nodiscard]] glm::vec2 popupPosition() const;

    glm::vec4 m_color{1.0f, 1.0f, 1.0f, 1.0f};
    float m_hue{0.0f};
    float m_saturation{0.0f};
    float m_value{1.0f};
    bool m_popupOpen{false};
    std::function<void(const glm::vec4&)> m_onColorChanged;
};

} // namespace Engine
