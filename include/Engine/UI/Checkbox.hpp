#pragma once

#include "Engine/UI/UIStyle.hpp"
#include "Engine/UI/Widget.hpp"

#include <functional>
#include <optional>

namespace Engine {

class Checkbox final : public Widget {
public:
    Checkbox(std::string id, bool checked = false);

    void update(UIContext& context) override;
    void render(Renderer2D& renderer2D, const UIStyle& style) const override;

    void setChecked(bool checked);
    void setStyle(const UICheckboxStyle& style);
    void clearStyleOverride();
    void setOnValueChanged(std::function<void(bool)> callback);

    [[nodiscard]] bool checked() const;

private:
    bool m_checked{false};
    std::optional<UICheckboxStyle> m_styleOverride;
    std::function<void(bool)> m_onValueChanged;
};

} // namespace Engine
