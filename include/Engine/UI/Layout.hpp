#pragma once

#include <optional>
#include <vector>

#include <glm/vec2.hpp>

namespace Engine {

class Widget;

struct UIRect {
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 size{0.0f, 0.0f};
};

struct UIEdgeInsets {
    float left{0.0f};
    float top{0.0f};
    float right{0.0f};
    float bottom{0.0f};

    [[nodiscard]] static UIEdgeInsets all(float value);
    [[nodiscard]] static UIEdgeInsets symmetric(float horizontal, float vertical);
};

enum class UILayoutAxis {
    Horizontal,
    Vertical,
};

enum class UIAlignment {
    Start,
    Center,
    End,
    Stretch,
};

enum class UIDock {
    Left,
    Top,
    Right,
    Bottom,
    Fill,
};

struct UIAnchors {
    glm::vec2 minimum{0.0f, 0.0f};
    glm::vec2 maximum{0.0f, 0.0f};
    glm::vec2 offsetMinimum{0.0f, 0.0f};
    glm::vec2 offsetMaximum{0.0f, 0.0f};

    [[nodiscard]] static UIAnchors fill();
    [[nodiscard]] static UIAnchors fixed(const glm::vec2& anchor, const glm::vec2& offset, const glm::vec2& size);
    [[nodiscard]] static UIAnchors horizontalStretch(float top, float height);
};

class UILayoutTarget {
public:
    UILayoutTarget(UIRect& rect);
    UILayoutTarget(Widget& widget);

    void setBounds(const UIRect& bounds) const;

private:
    UIRect* m_rect{nullptr};
    Widget* m_widget{nullptr};
};

class UILinearLayout {
public:
    explicit UILinearLayout(UILayoutAxis axis);

    void setBounds(const UIRect& bounds);
    void setPadding(const UIEdgeInsets& padding);
    void setGap(float gap);
    void setAlignment(UIAlignment alignment);
    void add(
        const UILayoutTarget& target,
        const glm::vec2& preferredSize,
        float grow = 0.0f,
        const UIEdgeInsets& margin = {},
        std::optional<UIAlignment> alignment = std::nullopt);
    void clear();
    void layout() const;

private:
    struct Item {
        UILayoutTarget target;
        glm::vec2 preferredSize{0.0f, 0.0f};
        float grow{0.0f};
        UIEdgeInsets margin;
        std::optional<UIAlignment> alignment;
    };

    UILayoutAxis m_axis;
    UIRect m_bounds;
    UIEdgeInsets m_padding;
    float m_gap{0.0f};
    UIAlignment m_alignment{UIAlignment::Stretch};
    std::vector<Item> m_items;
};

class UIDockLayout {
public:
    void setBounds(const UIRect& bounds);
    void setPadding(const UIEdgeInsets& padding);
    void setGap(float gap);
    void add(const UILayoutTarget& target, UIDock dock, float extent = 0.0f, const UIEdgeInsets& margin = {});
    void clear();
    void layout() const;

private:
    struct Item {
        UILayoutTarget target;
        UIDock dock{UIDock::Fill};
        float extent{0.0f};
        UIEdgeInsets margin;
    };

    UIRect m_bounds;
    UIEdgeInsets m_padding;
    float m_gap{0.0f};
    std::vector<Item> m_items;
};

class UIStackLayout {
public:
    void setBounds(const UIRect& bounds);
    void setPadding(const UIEdgeInsets& padding);
    void add(const UILayoutTarget& target, const UIAnchors& anchors, const UIEdgeInsets& margin = {});
    void clear();
    void layout() const;

private:
    struct Item {
        UILayoutTarget target;
        UIAnchors anchors;
        UIEdgeInsets margin;
    };

    UIRect m_bounds;
    UIEdgeInsets m_padding;
    std::vector<Item> m_items;
};

} // namespace Engine
