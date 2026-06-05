#include "Engine/UI/BasicWidgets.hpp"
#include "Engine/UI/ColorPicker.hpp"
#include "Engine/UI/ScrollContainer.hpp"
#include "Engine/UI/UIBuilder.hpp"
#include "Engine/UI/UIRenderQueue.hpp"
#include "Engine/UI/UIStyleParser.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct RenderEvent {
    std::string kind;
    std::uint64_t order{0};
};

class RecordingRenderer2D final : public Engine::Renderer2D {
public:
    explicit RecordingRenderer2D(std::vector<RenderEvent>& events)
        : m_events(&events)
    {
    }

    void begin(const glm::uvec2&) override {}

    std::uint64_t reserveRenderOrder() override
    {
        return m_nextOrder++;
    }

    void beginCompositeRenderItem(const std::uint64_t renderOrder) override
    {
        m_currentOrder = renderOrder;
        m_compositeActive = true;
    }

    void endCompositeRenderItem() override
    {
        m_currentOrder = 0;
        m_compositeActive = false;
    }

    void drawQuad(const glm::vec2&, const glm::vec2&, const glm::vec4&) override
    {
        record("shape");
    }

    void drawRect(const glm::vec2&, const glm::vec2&, const glm::vec4&, float) override
    {
        record("shape");
    }

    void drawSdfRect(const glm::vec2&, const glm::vec2&, float, const glm::vec4&, const glm::vec4&, float) override
    {
        record("shape");
    }

    void pushClipRect(const Engine::UIClipRect&) override {}
    void popClipRect() override {}
    void end() override {}

private:
    void record(std::string kind)
    {
        m_events->push_back({std::move(kind), m_compositeActive ? m_currentOrder : reserveRenderOrder()});
    }

    std::vector<RenderEvent>* m_events;
    std::uint64_t m_nextOrder{1};
    std::uint64_t m_currentOrder{0};
    bool m_compositeActive{false};
};

class RecordingTextRenderer final : public Engine::TextRenderer {
public:
    explicit RecordingTextRenderer(std::vector<RenderEvent>& events)
        : m_events(&events)
    {
    }

    void begin(const glm::uvec2&) override {}

    void beginCompositeRenderItem(const std::uint64_t renderOrder) override
    {
        m_currentOrder = renderOrder;
        m_compositeActive = true;
    }

    void endCompositeRenderItem() override
    {
        m_currentOrder = 0;
        m_compositeActive = false;
    }

    bool loadFont(std::string_view, const std::filesystem::path&, float) override
    {
        return true;
    }

    void drawText(std::string_view text, const glm::vec2&, const glm::vec4&, std::string_view, float, Engine::TextAlignment, const Engine::TextLayout&) override
    {
        if (!text.empty()) {
            m_events->push_back({"text", m_compositeActive ? m_currentOrder : 0});
        }
    }

    void drawSolidRect(const glm::vec2&, const glm::vec2&, const glm::vec4&, std::string_view) override
    {
        m_events->push_back({"text", m_compositeActive ? m_currentOrder : 0});
    }

    glm::vec2 measureText(std::string_view text, std::string_view, float scale, const Engine::TextLayout&) const override
    {
        return {static_cast<float>(text.size()) * 8.0f * scale, 16.0f * scale};
    }

    void pushClipRect(const Engine::UIClipRect&) override {}
    void popClipRect() override {}
    void end() override {}

private:
    std::vector<RenderEvent>* m_events;
    std::uint64_t m_currentOrder{0};
    bool m_compositeActive{false};
};

} // namespace

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

    std::vector<RenderEvent> compositeEvents;
    RecordingRenderer2D recordingShapes{compositeEvents};
    RecordingTextRenderer recordingText{compositeEvents};
    Engine::UIStyle compositeStyle;
    Engine::UIContext compositeContext;
    Engine::UIBuilder builder;

    recordingShapes.begin({160U, 28U});
    recordingText.begin({160U, 28U});
    compositeContext.beginFrame({.mousePosition = {-100.0f, -100.0f}});
    builder.begin(compositeContext, recordingShapes, recordingText, compositeStyle, {{0.0f, 0.0f}, {160.0f, 28.0f}});
    builder.panel("root")
        .dock(Engine::UIDock::Fill)
        .horizontal()
        .drawBackground(false)
        .padding(Engine::UIEdgeInsets::all(0.0f))
        .gap(0.0f);
    builder.button("first")
        .parent("root")
        .text("First")
        .width(80.0f)
        .height(28.0f);
    builder.button("second")
        .parent("root")
        .text("Second")
        .width(80.0f)
        .height(28.0f);
    builder.update();
    builder.render();
    builder.end();
    compositeContext.endFrame();
    recordingText.end();
    recordingShapes.end();

    assert(compositeEvents.size() == 4);
    for (const RenderEvent& event : compositeEvents) {
        assert(event.order == compositeEvents.front().order);
    }

    std::vector<std::string> replayedKinds;
    for (const RenderEvent& event : compositeEvents) {
        if (event.kind == "shape") {
            replayedKinds.push_back(event.kind);
        }
    }
    for (const RenderEvent& event : compositeEvents) {
        if (event.kind == "text") {
            replayedKinds.push_back(event.kind);
        }
    }
    assert((replayedKinds == std::vector<std::string>{"shape", "shape", "text", "text"}));

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
