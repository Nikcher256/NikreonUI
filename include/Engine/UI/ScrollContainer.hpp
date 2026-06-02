#pragma once

#include "Engine/Renderer/Renderer2D.hpp"

#include <string>
#include <memory>
#include <vector>

#include <glm/vec2.hpp>

namespace Engine {

class TextRenderer;
class UIContext;
class Widget;
struct UIStyle;

class ScrollContainer {
public:
    void setBounds(const UIClipRect& bounds);
    void setContentHeight(float contentHeight);
    void setWheelStep(float wheelStep);
    explicit ScrollContainer(std::string id = "scroll");

    void update(UIContext& context);
    void pushClip(UIContext& context) const;
    void popClip(UIContext& context) const;
    void pushClip(Renderer2D& renderer2D) const;
    void popClip(Renderer2D& renderer2D) const;
    void pushClip(TextRenderer& textRenderer) const;
    void popClip(TextRenderer& textRenderer) const;
    void renderScrollbar(Renderer2D& renderer2D) const;
    void renderScrollbar(Renderer2D& renderer2D, const UIStyle& style) const;
    Widget& addChild(std::unique_ptr<Widget> child, const glm::vec2& contentPosition);
    void clearChildren();
    void updateChildren(UIContext& context);
    void renderChildren(Renderer2D& renderer2D, const UIStyle& style);
    [[nodiscard]] glm::vec2 contentOrigin() const;

    [[nodiscard]] float offset() const;
    [[nodiscard]] float maxOffset() const;

private:
    void clampOffset();
    [[nodiscard]] UIClipRect thumbBounds() const;
    [[nodiscard]] UIClipRect thumbBounds(float width, float minimumThumbLength) const;

    struct Child {
        std::unique_ptr<Widget> widget;
        glm::vec2 contentPosition;
    };

    std::string m_id;
    UIClipRect m_bounds;
    float m_contentHeight{0.0f};
    float m_offset{0.0f};
    float m_wheelStep{32.0f};
    float m_dragStartOffset{0.0f};
    float m_dragStartMouseY{0.0f};
    bool m_thumbWasHeld{false};
    std::vector<Child> m_children;
};

} // namespace Engine
