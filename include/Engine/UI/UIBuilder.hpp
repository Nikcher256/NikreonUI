#pragma once

#include "Engine/UI/BasicWidgets.hpp"
#include "Engine/UI/Button.hpp"
#include "Engine/UI/Checkbox.hpp"
#include "Engine/UI/ColorPicker.hpp"
#include "Engine/UI/Dropdown.hpp"
#include "Engine/UI/FilePathInput.hpp"
#include "Engine/UI/NumberInput.hpp"
#include "Engine/UI/ScrollContainer.hpp"
#include "Engine/UI/Slider.hpp"
#include "Engine/UI/TextInput.hpp"
#include "Engine/UI/UIFrame.hpp"
#include "Engine/UI/UIRenderQueue.hpp"
#include "Engine/UI/UIStyle.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace Engine {

enum class UIContainerLayout {
    DockLayout,
    VerticalLayout,
    HorizontalLayout,
    GridLayout,
    OverlayLayout,
};

enum class UILabelPlacement {
    None,
    Top,
    Left,
    Right,
    Inside,
};

class UIBuilder {
private:
    struct Element;

public:
    class ElementBuilder {
    public:
        ElementBuilder(UIBuilder& builder, std::string id);

        ElementBuilder& parent(std::string_view parentId);
        ElementBuilder& styleClass(std::string_view styleClass);
        ElementBuilder& visible(bool visible);
        ElementBuilder& dock(UIDock dock);
        ElementBuilder& layout(UIContainerLayout layout);
        ElementBuilder& vertical();
        ElementBuilder& horizontal();
        ElementBuilder& grid(int columns);
        ElementBuilder& overlay();
        ElementBuilder& scrollable(bool scrollable);
        ElementBuilder& drawBackground(bool drawBackground);
        ElementBuilder& width(float width);
        ElementBuilder& height(float height);
        ElementBuilder& preferredSize(const glm::vec2& size);
        ElementBuilder& popupSize(const glm::vec2& size);
        ElementBuilder& grow(float grow);
        ElementBuilder& padding(const UIEdgeInsets& padding);
        ElementBuilder& gap(float gap);
        ElementBuilder& label(std::string_view label);
        ElementBuilder& labelStyle(std::string_view styleClass);
        ElementBuilder& labelPlacement(UILabelPlacement placement);
        ElementBuilder& text(std::string_view text);
        ElementBuilder& textStyle(std::string_view styleClass);
        ElementBuilder& tooltip(std::string_view tooltip);
        ElementBuilder& selected(bool selected);
        ElementBuilder& checked(bool checked);
        ElementBuilder& value(float value);
        ElementBuilder& value(std::string_view value);
        ElementBuilder& range(float minValue, float maxValue);
        ElementBuilder& precision(int precision);
        ElementBuilder& sensitivity(float sensitivity);
        ElementBuilder& placeholder(std::string_view placeholder);
        ElementBuilder& items(std::vector<std::string> items);
        ElementBuilder& selectedIndex(std::size_t selectedIndex);
        ElementBuilder& color(const glm::vec4& color);
        ElementBuilder& buttonLabel(std::string_view label);
        ElementBuilder& browseButtonWidth(float width);
        ElementBuilder& onClick(std::function<void()> callback);
        ElementBuilder& onChanged(std::function<void(float)> callback);
        ElementBuilder& onCheckedChanged(std::function<void(bool)> callback);
        ElementBuilder& onTextChanged(std::function<void(std::string_view)> callback);
        ElementBuilder& onBrowse(std::function<void()> callback);
        ElementBuilder& onSelectionChanged(std::function<void(std::size_t, std::string_view)> callback);
        ElementBuilder& onColorChanged(std::function<void(const glm::vec4&)> callback);

    private:
        [[nodiscard]] Element& element() const;

        UIBuilder* m_builder;
        std::string m_id;
    };

    void begin(UIContext& context, Renderer2D& renderer, TextRenderer& text, const UIStyle& style, UISurface surface);

    ElementBuilder panel(std::string_view id);
    ElementBuilder label(std::string_view id);
    ElementBuilder button(std::string_view id);
    ElementBuilder checkbox(std::string_view id);
    ElementBuilder sliderFloat(std::string_view id);
    ElementBuilder numberFloat(std::string_view id);
    ElementBuilder textInput(std::string_view id);
    ElementBuilder filePathInput(std::string_view id);
    ElementBuilder dropdown(std::string_view id);
    ElementBuilder colorPicker(std::string_view id);

    void layout();
    void update();
    void render();
    void renderBaseLayer();
    void renderTopLayer();
    void end();
    void closeDropdown(std::string_view id);

    [[nodiscard]] UIRect bounds(std::string_view id) const;
    [[nodiscard]] bool visible(std::string_view id) const;

private:
    enum class ElementType {
        Panel,
        Label,
        Button,
        Checkbox,
        SliderFloat,
        NumberFloat,
        TextInput,
        FilePathInput,
        Dropdown,
        ColorPicker,
    };

    struct Element {
        std::string id;
        ElementType type{ElementType::Panel};
        std::string parentId;
        std::string styleClass;
        std::string label;
        std::string labelStyleClass{"control-label"};
        UILabelPlacement labelPlacement{UILabelPlacement::Top};
        std::string text;
        std::string textStyleClass{"toolbar-toggle"};
        std::string tooltip;
        bool visible{true};
        bool drawBackground{true};
        bool scrollable{false};
        UIContainerLayout layout{UIContainerLayout::VerticalLayout};
        UIDock dock{UIDock::Fill};
        bool docked{false};
        float width{0.0f};
        float height{0.0f};
        glm::vec2 preferredSize{0.0f, 0.0f};
        glm::vec2 popupSize{0.0f, 0.0f};
        float grow{0.0f};
        std::optional<UIEdgeInsets> padding;
        std::optional<float> gap;
        int gridColumns{1};
        float floatValue{0.0f};
        float minValue{0.0f};
        float maxValue{1.0f};
        float sensitivity{0.01f};
        int precision{2};
        bool checked{false};
        bool selected{false};
        std::string stringValue;
        std::string placeholder;
        std::vector<std::string> items;
        std::size_t selectedIndex{0};
        glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
        std::string buttonLabel{"Browse"};
        float browseButtonWidth{72.0f};
        std::function<void()> onClick;
        std::function<void(float)> onFloatChanged;
        std::function<void(bool)> onBoolChanged;
        std::function<void(std::string_view)> onTextChanged;
        std::function<void()> onBrowse;
        std::function<void(std::size_t, std::string_view)> onSelectionChanged;
        std::function<void(const glm::vec4&)> onColorChanged;
        UIRect bounds;
        UIRect labelBounds;
        UIRect textBounds;
    };

    ElementBuilder makeElement(std::string_view id, ElementType type);
    [[nodiscard]] Element& element(std::string_view id);
    [[nodiscard]] const Element* findElement(std::string_view id) const;
    [[nodiscard]] std::vector<std::string> childrenOf(std::string_view parentId) const;
    [[nodiscard]] bool hasPopupChildren(const Element& element) const;
    [[nodiscard]] bool isVisible(const Element& element) const;
    [[nodiscard]] std::string typeName(const Element& element) const;
    [[nodiscard]] UIComputedStyle computedStyle(const Element& element) const;
    [[nodiscard]] UIEdgeInsets paddingFor(const Element& element) const;
    [[nodiscard]] float gapFor(const Element& element) const;
    [[nodiscard]] glm::vec2 preferredSizeFor(const Element& element) const;
    [[nodiscard]] Widget* widgetFor(Element& element);
    [[nodiscard]] const Widget* widgetFor(const Element& element) const;
    [[nodiscard]] ScrollContainer* scrollFor(const Element& element);
    [[nodiscard]] const ScrollContainer* scrollFor(const Element& element) const;

    void layoutRoot();
    void layoutChildren(Element& parent);
    void layoutVertical(Element& parent, const UIRect& contentBounds, const std::vector<std::string>& children);
    void layoutHorizontal(Element& parent, const UIRect& contentBounds, const std::vector<std::string>& children);
    void layoutGrid(Element& parent, const UIRect& contentBounds, const std::vector<std::string>& children);
    void assignControlBounds(Element& element, const UIRect& itemBounds);
    void applyWidgetState(Element& element);
    void updateElement(Element& element);
    void updateChildren(Element& parent);
    void registerPopupLayers();
    void updatePopups();
    void renderBase(UIFrame& frame);
    void renderElement(UIFrame& frame, Element& element);
    void renderChildren(UIFrame& frame, Element& parent);
    void renderWidget(UIFrame& frame, Element& element);
    void renderElementText(UIFrame& frame, Element& element);
    void renderPopups();
    void renderTooltip(std::string_view text);

    UIContext* m_context{nullptr};
    Renderer2D* m_renderer{nullptr};
    TextRenderer* m_text{nullptr};
    const UIStyle* m_style{nullptr};
    UISurface m_surface;
    std::unordered_map<std::string, Element> m_elements;
    std::vector<std::string> m_order;
    UIRenderQueue m_renderQueue;
    std::string m_tooltipText;
    std::unordered_map<std::string, std::unique_ptr<Button>> m_buttons;
    std::unordered_map<std::string, std::unique_ptr<Checkbox>> m_checkboxes;
    std::unordered_map<std::string, std::unique_ptr<Slider>> m_sliders;
    std::unordered_map<std::string, std::unique_ptr<NumberInput>> m_numberInputs;
    std::unordered_map<std::string, std::unique_ptr<TextInput>> m_textInputs;
    std::unordered_map<std::string, std::unique_ptr<FilePathInput>> m_filePathInputs;
    std::unordered_map<std::string, std::unique_ptr<Dropdown>> m_dropdowns;
    std::unordered_map<std::string, std::unique_ptr<ColorPicker>> m_colorPickers;
    std::unordered_map<std::string, std::unique_ptr<ScrollContainer>> m_scrollAreas;
};

} // namespace Engine
