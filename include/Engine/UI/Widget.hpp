#pragma once

#include <string>
#include <string_view>

#include <glm/vec2.hpp>

#include "Engine/UI/UIContext.hpp"

namespace Engine {

class Renderer2D;
struct UIStyle;

class Widget {
public:
    explicit Widget(std::string id);
    virtual ~Widget() = default;

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;
    Widget(Widget&&) noexcept = default;
    Widget& operator=(Widget&&) noexcept = default;

    virtual void update(UIContext& context);
    virtual void render(Renderer2D& renderer2D, const UIStyle& style) const = 0;

    void setBounds(const glm::vec2& position, const glm::vec2& size);
    void setVisible(bool visible);
    void setStyleClass(std::string styleClass);

    [[nodiscard]] std::string_view id() const;
    [[nodiscard]] std::string_view styleClass() const;
    [[nodiscard]] const glm::vec2& position() const;
    [[nodiscard]] const glm::vec2& size() const;
    [[nodiscard]] const UIInteraction& interaction() const;
    [[nodiscard]] bool visible() const;

protected:
    std::string m_id;
    std::string m_styleClass;
    glm::vec2 m_position{0.0f, 0.0f};
    glm::vec2 m_size{0.0f, 0.0f};
    UIInteraction m_interaction;
    bool m_visible{true};
};

} // namespace Engine
