#include "Engine/UI/UIBuilder.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include <glm/common.hpp>

namespace Engine {

namespace {

UIRect inset(const UIRect& bounds, const UIEdgeInsets& padding)
{
    return {
        {bounds.position.x + padding.left, bounds.position.y + padding.top},
        {
            std::max(0.0f, bounds.size.x - padding.left - padding.right),
            std::max(0.0f, bounds.size.y - padding.top - padding.bottom),
        },
    };
}

template <typename T, typename... Args>
T* ensureWidget(std::unordered_map<std::string, std::unique_ptr<T>>& widgets, const std::string& id, Args&&... args)
{
    auto& widget = widgets[id];
    if (!widget) {
        widget = std::make_unique<T>(id, std::forward<Args>(args)...);
    }
    return widget.get();
}

float labelHeight()
{
    return 20.0f;
}

float labelGap()
{
    return 4.0f;
}

} // namespace

UIBuilder::ElementBuilder::ElementBuilder(UIBuilder& builder, std::string id)
    : m_builder(&builder)
    , m_id(std::move(id))
{
}

UIBuilder::Element& UIBuilder::ElementBuilder::element() const
{
    return m_builder->element(m_id);
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::parent(const std::string_view parentId)
{
    element().parentId = std::string(parentId);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::styleClass(const std::string_view styleClass)
{
    element().styleClass = std::string(styleClass);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::visible(const bool visible)
{
    element().visible = visible;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::dock(const UIDock dock)
{
    Element& target = element();
    target.dock = dock;
    target.docked = true;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::layout(const UIContainerLayout layout)
{
    element().layout = layout;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::vertical()
{
    return layout(UIContainerLayout::VerticalLayout);
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::horizontal()
{
    return layout(UIContainerLayout::HorizontalLayout);
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::grid(const int columns)
{
    Element& target = element();
    target.layout = UIContainerLayout::GridLayout;
    target.gridColumns = std::max(1, columns);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::overlay()
{
    return layout(UIContainerLayout::OverlayLayout);
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::scrollable(const bool scrollable)
{
    element().scrollable = scrollable;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::drawBackground(const bool drawBackground)
{
    element().drawBackground = drawBackground;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::width(const float width)
{
    element().width = std::max(0.0f, width);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::height(const float height)
{
    element().height = std::max(0.0f, height);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::preferredSize(const glm::vec2& size)
{
    element().preferredSize = glm::max(size, glm::vec2{0.0f, 0.0f});
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::popupSize(const glm::vec2& size)
{
    element().popupSize = glm::max(size, glm::vec2{0.0f, 0.0f});
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::grow(const float grow)
{
    element().grow = std::max(0.0f, grow);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::padding(const UIEdgeInsets& padding)
{
    element().padding = padding;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::gap(const float gap)
{
    element().gap = std::max(0.0f, gap);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::label(const std::string_view label)
{
    element().label = std::string(label);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::labelStyle(const std::string_view styleClass)
{
    element().labelStyleClass = std::string(styleClass);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::labelPlacement(const UILabelPlacement placement)
{
    element().labelPlacement = placement;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::text(const std::string_view text)
{
    element().text = std::string(text);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::textStyle(const std::string_view styleClass)
{
    element().textStyleClass = std::string(styleClass);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::icon(const UIIcon icon)
{
    element().icon = icon;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::iconImage(const UIIconImage& icon)
{
    element().iconImage = icon;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::dropdownIconImages(const UIIconImage& collapsed, const UIIconImage& expanded)
{
    Element& target = element();
    target.dropdownCollapsedIcon = collapsed;
    target.dropdownExpandedIcon = expanded;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::tooltip(const std::string_view tooltip)
{
    element().tooltip = std::string(tooltip);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::selected(const bool selected)
{
    element().selected = selected;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::checked(const bool checked)
{
    element().checked = checked;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::value(const float value)
{
    element().floatValue = value;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::value(const std::string_view value)
{
    element().stringValue = std::string(value);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::range(const float minValue, const float maxValue)
{
    Element& target = element();
    target.minValue = minValue;
    target.maxValue = maxValue;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::precision(const int precision)
{
    element().precision = std::clamp(precision, 0, 6);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::sensitivity(const float sensitivity)
{
    element().sensitivity = std::max(0.0f, sensitivity);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::axis(const UINumberAxis axis)
{
    element().numberAxis = axis;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::placeholder(const std::string_view placeholder)
{
    element().placeholder = std::string(placeholder);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::items(std::vector<std::string> items)
{
    element().items = std::move(items);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::selectedIndex(const std::size_t selectedIndex)
{
    element().selectedIndex = selectedIndex;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::color(const glm::vec4& color)
{
    element().color = color;
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::buttonLabel(const std::string_view label)
{
    element().buttonLabel = std::string(label);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::browseButtonWidth(const float width)
{
    element().browseButtonWidth = std::max(28.0f, width);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::onClick(std::function<void()> callback)
{
    element().onClick = std::move(callback);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::onChanged(std::function<void(float)> callback)
{
    element().onFloatChanged = std::move(callback);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::onCheckedChanged(std::function<void(bool)> callback)
{
    element().onBoolChanged = std::move(callback);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::onTextChanged(std::function<void(std::string_view)> callback)
{
    element().onTextChanged = std::move(callback);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::onBrowse(std::function<void()> callback)
{
    element().onBrowse = std::move(callback);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::onSelectionChanged(std::function<void(std::size_t, std::string_view)> callback)
{
    element().onSelectionChanged = std::move(callback);
    return *this;
}

UIBuilder::ElementBuilder& UIBuilder::ElementBuilder::onColorChanged(std::function<void(const glm::vec4&)> callback)
{
    element().onColorChanged = std::move(callback);
    return *this;
}

void UIBuilder::begin(UIContext& context, Renderer2D& renderer, TextRenderer& text, const UIStyle& style, const UISurface surface)
{
    m_context = &context;
    m_renderer = &renderer;
    m_text = &text;
    m_style = &style;
    m_surface = surface;
    m_elements.clear();
    m_order.clear();
    m_renderQueue.clear();
}

UIBuilder::ElementBuilder UIBuilder::panel(const std::string_view id) { return makeElement(id, ElementType::Panel); }
UIBuilder::ElementBuilder UIBuilder::label(const std::string_view id) { return makeElement(id, ElementType::Label); }
UIBuilder::ElementBuilder UIBuilder::button(const std::string_view id) { return makeElement(id, ElementType::Button); }
UIBuilder::ElementBuilder UIBuilder::checkbox(const std::string_view id) { return makeElement(id, ElementType::Checkbox); }
UIBuilder::ElementBuilder UIBuilder::sliderFloat(const std::string_view id) { return makeElement(id, ElementType::SliderFloat); }
UIBuilder::ElementBuilder UIBuilder::numberFloat(const std::string_view id) { return makeElement(id, ElementType::NumberFloat); }
UIBuilder::ElementBuilder UIBuilder::textInput(const std::string_view id) { return makeElement(id, ElementType::TextInput); }
UIBuilder::ElementBuilder UIBuilder::filePathInput(const std::string_view id) { return makeElement(id, ElementType::FilePathInput); }
UIBuilder::ElementBuilder UIBuilder::dropdown(const std::string_view id) { return makeElement(id, ElementType::Dropdown); }
UIBuilder::ElementBuilder UIBuilder::colorPicker(const std::string_view id) { return makeElement(id, ElementType::ColorPicker); }

void UIBuilder::layout()
{
    layoutRoot();
}

void UIBuilder::update()
{
    for (const std::string& id : m_order) {
        Element& target = element(id);
        if (target.scrollable && isVisible(target)) {
            if (ScrollContainer* scroll = scrollFor(target)) {
                scroll->update(*m_context);
            }
        }
    }

    layout();

    for (const std::string& id : m_order) {
        applyWidgetState(element(id));
    }

    registerPopupLayers();

    for (const std::string& id : m_order) {
        Element& root = element(id);
        if (root.parentId.empty()) {
            updateElement(root);
        }
    }

    updatePopups();
}

void UIBuilder::render()
{
    renderBaseLayer();
    renderTopLayer();
}

void UIBuilder::renderBaseLayer()
{
    UIFrame frame{*m_context, *m_renderer, *m_text, *m_style, m_surface};
    m_renderQueue.add(UIRenderLayer::Background, [this, &frame]() {
        const UICompositeRenderScope compositeScope{frame};
        renderBase(frame);
    });
    m_renderQueue.flush();
}

void UIBuilder::renderTopLayer()
{
    UIFrame frame{*m_context, *m_renderer, *m_text, *m_style, m_surface};
    m_renderQueue.add(UIRenderLayer::Popup, [this, &frame]() {
        const UICompositeRenderScope compositeScope{frame};
        renderPopups();
    });
    m_renderQueue.add(UIRenderLayer::Tooltip, [this, &frame]() {
        if (m_context->hasTooltip()) {
            const UICompositeRenderScope compositeScope{frame};
            renderTooltip(m_context->tooltipText());
        }
    });
    m_renderQueue.flush();
}

void UIBuilder::end()
{
    m_context = nullptr;
    m_renderer = nullptr;
    m_text = nullptr;
    m_style = nullptr;
}

void UIBuilder::closeDropdown(const std::string_view id)
{
    const auto found = m_dropdowns.find(std::string(id));
    if (found != m_dropdowns.end() && found->second) {
        found->second->close();
    }
}

UIRect UIBuilder::bounds(const std::string_view id) const
{
    if (const Element* target = findElement(id)) {
        return target->bounds;
    }
    return {};
}

bool UIBuilder::visible(const std::string_view id) const
{
    if (const Element* target = findElement(id)) {
        return isVisible(*target);
    }
    return false;
}

UIBuilder::ElementBuilder UIBuilder::makeElement(const std::string_view id, const ElementType type)
{
    const std::string key{id};
    auto [found, inserted] = m_elements.try_emplace(key);
    if (inserted) {
        found->second.id = key;
        m_order.push_back(key);
    }
    found->second.type = type;
    return ElementBuilder{*this, key};
}

UIBuilder::Element& UIBuilder::element(const std::string_view id)
{
    return m_elements.at(std::string(id));
}

const UIBuilder::Element* UIBuilder::findElement(const std::string_view id) const
{
    const auto found = m_elements.find(std::string(id));
    return found == m_elements.end() ? nullptr : &found->second;
}

std::vector<std::string> UIBuilder::childrenOf(const std::string_view parentId) const
{
    std::vector<std::string> children;
    for (const std::string& id : m_order) {
        const auto found = m_elements.find(id);
        if (found != m_elements.end() && found->second.parentId == parentId) {
            children.push_back(id);
        }
    }
    return children;
}

bool UIBuilder::hasPopupChildren(const Element& target) const
{
    return target.type == ElementType::Dropdown && !childrenOf(target.id).empty();
}

bool UIBuilder::isVisible(const Element& target) const
{
    if (!target.visible) {
        return false;
    }
    if (target.parentId.empty()) {
        return true;
    }
    const Element* parent = findElement(target.parentId);
    return parent == nullptr || isVisible(*parent);
}

std::string UIBuilder::typeName(const Element& target) const
{
    switch (target.type) {
    case ElementType::Panel: return "panel";
    case ElementType::Label: return "text";
    case ElementType::Button: return "button";
    case ElementType::Checkbox: return "checkbox";
    case ElementType::SliderFloat: return "slider";
    case ElementType::NumberFloat: return "number-input";
    case ElementType::TextInput: return "text-input";
    case ElementType::FilePathInput: return "file-path-input";
    case ElementType::Dropdown: return "dropdown";
    case ElementType::ColorPicker: return "color-picker";
    }
    return "panel";
}

UIComputedStyle UIBuilder::computedStyle(const Element& target) const
{
    return m_style->resolveElement(typeName(target), target.styleClass, target.id);
}

UIEdgeInsets UIBuilder::paddingFor(const Element& target) const
{
    if (target.padding) {
        return *target.padding;
    }
    return computedStyle(target).padding;
}

float UIBuilder::gapFor(const Element& target) const
{
    if (target.gap) {
        return *target.gap;
    }
    return computedStyle(target).gap;
}

glm::vec2 UIBuilder::preferredSizeFor(const Element& target) const
{
    glm::vec2 size = target.preferredSize;
    if (size.x <= 0.0f) {
        size.x = target.width;
    }
    if (size.y <= 0.0f) {
        size.y = target.height;
    }

    if (size.y <= 0.0f) {
        switch (target.type) {
        case ElementType::Label: size.y = labelHeight(); break;
        case ElementType::Checkbox: size.y = 22.0f; break;
        case ElementType::SliderFloat: size.y = 24.0f; break;
        case ElementType::ColorPicker: size.y = 30.0f; break;
        case ElementType::Panel: size.y = 0.0f; break;
        default: size.y = 28.0f; break;
        }
    }

    if (size.x <= 0.0f) {
        switch (target.type) {
        case ElementType::Checkbox: size.x = 22.0f; break;
        case ElementType::ColorPicker: size.x = 30.0f; break;
        case ElementType::Button: size.x = 72.0f; break;
        default: size.x = 0.0f; break;
        }
    }

    return size;
}

Widget* UIBuilder::widgetFor(Element& target)
{
    switch (target.type) {
    case ElementType::Button: return ensureWidget(m_buttons, target.id);
    case ElementType::Checkbox: return ensureWidget(m_checkboxes, target.id, target.checked);
    case ElementType::SliderFloat: return ensureWidget(m_sliders, target.id, target.floatValue, target.minValue, target.maxValue);
    case ElementType::NumberFloat: return ensureWidget(m_numberInputs, target.id, target.floatValue, target.minValue, target.maxValue);
    case ElementType::TextInput: return ensureWidget(m_textInputs, target.id, target.stringValue);
    case ElementType::FilePathInput: return ensureWidget(m_filePathInputs, target.id, target.stringValue);
    case ElementType::Dropdown: return ensureWidget(m_dropdowns, target.id, target.items, target.selectedIndex);
    case ElementType::ColorPicker: return ensureWidget(m_colorPickers, target.id, target.color);
    default: return nullptr;
    }
}

const Widget* UIBuilder::widgetFor(const Element& target) const
{
    const auto find = [&target](const auto& widgets) -> const Widget* {
        const auto found = widgets.find(target.id);
        return found == widgets.end() ? nullptr : found->second.get();
    };
    switch (target.type) {
    case ElementType::Button: return find(m_buttons);
    case ElementType::Checkbox: return find(m_checkboxes);
    case ElementType::SliderFloat: return find(m_sliders);
    case ElementType::NumberFloat: return find(m_numberInputs);
    case ElementType::TextInput: return find(m_textInputs);
    case ElementType::FilePathInput: return find(m_filePathInputs);
    case ElementType::Dropdown: return find(m_dropdowns);
    case ElementType::ColorPicker: return find(m_colorPickers);
    default: return nullptr;
    }
}

ScrollContainer* UIBuilder::scrollFor(const Element& target)
{
    if (!target.scrollable) {
        return nullptr;
    }
    auto& scroll = m_scrollAreas[target.id];
    if (!scroll) {
        scroll = std::make_unique<ScrollContainer>(target.id + ".scroll");
    }
    return scroll.get();
}

const ScrollContainer* UIBuilder::scrollFor(const Element& target) const
{
    const auto found = m_scrollAreas.find(target.id);
    return found == m_scrollAreas.end() ? nullptr : found->second.get();
}

void UIBuilder::layoutRoot()
{
    UIDockLayout dockLayout;
    dockLayout.setBounds({m_surface.origin, m_surface.size});
    dockLayout.setGap(m_style->gap);

    for (const std::string& id : m_order) {
        Element& target = element(id);
        if (!target.parentId.empty() || !target.docked || !isVisible(target)) {
            continue;
        }

        float extent = 0.0f;
        if (target.dock == UIDock::Left || target.dock == UIDock::Right) {
            extent = target.width > 0.0f ? target.width : preferredSizeFor(target).x;
        } else if (target.dock == UIDock::Top || target.dock == UIDock::Bottom) {
            extent = target.height > 0.0f ? target.height : preferredSizeFor(target).y;
        }
        dockLayout.add(target.bounds, target.dock, extent);
    }

    dockLayout.layout();

    for (const std::string& id : m_order) {
        Element& target = element(id);
        if (target.parentId.empty() && isVisible(target)) {
            layoutChildren(target);
        }
    }
}

void UIBuilder::layoutChildren(Element& parent)
{
    const std::vector<std::string> children = childrenOf(parent.id);
    if (children.empty()) {
        return;
    }

    if (parent.type == ElementType::Dropdown) {
        auto* dropdown = static_cast<Dropdown*>(widgetFor(parent));
        if (dropdown == nullptr || !dropdown->popupOpen()) {
            return;
        }
        if (parent.popupSize.x > 0.0f && parent.popupSize.y > 0.0f) {
            dropdown->setPopupSize(parent.popupSize);
        }

        const UIRect popupBounds = dropdown->popupBounds();
        const UIRect contentBounds = inset(popupBounds, paddingFor(parent));
        switch (parent.layout) {
        case UIContainerLayout::HorizontalLayout:
            layoutHorizontal(parent, contentBounds, children);
            break;
        case UIContainerLayout::GridLayout:
            layoutGrid(parent, contentBounds, children);
            break;
        case UIContainerLayout::OverlayLayout:
        case UIContainerLayout::DockLayout:
        case UIContainerLayout::VerticalLayout:
            layoutVertical(parent, contentBounds, children);
            break;
        }
        return;
    }

    UIRect contentBounds = inset(parent.bounds, paddingFor(parent));
    if (ScrollContainer* scroll = scrollFor(parent)) {
        scroll->setBounds({contentBounds.position, contentBounds.size});
        scroll->setScrollbarRightEdge(parent.bounds.position.x + parent.bounds.size.x);
        contentBounds.position = scroll->contentOrigin();
    }

    switch (parent.layout) {
    case UIContainerLayout::HorizontalLayout:
        layoutHorizontal(parent, contentBounds, children);
        break;
    case UIContainerLayout::GridLayout:
        layoutGrid(parent, contentBounds, children);
        break;
    case UIContainerLayout::OverlayLayout:
    case UIContainerLayout::DockLayout:
    case UIContainerLayout::VerticalLayout:
        layoutVertical(parent, contentBounds, children);
        break;
    }
}

void UIBuilder::layoutVertical(Element& parent, const UIRect& contentBounds, const std::vector<std::string>& children)
{
    const float gap = gapFor(parent);
    float cursor = contentBounds.position.y;

    for (const std::string& id : children) {
        Element& child = element(id);
        if (!isVisible(child)) {
            continue;
        }

        const glm::vec2 preferred = preferredSizeFor(child);
        const bool hasTopLabel = !child.label.empty() && child.labelPlacement == UILabelPlacement::Top;
        const float itemHeight = std::max(
            child.height,
            preferred.y + (hasTopLabel ? labelHeight() + labelGap() : 0.0f));
        assignControlBounds(child, {{contentBounds.position.x, cursor}, {contentBounds.size.x, itemHeight}});
        cursor += itemHeight + gap;
        layoutChildren(child);
    }

    if (ScrollContainer* scroll = scrollFor(parent)) {
        scroll->setContentHeight(std::max(cursor - contentBounds.position.y, contentBounds.size.y));
    }
}

void UIBuilder::layoutHorizontal(Element& parent, const UIRect& contentBounds, const std::vector<std::string>& children)
{
    const float gap = gapFor(parent);
    float fixedWidth = gap * static_cast<float>(children.empty() ? 0U : children.size() - 1U);
    float totalGrow = 0.0f;
    for (const std::string& id : children) {
        const Element& child = *findElement(id);
        if (!isVisible(child)) {
            continue;
        }
        fixedWidth += std::max(preferredSizeFor(child).x, child.width);
        totalGrow += child.grow;
    }

    const float extra = std::max(0.0f, contentBounds.size.x - fixedWidth);
    float cursor = contentBounds.position.x;
    for (const std::string& id : children) {
        Element& child = element(id);
        if (!isVisible(child)) {
            continue;
        }
        glm::vec2 preferred = preferredSizeFor(child);
        const float width = std::max(preferred.x, child.width) + (totalGrow > 0.0f ? extra * child.grow / totalGrow : 0.0f);
        assignControlBounds(child, {{cursor, contentBounds.position.y}, {width, contentBounds.size.y}});
        cursor += width + gap;
        layoutChildren(child);
    }
}

void UIBuilder::layoutGrid(Element& parent, const UIRect& contentBounds, const std::vector<std::string>& children)
{
    const int columns = std::max(1, parent.gridColumns);
    const float gap = gapFor(parent);
    const float cellWidth = std::max(0.0f, (contentBounds.size.x - gap * static_cast<float>(columns - 1)) / static_cast<float>(columns));
    float cursorY = contentBounds.position.y;
    int column = 0;
    float rowHeight = 0.0f;

    for (const std::string& id : children) {
        Element& child = element(id);
        if (!isVisible(child)) {
            continue;
        }

        const glm::vec2 preferred = preferredSizeFor(child);
        rowHeight = std::max(rowHeight, preferred.y);
        const float x = contentBounds.position.x + static_cast<float>(column) * (cellWidth + gap);
        assignControlBounds(child, {{x, cursorY}, {cellWidth, preferred.y}});
        layoutChildren(child);

        ++column;
        if (column >= columns) {
            column = 0;
            cursorY += rowHeight + gap;
            rowHeight = 0.0f;
        }
    }

    if (ScrollContainer* scroll = scrollFor(parent)) {
        const float usedHeight = cursorY - contentBounds.position.y + (column == 0 ? 0.0f : rowHeight);
        scroll->setContentHeight(std::max(usedHeight, contentBounds.size.y));
    }
}

void UIBuilder::assignControlBounds(Element& target, const UIRect& itemBounds)
{
    target.labelBounds = {};
    target.textBounds = {};
    target.bounds = itemBounds;

    if (target.type == ElementType::Label) {
        target.textBounds = itemBounds;
        return;
    }

    const glm::vec2 preferred = preferredSizeFor(target);
    if (!target.label.empty() && target.labelPlacement == UILabelPlacement::Top) {
        target.labelBounds = {itemBounds.position, {itemBounds.size.x, labelHeight()}};
        target.bounds = {
            {itemBounds.position.x, itemBounds.position.y + labelHeight() + labelGap()},
            {itemBounds.size.x, std::max(0.0f, itemBounds.size.y - labelHeight() - labelGap())},
        };
    } else if (!target.label.empty() && target.labelPlacement == UILabelPlacement::Right) {
        const float controlWidth = preferred.x > 0.0f ? preferred.x : itemBounds.size.y;
        target.bounds = {itemBounds.position, {controlWidth, itemBounds.size.y}};
        target.labelBounds = {
            {itemBounds.position.x + controlWidth + labelGap(), itemBounds.position.y},
            {std::max(0.0f, itemBounds.size.x - controlWidth - labelGap()), itemBounds.size.y},
        };
    } else if (!target.label.empty() && target.labelPlacement == UILabelPlacement::Left) {
        const float labelWidth = std::min(std::max(80.0f, itemBounds.size.x * 0.36f), itemBounds.size.x);
        target.labelBounds = {itemBounds.position, {labelWidth, itemBounds.size.y}};
        target.bounds = {
            {itemBounds.position.x + labelWidth + labelGap(), itemBounds.position.y},
            {std::max(0.0f, itemBounds.size.x - labelWidth - labelGap()), itemBounds.size.y},
        };
    }

    if (target.type == ElementType::ColorPicker && preferred.x > 0.0f) {
        target.bounds.size.x = std::min(target.bounds.size.x, preferred.x);
    }

    if (Widget* widget = widgetFor(target)) {
        widget->setBounds(target.bounds.position, target.bounds.size);
    }

    if (target.type == ElementType::Button) {
        constexpr float insetX = 8.0f;
        target.textBounds = {
            target.bounds.position + glm::vec2{insetX, 0.0f},
            {std::max(0.0f, target.bounds.size.x - insetX * 2.0f), target.bounds.size.y},
        };
    } else if (target.type == ElementType::Dropdown) {
        target.textBounds = {
            target.bounds.position + glm::vec2{10.0f, 0.0f},
            {std::max(0.0f, target.bounds.size.x - 30.0f), target.bounds.size.y},
        };
    } else {
        target.textBounds = target.bounds;
    }
}

void UIBuilder::applyWidgetState(Element& target)
{
    Widget* widget = widgetFor(target);
    if (widget == nullptr) {
        return;
    }

    const bool targetVisible = isVisible(target);
    widget->setVisible(targetVisible);
    widget->setStyleClass(target.styleClass);

    switch (target.type) {
    case ElementType::Button: {
        auto& button = *static_cast<Button*>(widget);
        button.setSelected(target.selected);
        button.setIcon(target.icon);
        if (target.iconImage) {
            button.setIconImage(*target.iconImage);
        } else {
            button.clearIconImage();
        }
        button.setOnClick(target.onClick);
        break;
    }
    case ElementType::Checkbox: {
        auto& checkbox = *static_cast<Checkbox*>(widget);
        checkbox.setOnValueChanged(target.onBoolChanged);
        checkbox.setChecked(target.checked);
        break;
    }
    case ElementType::SliderFloat: {
        auto& slider = *static_cast<Slider*>(widget);
        slider.setOnValueChanged(target.onFloatChanged);
        slider.setRange(target.minValue, target.maxValue);
        slider.setPrecision(target.precision);
        slider.setValue(target.floatValue);
        break;
    }
    case ElementType::NumberFloat: {
        auto& number = *static_cast<NumberInput*>(widget);
        number.setOnValueChanged(target.onFloatChanged);
        number.setRange(target.minValue, target.maxValue);
        number.setSensitivity(target.sensitivity);
        number.setPrecision(target.precision);
        number.setAxis(target.numberAxis);
        number.setValue(target.floatValue);
        break;
    }
    case ElementType::TextInput: {
        auto& input = *static_cast<TextInput*>(widget);
        input.setValue(target.stringValue);
        input.setPlaceholder(target.placeholder);
        input.setOnValueChanged(target.onTextChanged);
        break;
    }
    case ElementType::FilePathInput: {
        auto& input = *static_cast<FilePathInput*>(widget);
        input.setValue(target.stringValue);
        input.setPlaceholder(target.placeholder);
        input.setButtonLabel(target.buttonLabel);
        input.setBrowseButtonWidth(target.browseButtonWidth);
        input.setInputStyleClass(target.styleClass.empty() ? "inspector" : target.styleClass);
        input.setButtonStyleClass("toolbar");
        input.setOnValueChanged(target.onTextChanged);
        input.setOnBrowse(target.onBrowse);
        break;
    }
    case ElementType::Dropdown: {
        auto& dropdown = *static_cast<Dropdown*>(widget);
        dropdown.setOnSelectionChanged(target.onSelectionChanged);
        dropdown.setItems(target.items);
        dropdown.setSelectedIndex(target.selectedIndex);
        if (target.popupSize.x > 0.0f && target.popupSize.y > 0.0f) {
            dropdown.setPopupSize(target.popupSize);
        } else {
            dropdown.clearPopupSize();
        }
        if (target.dropdownCollapsedIcon && target.dropdownExpandedIcon) {
            dropdown.setArrowIconImages(*target.dropdownCollapsedIcon, *target.dropdownExpandedIcon);
        } else {
            dropdown.clearArrowIconImages();
        }
        break;
    }
    case ElementType::ColorPicker: {
        auto& picker = *static_cast<ColorPicker*>(widget);
        picker.setOnColorChanged(target.onColorChanged);
        picker.setColor(target.color);
        break;
    }
    default:
        break;
    }
}

void UIBuilder::updateElement(Element& target)
{
    if (!isVisible(target)) {
        return;
    }

    if (target.type == ElementType::Panel) {
        updateChildren(target);
        return;
    }

    Widget* widget = widgetFor(target);
    if (widget == nullptr) {
        return;
    }

    const UITextStyle& inputValueStyle = m_style->resolveText("input-value");
    switch (target.type) {
    case ElementType::SliderFloat:
        static_cast<Slider*>(widget)->update(*m_context, *m_text, *m_style, inputValueStyle.font, inputValueStyle.scale);
        break;
    case ElementType::NumberFloat:
        static_cast<NumberInput*>(widget)->update(*m_context, *m_text, *m_style, inputValueStyle.font, inputValueStyle.scale);
        break;
    case ElementType::TextInput:
        static_cast<TextInput*>(widget)->update(*m_context, *m_text, *m_style, inputValueStyle.font, inputValueStyle.scale);
        break;
    case ElementType::FilePathInput:
        static_cast<FilePathInput*>(widget)->update(*m_context, *m_text, *m_style, inputValueStyle.font, inputValueStyle.scale);
        break;
    default:
        widget->update(*m_context);
        break;
    }

    if (target.type == ElementType::Dropdown) {
        return;
    }

    updateChildren(target);
}

void UIBuilder::updateChildren(Element& parent)
{
    if (ScrollContainer* scroll = scrollFor(parent)) {
        scroll->pushClip(*m_context);
        for (const std::string& childId : childrenOf(parent.id)) {
            updateElement(element(childId));
        }
        scroll->popClip(*m_context);
        return;
    }

    for (const std::string& childId : childrenOf(parent.id)) {
        updateElement(element(childId));
    }
}

void UIBuilder::registerPopupLayers()
{
    for (const std::string& id : m_order) {
        Element& target = element(id);
        if (!isVisible(target)) {
            continue;
        }
        if (target.type == ElementType::Dropdown) {
            if (auto* dropdown = static_cast<Dropdown*>(widgetFor(target))) {
                dropdown->registerPopupLayer(*m_context);
            }
        } else if (target.type == ElementType::ColorPicker) {
            if (auto* picker = static_cast<ColorPicker*>(widgetFor(target))) {
                picker->registerPopupLayer(*m_context);
            }
        }
    }
}

void UIBuilder::updatePopups()
{
    for (const std::string& id : m_order) {
        Element& target = element(id);
        if (!isVisible(target)) {
            continue;
        }
        if (target.type == ElementType::Dropdown) {
            if (auto* dropdown = static_cast<Dropdown*>(widgetFor(target))) {
                dropdown->updatePopup(*m_context);
                if (dropdown->popupOpen() && hasPopupChildren(target)) {
                    dropdown->registerPopupLayer(*m_context);
                    layoutChildren(target);
                    m_context->pushLayer(target.id + ".popup-layer");
                    updateChildren(target);
                    m_context->popLayer();
                }
            }
        } else if (target.type == ElementType::ColorPicker) {
            if (auto* picker = static_cast<ColorPicker*>(widgetFor(target))) {
                picker->updatePopup(*m_context, *m_text, *m_style);
            }
        }
    }
}

void UIBuilder::renderBase(UIFrame& frame)
{
    for (const std::string& id : m_order) {
        Element& root = element(id);
        if (root.parentId.empty()) {
            renderElement(frame, root);
        }
    }
}

void UIBuilder::renderElement(UIFrame& frame, Element& target)
{
    if (!isVisible(target)) {
        return;
    }

    if (target.type == ElementType::Panel) {
        if (target.drawBackground) {
            const UIComputedStyle style = computedStyle(target);
            frame.shapes().drawSdfRect(
                target.bounds.position,
                target.bounds.size,
                style.borderRadius,
                style.background,
                style.border,
                style.borderWidth);
        }

        renderChildren(frame, target);
        if (const ScrollContainer* scroll = scrollFor(target)) {
            scroll->renderScrollbar(frame.shapes(), frame.style());
        }
        return;
    }

    renderWidget(frame, target);
    renderElementText(frame, target);
    if (target.type == ElementType::Dropdown) {
        return;
    }
    renderChildren(frame, target);
}

void UIBuilder::renderChildren(UIFrame& frame, Element& parent)
{
    if (ScrollContainer* scroll = scrollFor(parent)) {
        scroll->pushClip(frame.shapes());
        scroll->pushClip(frame.text());
        for (const std::string& childId : childrenOf(parent.id)) {
            renderElement(frame, element(childId));
        }
        scroll->popClip(frame.text());
        scroll->popClip(frame.shapes());
        return;
    }

    for (const std::string& childId : childrenOf(parent.id)) {
        renderElement(frame, element(childId));
    }
}

void UIBuilder::renderWidget(UIFrame& frame, Element& target)
{
    Widget* widget = widgetFor(target);
    if (widget == nullptr) {
        return;
    }

    switch (target.type) {
    case ElementType::SliderFloat:
        static_cast<Slider*>(widget)->render(frame);
        break;
    case ElementType::NumberFloat:
        static_cast<NumberInput*>(widget)->render(frame);
        break;
    case ElementType::TextInput:
        static_cast<TextInput*>(widget)->render(frame);
        break;
    case ElementType::FilePathInput:
        static_cast<FilePathInput*>(widget)->render(frame);
        break;
    default:
        widget->render(frame.shapes(), frame.style());
        break;
    }
}

void UIBuilder::renderElementText(UIFrame& frame, Element& target)
{
    const Widget* widget = widgetFor(target);
    const bool hovered = widget != nullptr && widget->interaction().hovered;

    if (!target.label.empty() && target.labelPlacement != UILabelPlacement::Inside) {
        UITextStyle labelStyle = frame.style().resolveText(target.labelStyleClass);
        labelStyle.overflow = UIOverflow::Ellipsis;
        frame.drawText(target.label, {target.labelBounds.position, target.labelBounds.size}, labelStyle);
    }

    std::string text;
    if (target.type == ElementType::Label) {
        text = target.text.empty() ? target.label : target.text;
    } else if (target.type == ElementType::Button) {
        text = target.text;
    } else if (target.type == ElementType::Dropdown) {
        if (!target.text.empty()) {
            text = target.text;
        } else if (auto* dropdown = static_cast<Dropdown*>(widgetFor(target))) {
            text = std::string(dropdown->selectedText());
        }
    }

    if (!text.empty()) {
        UITextStyle textStyle = frame.style().resolveText(target.textStyleClass, target.id);
        textStyle.overflow = UIOverflow::Ellipsis;
        const bool truncated = frame.drawText(text, {target.textBounds.position, target.textBounds.size}, textStyle);
        if (hovered && !target.tooltip.empty()) {
            frame.input().requestTooltip(target.tooltip);
        } else if (hovered && truncated) {
            frame.input().requestTooltip(text);
        }
    } else if (hovered && !target.tooltip.empty()) {
        frame.input().requestTooltip(target.tooltip);
    }
}

void UIBuilder::renderPopups()
{
    UIFrame frame{*m_context, *m_renderer, *m_text, *m_style, m_surface};
    for (const std::string& id : m_order) {
        Element& target = element(id);
        if (!isVisible(target)) {
            continue;
        }
        if (target.type == ElementType::Dropdown) {
            if (auto* dropdown = static_cast<Dropdown*>(widgetFor(target))) {
                dropdown->renderPopup(*m_context, *m_renderer, *m_text, *m_style);
                if (dropdown->popupOpen() && hasPopupChildren(target)) {
                    renderChildren(frame, target);
                }
            }
        } else if (target.type == ElementType::ColorPicker) {
            if (auto* picker = static_cast<ColorPicker*>(widgetFor(target))) {
                picker->renderPopup(*m_context, *m_renderer, *m_text, *m_style);
            }
        }
    }
}

void UIBuilder::renderTooltip(const std::string_view text)
{
    const UITextStyle& textStyle = m_style->resolveText("toolbar-toggle");
    const UIComputedStyle tooltipStyle = m_style->resolveElement("tooltip", "tooltip");
    const glm::vec2 padding{9.0f, 6.0f};
    const glm::vec2 textSize = m_text->measureText(text, textStyle.font, textStyle.scale);
    const glm::vec2 size{textSize.x + padding.x * 2.0f, textSize.y + padding.y * 2.0f};
    glm::vec2 position = m_context->mousePosition() + glm::vec2{14.0f, 16.0f};
    position.x = std::clamp(position.x, 4.0f, std::max(4.0f, m_surface.size.x - size.x - 4.0f));
    position.y = std::clamp(position.y, 4.0f, std::max(4.0f, m_surface.size.y - size.y - 4.0f));

    m_renderer->drawSdfRect(
        position,
        size,
        tooltipStyle.borderRadius > 0.0f ? tooltipStyle.borderRadius : 4.0f,
        tooltipStyle.background,
        tooltipStyle.border,
        tooltipStyle.borderWidth > 0.0f ? tooltipStyle.borderWidth : 1.0f);

    glm::vec4 color = textStyle.color;
    color.a *= std::clamp(textStyle.opacity, 0.0f, 1.0f);
    m_text->drawText(text, position + padding, color, textStyle.font, textStyle.scale);
}

} // namespace Engine
