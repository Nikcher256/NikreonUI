#pragma once

#include "Engine/Renderer/Renderer2D.hpp"

#include <string>

namespace Engine {

class TextRenderer;
class UIContext;

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

    [[nodiscard]] float offset() const;
    [[nodiscard]] float maxOffset() const;

private:
    void clampOffset();
    [[nodiscard]] UIClipRect thumbBounds() const;

    std::string m_id;
    UIClipRect m_bounds;
    float m_contentHeight{0.0f};
    float m_offset{0.0f};
    float m_wheelStep{32.0f};
    float m_dragStartOffset{0.0f};
    float m_dragStartMouseY{0.0f};
    bool m_thumbWasHeld{false};
};

} // namespace Engine
