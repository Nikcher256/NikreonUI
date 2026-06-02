#include "Engine/UI/BasicWidgets.hpp"

#include <algorithm>
#include <utility>

#include "Engine/UI/Layout.hpp"

namespace Engine {

Label::Label(std::string id, std::string text) : Widget(std::move(id)), m_text(std::move(text)) {}
void Label::render(const UIFrame& frame) const { if (m_visible) frame.drawText(m_text, {m_position, m_size}, frame.style().resolveText(m_styleClass, m_id)); }
void Label::setText(std::string text) { m_text = std::move(text); }

Panel::Panel(std::string id) : Widget(std::move(id)) {}
void Panel::render(Renderer2D& renderer, const UIStyle& style) const
{
    if (m_visible) renderer.drawSdfRect(m_position, m_size, style.panel.borderRadius, style.panel.fill, style.panel.border, style.panel.borderWidth);
}

Image::Image(std::string id, const UITextureId texture) : Widget(std::move(id)), m_texture(texture) {}
void Image::render(Renderer2D& renderer, const UIStyle&) const { if (m_visible) renderer.drawImage(m_texture, m_position, m_size, {}, {1.0f, 1.0f}, m_tint); }
void Image::setTexture(const UITextureId texture) { m_texture = texture; }
void Image::setTint(const glm::vec4& tint) { m_tint = tint; }

NineSlicePanel::NineSlicePanel(std::string id, const UITextureId texture) : Image(std::move(id), texture) {}
void NineSlicePanel::setBorder(const UIEdgeInsets& border) { m_border = border; }
void NineSlicePanel::render(Renderer2D& renderer, const UIStyle&) const
{
    if (!m_visible || m_size.x <= 0.0f || m_size.y <= 0.0f) return;
    const float left = std::min(m_border.left, m_size.x * 0.5f), right = std::min(m_border.right, m_size.x * 0.5f);
    const float top = std::min(m_border.top, m_size.y * 0.5f), bottom = std::min(m_border.bottom, m_size.y * 0.5f);
    const float xs[] = {0.0f, left, m_size.x - right, m_size.x};
    const float ys[] = {0.0f, top, m_size.y - bottom, m_size.y};
    const float us[] = {0.0f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f};
    for (int y = 0; y < 3; ++y) for (int x = 0; x < 3; ++x)
        renderer.drawImage(m_texture, m_position + glm::vec2{xs[x], ys[y]}, {xs[x + 1] - xs[x], ys[y + 1] - ys[y]}, {us[x], us[y]}, {us[x + 1], us[y + 1]}, m_tint);
}

ProgressBar::ProgressBar(std::string id, const float value) : Widget(std::move(id)) { setValue(value); }
void ProgressBar::render(Renderer2D& renderer, const UIStyle& style) const
{
    if (!m_visible) return;
    const auto& bar = style.progressBar;
    renderer.drawSdfRect(m_position, m_size, bar.track.borderRadius, bar.track.fill, bar.track.border, bar.track.borderWidth);
    renderer.drawSdfRect(m_position, {m_size.x * m_value, m_size.y}, bar.track.borderRadius, bar.fill, bar.fill, 0.0f);
}
void ProgressBar::setValue(const float value) { m_value = std::clamp(value, 0.0f, 1.0f); }
float ProgressBar::value() const { return m_value; }

} // namespace Engine
