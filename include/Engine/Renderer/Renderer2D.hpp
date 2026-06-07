#pragma once

#include <cstdint>
#include <cstddef>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace Engine {

struct UIClipRect {
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 size{0.0f, 0.0f};
};

using UITextureId = std::uintptr_t;

class Renderer2D {
public:
    virtual ~Renderer2D() = default;

    virtual void begin(const glm::uvec2& viewportSize) = 0;
    [[nodiscard]] virtual std::uint64_t reserveRenderOrder() { return 0; }
    virtual void beginCompositeRenderItem(std::uint64_t renderOrder) { (void)renderOrder; }
    virtual void endCompositeRenderItem() {}
    virtual void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) = 0;
    virtual void drawGradientQuad(
        const glm::vec2& position,
        const glm::vec2& size,
        const glm::vec4& topLeft,
        const glm::vec4& topRight,
        const glm::vec4& bottomRight,
        const glm::vec4& bottomLeft)
    {
        // Fallback for simple/headless backends.
        drawQuad(position, size, (topLeft + topRight + bottomRight + bottomLeft) * 0.25f);
    }
    virtual void drawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, float thickness = 1.0f) = 0;
    virtual void drawSdfRect(
        const glm::vec2& position,
        const glm::vec2& size,
        float radius,
        const glm::vec4& fillColor,
        const glm::vec4& borderColor,
        float borderWidth = 0.0f) = 0;
    virtual void drawImage(
        UITextureId texture,
        const glm::vec2& position,
        const glm::vec2& size,
        const glm::vec2& uvMinimum = {0.0f, 0.0f},
        const glm::vec2& uvMaximum = {1.0f, 1.0f},
        const glm::vec4& tint = {1.0f, 1.0f, 1.0f, 1.0f})
    {
        (void)texture;
        (void)uvMinimum;
        (void)uvMaximum;
        drawQuad(position, size, tint);
    }
    virtual void uploadImage(
        UITextureId texture,
        std::uint32_t width,
        std::uint32_t height,
        const std::uint8_t* rgba8,
        std::size_t byteCount)
    {
        (void)texture;
        (void)width;
        (void)height;
        (void)rgba8;
        (void)byteCount;
    }
    virtual void pushClipRect(const UIClipRect& clipRect) = 0;
    virtual void popClipRect() = 0;
    virtual void end() = 0;
};

} // namespace Engine
