#include "Engine/UI/TextInput.hpp"

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/UI/UIStyle.hpp"

#include <algorithm>
#include <utility>

namespace Engine {

namespace {

std::string encodeUtf8(const char32_t codepoint)
{
    std::string encoded;
    if (codepoint <= 0x7F) {
        encoded.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        encoded.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        encoded.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        encoded.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        encoded.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        encoded.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
        encoded.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        encoded.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        encoded.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        encoded.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    return encoded;
}

bool isContinuation(const char character)
{
    return (static_cast<unsigned char>(character) & 0xC0U) == 0x80U;
}

std::size_t previousBoundary(const std::string& text, std::size_t index)
{
    if (index == 0) {
        return 0;
    }
    --index;
    while (index > 0 && isContinuation(text[index])) {
        --index;
    }
    return index;
}

std::size_t nextBoundary(const std::string& text, std::size_t index)
{
    if (index >= text.size()) {
        return text.size();
    }
    ++index;
    while (index < text.size() && isContinuation(text[index])) {
        ++index;
    }
    return index;
}

} // namespace

TextInput::TextInput(std::string id, std::string value)
    : Widget(std::move(id))
    , m_value(std::move(value))
    , m_caretIndex(m_value.size())
    , m_selectionAnchor(m_value.size())
{
}

void TextInput::update(UIContext& context)
{
    Widget::update(context);
    if (m_interaction.pressed) {
        context.focus(m_id);
    } else if (context.primaryMousePressed() && !m_interaction.hovered && context.isFocused(m_id)) {
        context.clearFocus();
    }

    m_focused = context.isFocused(m_id);
    if (!m_focused) {
        return;
    }

    bool changed = false;
    for (const char32_t character : context.typedCharacters()) {
        if (character >= 32) {
            eraseSelection();
            const std::string encoded = encodeUtf8(character);
            m_value.insert(m_caretIndex, encoded);
            m_caretIndex += encoded.size();
            m_selectionAnchor = m_caretIndex;
            changed = true;
        }
    }

    for (const UIKey key : context.pressedKeys()) {
        switch (key) {
        case UIKey::Backspace:
            if (hasSelection()) {
                eraseSelection();
                changed = true;
            } else if (m_caretIndex > 0) {
                const std::size_t previous = previousBoundary(m_value, m_caretIndex);
                m_value.erase(previous, m_caretIndex - previous);
                moveCaret(previous, false);
                changed = true;
            }
            break;
        case UIKey::Delete:
            if (hasSelection()) {
                eraseSelection();
                changed = true;
            } else if (m_caretIndex < m_value.size()) {
                m_value.erase(m_caretIndex, nextBoundary(m_value, m_caretIndex) - m_caretIndex);
                changed = true;
            }
            break;
        case UIKey::Left:
            moveCaret(previousBoundary(m_value, m_caretIndex), context.shiftDown());
            break;
        case UIKey::Right:
            moveCaret(nextBoundary(m_value, m_caretIndex), context.shiftDown());
            break;
        case UIKey::Home:
            moveCaret(0, context.shiftDown());
            break;
        case UIKey::End:
            moveCaret(m_value.size(), context.shiftDown());
            break;
        case UIKey::Enter:
        case UIKey::Escape:
            context.clearFocus();
            m_focused = false;
            break;
        }
    }

    if (changed) {
        notifyValueChanged();
    }
}

void TextInput::render(Renderer2D& renderer2D, const UIStyle& style) const
{
    if (!m_visible) {
        return;
    }

    const UITextInputStyle& inputStyle = m_styleOverride ? *m_styleOverride : style.resolveTextInput(m_styleClass, m_id);
    const glm::vec4 fill = m_focused
        ? inputStyle.focused
        : m_interaction.hovered
            ? inputStyle.hovered
            : inputStyle.box.fill;
    const glm::vec4 border = m_focused ? inputStyle.focusedBorder : inputStyle.box.border;
    renderer2D.drawSdfRect(m_position, m_size, inputStyle.box.borderRadius, fill, border, inputStyle.box.borderWidth);
}

void TextInput::setValue(std::string value)
{
    m_value = std::move(value);
    m_caretIndex = std::min(m_caretIndex, m_value.size());
    m_selectionAnchor = m_caretIndex;
}

void TextInput::setPlaceholder(std::string placeholder)
{
    m_placeholder = std::move(placeholder);
}

void TextInput::setStyle(const UITextInputStyle& style)
{
    m_styleOverride = style;
}

void TextInput::clearStyleOverride()
{
    m_styleOverride.reset();
}

void TextInput::setOnValueChanged(std::function<void(std::string_view)> callback)
{
    m_onValueChanged = std::move(callback);
}

const std::string& TextInput::value() const
{
    return m_value;
}

const std::string& TextInput::placeholder() const
{
    return m_placeholder;
}

std::size_t TextInput::caretIndex() const
{
    return m_caretIndex;
}

std::size_t TextInput::selectionStart() const
{
    return std::min(m_caretIndex, m_selectionAnchor);
}

std::size_t TextInput::selectionEnd() const
{
    return std::max(m_caretIndex, m_selectionAnchor);
}

bool TextInput::hasSelection() const
{
    return m_caretIndex != m_selectionAnchor;
}

bool TextInput::focused() const
{
    return m_focused;
}

void TextInput::notifyValueChanged() const
{
    if (m_onValueChanged) {
        m_onValueChanged(m_value);
    }
}

void TextInput::eraseSelection()
{
    if (!hasSelection()) {
        return;
    }

    const std::size_t start = selectionStart();
    m_value.erase(start, selectionEnd() - start);
    moveCaret(start, false);
}

void TextInput::moveCaret(const std::size_t caretIndex, const bool extendSelection)
{
    m_caretIndex = std::min(caretIndex, m_value.size());
    if (!extendSelection) {
        m_selectionAnchor = m_caretIndex;
    }
}

} // namespace Engine
