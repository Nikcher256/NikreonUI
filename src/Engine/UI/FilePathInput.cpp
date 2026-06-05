#include "Engine/UI/FilePathInput.hpp"

#include "Engine/UI/UIFrame.hpp"

#include <algorithm>
#include <utility>

namespace Engine {

FilePathInput::FilePathInput(std::string id, std::string value)
    : Widget(id)
    , m_input(id + ".path", std::move(value))
    , m_browseButton(id + ".browse")
{
    m_input.setStyleClass("inspector");
    m_browseButton.setStyleClass("toolbar");
    m_input.moveCaretToEnd();
}

void FilePathInput::update(UIContext& context)
{
    if (!m_visible) {
        return;
    }

    layoutChildren();
    m_input.update(context);
    m_browseButton.update(context);
}

void FilePathInput::update(
    UIContext& context,
    const TextRenderer& textRenderer,
    const UIStyle& style,
    const std::string_view fontName,
    const float scale)
{
    if (!m_visible) {
        return;
    }

    layoutChildren();
    m_input.update(context, textRenderer, style, fontName, scale);
    m_browseButton.update(context);
}

void FilePathInput::render(Renderer2D& renderer2D, const UIStyle& style) const
{
    if (!m_visible) {
        return;
    }

    m_input.render(renderer2D, style);
    m_browseButton.render(renderer2D, style);
}

void FilePathInput::render(const UIFrame& frame) const
{
    if (!m_visible) {
        return;
    }

    render(frame.shapes(), frame.style());
    const UITextStyle& inputStyle = frame.style().resolveText(m_input.value().empty() ? "input-placeholder" : "input-value");
    m_input.renderText(frame.text(), frame.style(), inputStyle.font, inputStyle.scale);
    frame.drawText(m_buttonLabel, {m_browseButton.position(), m_browseButton.size()}, frame.style().resolveText("toolbar-toggle"));
}

void FilePathInput::setValue(std::string value)
{
    if (m_input.value() == value) {
        return;
    }

    m_input.setValue(std::move(value));
    if (m_keepPathEndVisible) {
        m_input.moveCaretToEnd();
    }
}

void FilePathInput::setPlaceholder(std::string placeholder)
{
    m_input.setPlaceholder(std::move(placeholder));
}

void FilePathInput::setButtonLabel(std::string label)
{
    m_buttonLabel = std::move(label);
}

void FilePathInput::setBrowseButtonWidth(const float width)
{
    m_browseButtonWidth = std::max(width, 28.0f);
}

void FilePathInput::setGap(const float gap)
{
    m_gap = std::max(gap, 0.0f);
}

void FilePathInput::setKeepPathEndVisible(const bool keepPathEndVisible)
{
    m_keepPathEndVisible = keepPathEndVisible;
}

void FilePathInput::setOnValueChanged(std::function<void(std::string_view)> callback)
{
    m_input.setOnValueChanged(std::move(callback));
}

void FilePathInput::setOnBrowse(std::function<void()> callback)
{
    m_browseButton.setOnClick(std::move(callback));
}

void FilePathInput::setInputStyleClass(std::string styleClass)
{
    m_input.setStyleClass(std::move(styleClass));
}

void FilePathInput::setButtonStyleClass(std::string styleClass)
{
    m_browseButton.setStyleClass(std::move(styleClass));
}

const std::string& FilePathInput::value() const
{
    return m_input.value();
}

const std::string& FilePathInput::placeholder() const
{
    return m_input.placeholder();
}

const glm::vec2& FilePathInput::inputPosition() const
{
    return m_input.position();
}

const glm::vec2& FilePathInput::inputSize() const
{
    return m_input.size();
}

const glm::vec2& FilePathInput::buttonPosition() const
{
    return m_browseButton.position();
}

const glm::vec2& FilePathInput::buttonSize() const
{
    return m_browseButton.size();
}

void FilePathInput::layoutChildren()
{
    const float buttonWidth = std::min(m_browseButtonWidth, std::max(m_size.x, 0.0f));
    const float gap = m_size.x > buttonWidth ? m_gap : 0.0f;
    const float inputWidth = std::max(m_size.x - buttonWidth - gap, 0.0f);
    m_input.setBounds(m_position, {inputWidth, m_size.y});
    m_browseButton.setBounds({m_position.x + inputWidth + gap, m_position.y}, {buttonWidth, m_size.y});
    m_input.setVisible(m_visible);
    m_browseButton.setVisible(m_visible);
}

} // namespace Engine
