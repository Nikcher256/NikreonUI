#pragma once

#include <string>

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/UI/Layout.hpp"
#include "Engine/UI/UIFrame.hpp"
#include "Engine/UI/Widget.hpp"

namespace Engine {

class Label final : public Widget {
public:
    Label(std::string id, std::string text = {});
    void render(Renderer2D&, const UIStyle&) const override {}
    void render(const UIFrame& frame) const;
    void setText(std::string text);
private:
    std::string m_text;
};

class Panel final : public Widget {
public:
    explicit Panel(std::string id);
    void render(Renderer2D& renderer, const UIStyle& style) const override;
};

class Image : public Widget {
public:
    Image(std::string id, UITextureId texture = 0);
    void render(Renderer2D& renderer, const UIStyle&) const override;
    void setTexture(UITextureId texture);
    void setTint(const glm::vec4& tint);
protected:
    UITextureId m_texture{0};
    glm::vec4 m_tint{1.0f};
};

using Icon = Image;

class NineSlicePanel final : public Image {
public:
    NineSlicePanel(std::string id, UITextureId texture = 0);
    void render(Renderer2D& renderer, const UIStyle&) const override;
    void setBorder(const UIEdgeInsets& border);
private:
    UIEdgeInsets m_border{8.0f, 8.0f, 8.0f, 8.0f};
};

class ProgressBar final : public Widget {
public:
    explicit ProgressBar(std::string id, float value = 0.0f);
    void render(Renderer2D& renderer, const UIStyle& style) const override;
    void setValue(float value);
    [[nodiscard]] float value() const;
private:
    float m_value{0.0f};
};

} // namespace Engine
