#pragma once

#include "Engine/UI/Button.hpp"
#include "Engine/UI/TextInput.hpp"

#include <functional>
#include <string>
#include <string_view>

namespace Engine {

class UIFrame;

class FilePathInput final : public Widget {
public:
    explicit FilePathInput(std::string id, std::string value = {});

    void update(UIContext& context) override;
    void update(UIContext& context, const TextRenderer& textRenderer, const UIStyle& style, std::string_view fontName = "default", float scale = 1.0f);
    void render(Renderer2D& renderer2D, const UIStyle& style) const override;
    void render(const UIFrame& frame) const;

    void setValue(std::string value);
    void setPlaceholder(std::string placeholder);
    void setButtonLabel(std::string label);
    void setBrowseButtonWidth(float width);
    void setGap(float gap);
    void setKeepPathEndVisible(bool keepPathEndVisible);
    void setOnValueChanged(std::function<void(std::string_view)> callback);
    void setOnBrowse(std::function<void()> callback);
    void setInputStyleClass(std::string styleClass);
    void setButtonStyleClass(std::string styleClass);

    [[nodiscard]] const std::string& value() const;
    [[nodiscard]] const std::string& placeholder() const;
    [[nodiscard]] const glm::vec2& inputPosition() const;
    [[nodiscard]] const glm::vec2& inputSize() const;
    [[nodiscard]] const glm::vec2& buttonPosition() const;
    [[nodiscard]] const glm::vec2& buttonSize() const;

private:
    void layoutChildren();

    TextInput m_input;
    Button m_browseButton;
    std::string m_buttonLabel{"Browse"};
    float m_browseButtonWidth{72.0f};
    float m_gap{8.0f};
    bool m_keepPathEndVisible{true};
};

} // namespace Engine
