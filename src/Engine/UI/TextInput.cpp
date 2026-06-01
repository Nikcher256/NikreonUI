#include "Engine/UI/TextInput.hpp"

#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/Renderer/TextRenderer.hpp"
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

std::string sanitizeSingleLineText(const std::string_view text)
{
    std::string sanitized;
    sanitized.reserve(text.size());
    for (const unsigned char character : text) {
        if (character >= 32 && character != 127) {
            sanitized.push_back(static_cast<char>(character));
        }
    }
    return sanitized;
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
    updateEditing(context, nullptr, nullptr, "default", 1.0f);
}

void TextInput::update(
    UIContext& context,
    const TextRenderer& textRenderer,
    const UIStyle& style,
    const std::string_view fontName,
    const float scale)
{
    const UITextInputStyle& inputStyle = m_styleOverride ? *m_styleOverride : style.resolveTextInput(m_styleClass, m_id);
    updateEditing(context, &textRenderer, &inputStyle, fontName, scale);
}

void TextInput::updateEditing(
    UIContext& context,
    const TextRenderer* textRenderer,
    const UITextInputStyle* inputStyle,
    const std::string_view fontName,
    const float scale)
{
    Widget::update(context);
    if (context.primaryMousePressed() && m_interaction.hovered) {
        context.focus(m_id);
        constexpr float scrollbarInteractionHeight = 7.0f;
        const bool pressedScrollbar = m_horizontalScrollRange > 0.0f &&
            context.mousePosition().y >= m_position.y + m_size.y - scrollbarInteractionHeight;
        if (textRenderer && inputStyle && !pressedScrollbar) {
            moveCaret(caretIndexAt(context.mousePosition().x, *textRenderer, *inputStyle, fontName, scale), context.shiftDown());
        }
    } else if (context.primaryMousePressed() && !m_interaction.hovered && context.isFocused(m_id)) {
        context.clearFocus();
    }

    m_focused = context.isFocused(m_id);
    if (m_focused && m_interaction.held && textRenderer && inputStyle) {
        moveCaret(caretIndexAt(context.mousePosition().x, *textRenderer, *inputStyle, fontName, scale), true);
    }
    if (!m_focused) {
        if (textRenderer && inputStyle) {
            ensureCaretVisible(*textRenderer, *inputStyle, fontName, scale);
            updateHorizontalScrollbar(context, *inputStyle);
        }
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
        case UIKey::SelectAll:
            m_selectionAnchor = 0;
            m_caretIndex = m_value.size();
            break;
        case UIKey::Copy:
            if (hasSelection()) {
                context.setClipboardText(std::string_view{
                    m_value.data() + selectionStart(),
                    selectionEnd() - selectionStart(),
                });
            }
            break;
        case UIKey::Paste: {
            const std::string pastedText = sanitizeSingleLineText(context.clipboardText());
            if (!pastedText.empty()) {
                eraseSelection();
                m_value.insert(m_caretIndex, pastedText);
                m_caretIndex += pastedText.size();
                m_selectionAnchor = m_caretIndex;
                changed = true;
            }
            break;
        }
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

    if (textRenderer && inputStyle) {
        ensureCaretVisible(*textRenderer, *inputStyle, fontName, scale);
        updateHorizontalScrollbar(context, *inputStyle);
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
    if (m_horizontalScrollRange > 0.0f) {
        constexpr float trackHeight = 3.0f;
        const float trackWidth = std::max(m_size.x - inputStyle.box.padding.x * 2.0f, 1.0f);
        const float thumbWidth = std::max(trackWidth * trackWidth / (trackWidth + m_horizontalScrollRange), 18.0f);
        const float thumbTravel = std::max(trackWidth - thumbWidth, 0.0f);
        const float thumbX = m_position.x + inputStyle.box.padding.x +
            thumbTravel * (m_horizontalScrollOffset / m_horizontalScrollRange);
        const float trackY = m_position.y + m_size.y - trackHeight - 2.0f;
        renderer2D.drawSdfRect(
            {m_position.x + inputStyle.box.padding.x, trackY},
            {trackWidth, trackHeight},
            1.5f,
            inputStyle.scrollbarTrack,
            inputStyle.scrollbarTrack,
            0.0f);
        renderer2D.drawSdfRect(
            {thumbX, trackY},
            {thumbWidth, trackHeight},
            1.5f,
            inputStyle.scrollbarThumb,
            inputStyle.scrollbarThumb,
            0.0f);
    }
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

float TextInput::horizontalScrollOffset() const
{
    return m_horizontalScrollOffset;
}

float TextInput::horizontalScrollRange() const
{
    return m_horizontalScrollRange;
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

std::size_t TextInput::caretIndexAt(
    const float mouseX,
    const TextRenderer& textRenderer,
    const UITextInputStyle& inputStyle,
    const std::string_view fontName,
    const float scale) const
{
    const float targetX = std::max(mouseX - m_position.x - inputStyle.box.padding.x + m_horizontalScrollOffset, 0.0f);
    std::size_t boundary = 0;
    float previousWidth = 0.0f;
    while (boundary < m_value.size()) {
        const std::size_t next = nextBoundary(m_value, boundary);
        const float nextWidth = textRenderer.measureText(std::string_view{m_value.data(), next}, fontName, scale).x;
        if (targetX < previousWidth + (nextWidth - previousWidth) * 0.5f) {
            return boundary;
        }
        boundary = next;
        previousWidth = nextWidth;
    }
    return m_value.size();
}

void TextInput::ensureCaretVisible(
    const TextRenderer& textRenderer,
    const UITextInputStyle& inputStyle,
    const std::string_view fontName,
    const float scale)
{
    const float availableWidth = std::max(m_size.x - inputStyle.box.padding.x * 2.0f, 1.0f);
    const float textWidth = textRenderer.measureText(m_value, fontName, scale).x;
    const float caretX = textRenderer.measureText(std::string_view{m_value.data(), m_caretIndex}, fontName, scale).x;
    m_horizontalScrollRange = std::max(textWidth - availableWidth, 0.0f);
    if (caretX < m_horizontalScrollOffset) {
        m_horizontalScrollOffset = caretX;
    } else if (caretX > m_horizontalScrollOffset + availableWidth) {
        m_horizontalScrollOffset = caretX - availableWidth;
    }
    m_horizontalScrollOffset = std::clamp(m_horizontalScrollOffset, 0.0f, m_horizontalScrollRange);
}

void TextInput::updateHorizontalScrollbar(UIContext& context, const UITextInputStyle& inputStyle)
{
    if (m_horizontalScrollRange <= 0.0f) {
        m_scrollThumbWasHeld = false;
        return;
    }

    constexpr float interactionHeight = 7.0f;
    const float trackWidth = std::max(m_size.x - inputStyle.box.padding.x * 2.0f, 1.0f);
    const float thumbWidth = std::max(trackWidth * trackWidth / (trackWidth + m_horizontalScrollRange), 18.0f);
    const float thumbTravel = std::max(trackWidth - thumbWidth, 0.0f);
    const float thumbX = m_position.x + inputStyle.box.padding.x +
        thumbTravel * (m_horizontalScrollOffset / m_horizontalScrollRange);
    const UIInteraction thumb = context.interact(
        m_id + ".horizontal-scrollbar",
        {thumbX, m_position.y + m_size.y - interactionHeight},
        {thumbWidth, interactionHeight});
    if (thumb.held && !m_scrollThumbWasHeld) {
        m_scrollDragStartOffset = m_horizontalScrollOffset;
        m_scrollDragStartMouseX = context.mousePosition().x;
    }
    if (thumb.held && thumbTravel > 0.0f) {
        m_horizontalScrollOffset = std::clamp(
            m_scrollDragStartOffset +
                (context.mousePosition().x - m_scrollDragStartMouseX) * m_horizontalScrollRange / thumbTravel,
            0.0f,
            m_horizontalScrollRange);
    }
    m_scrollThumbWasHeld = thumb.held;
}

} // namespace Engine
