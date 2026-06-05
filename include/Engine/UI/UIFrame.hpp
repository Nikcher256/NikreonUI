#pragma once

#include <string_view>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/Renderer/TextRenderer.hpp"
#include "Engine/UI/UIStyle.hpp"

namespace Engine {

class UIContext;

struct UISurface {
    glm::vec2 origin{0.0f, 0.0f};
    glm::vec2 size{0.0f, 0.0f};
};

class UIFrame {
public:
    UIFrame(UIContext& input, Renderer2D& shapes, TextRenderer& text, const UIStyle& style, UISurface surface = {});

    [[nodiscard]] UIContext& input() const;
    [[nodiscard]] Renderer2D& shapes() const;
    [[nodiscard]] TextRenderer& text() const;
    [[nodiscard]] const UIStyle& style() const;
    [[nodiscard]] const UISurface& surface() const;
    [[nodiscard]] glm::vec2 toScreen(const glm::vec2& localPosition) const;
    [[nodiscard]] UIClipRect toScreen(const UIClipRect& localRect) const;

    void drawBox(const UIClipRect& bounds, const UIBoxStyle& box) const;
    bool drawText(std::string_view value, const UIClipRect& bounds, const UITextStyle& textStyle) const;
    void pushClip(const UIClipRect& bounds) const;
    void popClip() const;
    void beginCompositeRenderItem() const;
    void endCompositeRenderItem() const;

private:
    UIContext* m_input;
    Renderer2D* m_shapes;
    TextRenderer* m_text;
    const UIStyle* m_style;
    UISurface m_surface;
};

class UICompositeRenderScope {
public:
    explicit UICompositeRenderScope(const UIFrame& frame);
    ~UICompositeRenderScope();

    UICompositeRenderScope(const UICompositeRenderScope&) = delete;
    UICompositeRenderScope& operator=(const UICompositeRenderScope&) = delete;

private:
    const UIFrame* m_frame;
};

} // namespace Engine
