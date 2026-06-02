#include "Engine/UI/BasicWidgets.hpp"
#include "Engine/UI/ColorPicker.hpp"
#include "Engine/UI/ScrollContainer.hpp"

#include <cassert>
#include <memory>

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
    picker.setBounds({10.0f, 20.0f}, {140.0f, 60.0f});
    Engine::UIContext context;
    context.beginFrame({.mousePosition = {80.0f, 22.0f}, .primaryMouseDown = true});
    picker.update(context);
    assert(picker.color().r == 0.3f);
}
