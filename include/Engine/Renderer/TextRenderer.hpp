#pragma once

#include <filesystem>
#include <string_view>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Engine/Renderer/Renderer2D.hpp"

namespace Engine {

enum class TextAlignment {
    Left,
    Center,
    Right,
};

class TextRenderer {
public:
    virtual ~TextRenderer() = default;

    virtual void begin(const glm::uvec2& viewportSize) = 0;
    virtual bool loadFont(std::string_view name, const std::filesystem::path& path, float pixelSize) = 0;
    virtual void drawText(
        std::string_view text,
        const glm::vec2& position,
        const glm::vec4& color,
        std::string_view fontName = "default",
        float scale = 1.0f,
        TextAlignment alignment = TextAlignment::Left) = 0;
    [[nodiscard]] virtual glm::vec2 measureText(
        std::string_view text,
        std::string_view fontName = "default",
        float scale = 1.0f) const = 0;
    virtual void pushClipRect(const UIClipRect& clipRect) = 0;
    virtual void popClipRect() = 0;
    virtual void end() = 0;
};

} // namespace Engine
