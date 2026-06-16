#pragma once

#include "Engine/UI/Button.hpp"
#include "Engine/UI/Layout.hpp"
#include "Engine/UI/Widget.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Engine {

class TextRenderer;

class Dropdown final: public Widget {
public:
    explicit Dropdown(std::string id, std::vector<std::string> items = {}, std::size_t selectedIndex = 0);

    void update(UIContext& context) override;
    void render(Renderer2D& renderer2D, const UIStyle& style) const override;

    void updatePopup(UIContext& context);
    void renderPopup(UIContext& context, Renderer2D& renderer2D, TextRenderer& textRenderer, const UIStyle& style) const;
    void registerPopupLayer(UIContext& context, int zIndex = 100, bool modal = false) const;

    void setItems(std::vector<std::string> items);
    void setSelectedIndex(std::size_t selectedIndex);
    void setPopupSize(const glm::vec2& size);
    void clearPopupSize();
    void setArrowIconImages(const UIIconImage& collapsed, const UIIconImage& expanded);
    void clearArrowIconImages();
    void close();
    void setOnSelectionChanged(std::function<void(std::size_t, std::string_view)> callback);

    [[nodiscard]] std::size_t selectedIndex() const;
    [[nodiscard]] std::string_view selectedText() const;
    [[nodiscard]] bool popupOpen() const;
    [[nodiscard]] UIRect popupBounds() const;

private:
    [[nodiscard]] glm::vec2 popupPosition() const;
    [[nodiscard]] glm::vec2 popupSize() const;
    void notifySelectionChanged();

    std::vector<std::string> m_items;
    std::size_t m_selectedIndex{0};
    int m_hoveredIndex{-1};
    bool m_popupOpen{false};
    float m_rowHeight{26.0f};
    std::optional<glm::vec2> m_popupSizeOverride;
    std::optional<UIIconImage> m_collapsedArrowIcon;
    std::optional<UIIconImage> m_expandedArrowIcon;
    std::function<void(std::size_t, std::string_view)> m_onSelectionChanged;
};

} // namespace Engine
