#pragma once

#include "Engine/Renderer/Renderer2D.hpp"

namespace Engine {

class TextRenderer;
class UIContext;

class ScrollContainer {
public:
    void setBounds(const UIClipRect& bounds);
    void setContentHeight(float contentHeight);
    void setWheelStep(float wheelStep);
    void update(const UIContext& context);
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

    UIClipRect m_bounds;
    float m_contentHeight{0.0f};
    float m_offset{0.0f};
    float m_wheelStep{32.0f};
};

} // namespace Engine
