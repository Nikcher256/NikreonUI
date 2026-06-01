#include "Engine/UI/TextInput.hpp"

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/UI/UIStyle.hpp"

#include <algorithm>
#include <utility>

namespace Engine {

TextInput::TextInput(std::string id, std::string value)
    : Widget(std::move(id))
    , m_value(std::move(value))
    , m_caretIndex(m_value.size())
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
        if (character >= 32 && character <= 126) {
            m_value.insert(m_caretIndex, 1, static_cast<char>(character));
            ++m_caretIndex;
            changed = true;
        }
    }

    for (const UIKey key : context.pressedKeys()) {
        switch (key) {
        case UIKey::Backspace:
            if (m_caretIndex > 0) {
                m_value.erase(m_caretIndex - 1, 1);
                --m_caretIndex;
                changed = true;
            }
            break;
        case UIKey::Delete:
            if (m_caretIndex < m_value.size()) {
                m_value.erase(m_caretIndex, 1);
                changed = true;
            }
            break;
        case UIKey::Left:
            m_caretIndex = m_caretIndex > 0 ? m_caretIndex - 1 : 0;
            break;
        case UIKey::Right:
            m_caretIndex = std::min(m_caretIndex + 1, m_value.size());
            break;
        case UIKey::Home:
            m_caretIndex = 0;
            break;
        case UIKey::End:
            m_caretIndex = m_value.size();
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

} // namespace Engine
