#pragma once

#include "Engine/UI/UIStyle.hpp"
#include "Engine/UI/Widget.hpp"

#include <functional>
#include <optional>

namespace Engine {

class Button final : public Widget {
public:
    explicit Button(std::string id);

    void update(UIContext& context) override;
    void render(Renderer2D& renderer2D, const UIStyle& style) const override;

    void setSelected(bool selected);
    void setStyle(const UIButtonStyle& style);
    void clearStyleOverride();
    void setOnClick(std::function<void()> callback);

    [[nodiscard]] bool selected() const;

private:
    bool m_selected{false};
    std::optional<UIButtonStyle> m_styleOverride;
    std::function<void()> m_onClick;
};

} // namespace Engine
