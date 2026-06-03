#pragma once

#include "Engine/UI/Slider.hpp"
#include "Engine/UI/TextInput.hpp"
#include "Engine/UI/Widget.hpp"

#include <functional>

#include <glm/vec4.hpp>

namespace Engine {

class TextRenderer;

class ColorPicker final : public Widget {
public:
    ColorPicker(std::string id, const glm::vec4& color = {1.0f, 1.0f, 1.0f, 1.0f});

    void update(UIContext& context) override;
    void render(Renderer2D& renderer2D, const UIStyle& style) const override;
    void updatePopup(UIContext& context);
    void updatePopup(UIContext& context, const TextRenderer& textRenderer, const UIStyle& style);
    void renderPopup(UIContext& context, Renderer2D& renderer2D, TextRenderer& textRenderer, const UIStyle& style) const;
    void registerPopupLayer(UIContext& context, int zIndex = 100, bool modal = false) const;

    void setColor(const glm::vec4& color);
    void setOnColorChanged(std::function<void(const glm::vec4&)> callback);

    [[nodiscard]] const glm::vec4& color() const;
    [[nodiscard]] bool popupOpen() const;

private:
    void updateSaturationValue(UIContext& context);
    void updateHue(UIContext& context);
    void updateChannel(UIContext& context, std::string_view suffix, float y, float& value);
    void layoutChannelInputs();
    void syncChannelInputs();
    void syncHexInput();
    void notifyColorChanged();
    void updateHsvFromColor();
    [[nodiscard]] glm::vec2 popupPosition() const;

    glm::vec4 m_color{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 m_originalColor{1.0f, 1.0f, 1.0f, 1.0f};
    float m_hue{0.0f};
    float m_saturation{0.0f};
    float m_value{1.0f};
    bool m_popupOpen{false};
    Slider m_redSlider;
    Slider m_greenSlider;
    Slider m_blueSlider;
    TextInput m_hexInput;
    bool m_syncingInputs{false};
    std::function<void(const glm::vec4&)> m_onColorChanged;
};

} // namespace Engine
