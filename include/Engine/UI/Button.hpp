#pragma once

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/UI/UIStyle.hpp"
#include "Engine/UI/Widget.hpp"

#include <functional>
#include <optional>

namespace Engine {

struct UIIconImage {
    UITextureId texture{0};
    glm::vec2 uvMinimum{0.0f, 0.0f};
    glm::vec2 uvMaximum{1.0f, 1.0f};
    glm::vec4 tint{1.0f, 1.0f, 1.0f, 1.0f};
};

enum class UIIcon {
    None,
    Play,
    Pause,
    Stop,
    ChevronDown,
    ChevronUp,
};

void drawUIIcon(Renderer2D& renderer2D, UIIcon icon, const glm::vec2& center, float size, const glm::vec4& color);

class Button final : public Widget {
public:
    explicit Button(std::string id);

    void update(UIContext& context) override;
    void render(Renderer2D& renderer2D, const UIStyle& style) const override;

    void setSelected(bool selected);
    void setStyle(const UIButtonStyle& style);
    void clearStyleOverride();
    void setOnClick(std::function<void()> callback);
    void setIcon(UIIcon icon);
    void setIconImage(const UIIconImage& icon);
    void clearIconImage();

    [[nodiscard]] bool selected() const;
    [[nodiscard]] UIIcon icon() const;
    [[nodiscard]] const std::optional<UIIconImage>& iconImage() const;

private:
    bool m_selected{false};
    UIIcon m_icon{UIIcon::None};
    std::optional<UIIconImage> m_iconImage;
    std::optional<UIButtonStyle> m_styleOverride;
    std::function<void()> m_onClick;
};

} // namespace Engine
