#include "Engine/UI/BasicWidgets.hpp"
#include "Engine/UI/ColorPicker.hpp"
#include "Engine/UI/ScrollContainer.hpp"
#include "Engine/UI/UIRenderQueue.hpp"
#include "Engine/UI/UIStyleParser.hpp"

#include <cassert>
#include <memory>
#include <vector>

int main()
{
    Engine::ProgressBar progress{"progress", 2.0f};
    assert(progress.value() == 1.0f);
    progress.setValue(-1.0f);
    assert(progress.value() == 0.0f);

    Engine::ScrollContainer scroll{"list"};
    scroll.setBounds({{10.0f, 20.0f}, {100.0f, 50.0f}});
    scroll.setContentHeight(150.0f);
    assert(scroll.maxOffset() == 100.0f);
    assert(scroll.contentOrigin() == glm::vec2(10.0f, 20.0f));

    auto child = std::make_unique<Engine::Panel>("panel");
    child->setBounds({}, {40.0f, 12.0f});
    Engine::Widget& retained = scroll.addChild(std::move(child), {4.0f, 6.0f});
    assert(retained.size() == glm::vec2(40.0f, 12.0f));

    Engine::ColorPicker picker{"clear-color", {2.0f, -1.0f, 0.5f, 1.0f}};
    assert(picker.color() == glm::vec4(1.0f, 0.0f, 0.5f, 1.0f));
    picker.setBounds({300.0f, 20.0f}, {140.0f, 28.0f});
    Engine::UIContext context;
    context.beginFrame({.mousePosition = {320.0f, 30.0f}, .primaryMouseDown = true});
    picker.update(context);
    context.endFrame();
    context.beginFrame({.mousePosition = {320.0f, 30.0f}, .primaryMouseDown = false});
    picker.update(context);
    assert(picker.popupOpen());
    context.endFrame();

    context.beginFrame({.mousePosition = {163.0f, 66.0f}, .primaryMouseDown = true});
    picker.updatePopup(context);
    assert(picker.color().g > 0.45f);
    assert(picker.color().b > 0.45f);

    Engine::UIContext layeredContext;
    layeredContext.beginFrame({.mousePosition = {25.0f, 25.0f}, .primaryMouseDown = true});
    layeredContext.registerLayer("popup", 100, {{0.0f, 0.0f}, {50.0f, 50.0f}});
    const Engine::UIInteraction blocked = layeredContext.interact("base-button", {0.0f, 0.0f}, {50.0f, 50.0f});
    assert(!blocked.hovered);
    layeredContext.pushLayer("popup");
    const Engine::UIInteraction popupDown = layeredContext.interact("popup-button", {0.0f, 0.0f}, {50.0f, 50.0f});
    assert(popupDown.hovered);
    assert(popupDown.held);
    layeredContext.popLayer();
    layeredContext.endFrame();

    layeredContext.beginFrame({.mousePosition = {25.0f, 25.0f}, .primaryMouseDown = false});
    layeredContext.registerLayer("popup", 100, {{0.0f, 0.0f}, {50.0f, 50.0f}});
    layeredContext.pushLayer("popup");
    const Engine::UIInteraction popupClick = layeredContext.interact("popup-button", {0.0f, 0.0f}, {50.0f, 50.0f});
    assert(popupClick.pressed);
    layeredContext.popLayer();
    layeredContext.endFrame();

    Engine::UIRenderQueue renderQueue;
    std::vector<int> renderOrder;
    renderQueue.add(Engine::UIRenderLayer::Popup, [&renderOrder]() { renderOrder.push_back(3); });
    renderQueue.add(Engine::UIRenderLayer::Content, [&renderOrder]() { renderOrder.push_back(1); });
    renderQueue.add(Engine::UIRenderLayer::Content, [&renderOrder]() { renderOrder.push_back(2); });
    renderQueue.flush();
    assert((renderOrder == std::vector<int>{1, 2, 3}));

    Engine::UIStyle parsedStyle;
    std::string styleError;
    const bool parsed = Engine::UIStyleParser::parse(R"(
        button {
            padding: 4, 5, 6, 7;
            gap: 3;
            color: 0.1, 0.2, 0.3, 0.4;
            border: 2, 0.5, 0.6, 0.7, 0.8;
            border-radius: 5;
            font-size: 0.75;
            horizontal-align: center;
            vertical-align: bottom;
            wrap: word;
            overflow: clip;
            max-lines: 2;
            opacity: 0.5;
        }
        .toolbar {
            gap: 9;
        }
        #special {
            opacity: 0.25;
        }
    )", parsedStyle, styleError);
    assert(parsed);
    assert(styleError.empty());
    const Engine::UIComputedStyle buttonStyle = parsedStyle.resolveElement("button");
    assert(buttonStyle.padding.left == 4.0f);
    assert(buttonStyle.padding.top == 5.0f);
    assert(buttonStyle.padding.right == 6.0f);
    assert(buttonStyle.padding.bottom == 7.0f);
    assert(buttonStyle.gap == 3.0f);
    assert(buttonStyle.borderWidth == 2.0f);
    assert(buttonStyle.borderRadius == 5.0f);
    assert(buttonStyle.fontSize == 0.75f);
    assert(buttonStyle.horizontalAlignment == Engine::UITextHorizontalAlignment::Center);
    assert(buttonStyle.verticalAlignment == Engine::UITextVerticalAlignment::Bottom);
    assert(buttonStyle.wrap == Engine::UITextWrap::Word);
    assert(buttonStyle.overflow == Engine::UIOverflow::Clip);
    assert(buttonStyle.maxLines == 2);
    assert(buttonStyle.opacity == 0.5f);
    assert(parsedStyle.resolveElement("button", "toolbar").gap == 9.0f);
    assert(parsedStyle.resolveElement("button", "toolbar", "special").opacity == 0.25f);
}
