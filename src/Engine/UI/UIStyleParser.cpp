#include "Engine/UI/UIStyleParser.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

namespace Engine {

namespace {

std::string trim(const std::string_view text)
{
    const auto first = std::find_if_not(text.begin(), text.end(), [](const unsigned char character) {
        return std::isspace(character) != 0;
    });
    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](const unsigned char character) {
        return std::isspace(character) != 0;
    }).base();

    return first < last ? std::string(first, last) : std::string{};
}

std::string stripComments(const std::string_view source, std::string& error)
{
    std::string stripped;
    stripped.reserve(source.size());

    for (std::size_t index = 0; index < source.size();) {
        if (index + 1 < source.size() && source[index] == '/' && source[index + 1] == '*') {
            const std::size_t end = source.find("*/", index + 2);
            if (end == std::string_view::npos) {
                error = "Unterminated stylesheet comment.";
                return {};
            }

            for (; index < end + 2; ++index) {
                stripped.push_back(source[index] == '\n' ? '\n' : ' ');
            }
            continue;
        }

        stripped.push_back(source[index]);
        ++index;
    }

    return stripped;
}

bool parseFloats(const std::string_view text, const std::size_t expectedCount, std::vector<float>& values)
{
    std::string normalized{text};
    std::replace(normalized.begin(), normalized.end(), ',', ' ');

    std::istringstream stream(normalized);
    values.clear();

    float value = 0.0f;
    while (stream >> value) {
        values.push_back(value);
    }

    stream.clear();
    std::string remainder;
    stream >> remainder;
    return values.size() == expectedCount && remainder.empty();
}

bool parseFloat(const std::string_view text, float& value)
{
    std::vector<float> values;
    if (!parseFloats(text, 1, values)) {
        return false;
    }

    value = values.front();
    return true;
}

bool parseBool(const std::string_view text, bool& value)
{
    if (text == "true") {
        value = true;
        return true;
    }
    if (text == "false") {
        value = false;
        return true;
    }
    return false;
}

bool parseVec2(const std::string_view text, glm::vec2& value)
{
    std::vector<float> values;
    if (!parseFloats(text, 2, values)) {
        return false;
    }

    value = {values[0], values[1]};
    return true;
}

bool parseVec4(const std::string_view text, glm::vec4& value)
{
    std::vector<float> values;
    if (!parseFloats(text, 4, values)) {
        return false;
    }

    value = {values[0], values[1], values[2], values[3]};
    return true;
}

bool applyBoxProperty(UIBoxStyle& box, const std::string_view property, const std::string_view value)
{
    if (property == "fill") {
        return parseVec4(value, box.fill);
    }
    if (property == "border-color") {
        return parseVec4(value, box.border);
    }
    if (property == "border-width") {
        return parseFloat(value, box.borderWidth);
    }
    if (property == "border-radius") {
        return parseFloat(value, box.borderRadius);
    }
    if (property == "padding") {
        return parseVec2(value, box.padding);
    }

    return false;
}

struct StyleSelector {
    std::string type;
    std::string styleClass;
    std::string id;
};

StyleSelector parseSelector(const std::string_view selector)
{
    const std::size_t classSeparator = selector.find('.');
    const std::size_t idSeparator = selector.find('#');
    const std::size_t typeEnd = std::min(
        classSeparator == std::string_view::npos ? selector.size() : classSeparator,
        idSeparator == std::string_view::npos ? selector.size() : idSeparator);

    StyleSelector parsed;
    parsed.type = std::string(selector.substr(0, typeEnd));

    if (classSeparator != std::string_view::npos) {
        const std::size_t classEnd = idSeparator == std::string_view::npos ? selector.size() : idSeparator;
        if (classSeparator < classEnd) {
            parsed.styleClass = std::string(selector.substr(classSeparator + 1, classEnd - classSeparator - 1));
        }
    }

    if (idSeparator != std::string_view::npos) {
        parsed.id = std::string(selector.substr(idSeparator + 1));
    }

    return parsed;
}

UIButtonStyle& resolveButton(UIStyle& style, const std::string& styleClass, const std::string& id)
{
    if (!id.empty()) {
        const UIButtonStyle base = styleClass.empty()
            ? style.button
            : style.buttonClasses.try_emplace(styleClass, style.button).first->second;
        return style.buttonIds.try_emplace(id, base).first->second;
    }

    if (styleClass.empty()) {
        return style.button;
    }

    return style.buttonClasses.try_emplace(styleClass, style.button).first->second;
}

UICheckboxStyle& resolveCheckbox(UIStyle& style, const std::string& styleClass, const std::string& id)
{
    if (!id.empty()) {
        const UICheckboxStyle base = styleClass.empty()
            ? style.checkbox
            : style.checkboxClasses.try_emplace(styleClass, style.checkbox).first->second;
        return style.checkboxIds.try_emplace(id, base).first->second;
    }

    if (styleClass.empty()) {
        return style.checkbox;
    }

    return style.checkboxClasses.try_emplace(styleClass, style.checkbox).first->second;
}

UISliderStyle& resolveSlider(UIStyle& style, const std::string& styleClass, const std::string& id)
{
    if (!id.empty()) {
        const UISliderStyle base = styleClass.empty()
            ? style.slider
            : style.sliderClasses.try_emplace(styleClass, style.slider).first->second;
        return style.sliderIds.try_emplace(id, base).first->second;
    }

    if (styleClass.empty()) {
        return style.slider;
    }

    return style.sliderClasses.try_emplace(styleClass, style.slider).first->second;
}

UINumberInputStyle& resolveNumberInput(UIStyle& style, const std::string& styleClass, const std::string& id)
{
    if (!id.empty()) {
        const UINumberInputStyle base = styleClass.empty()
            ? style.numberInput
            : style.numberInputClasses.try_emplace(styleClass, style.numberInput).first->second;
        return style.numberInputIds.try_emplace(id, base).first->second;
    }

    if (styleClass.empty()) {
        return style.numberInput;
    }

    return style.numberInputClasses.try_emplace(styleClass, style.numberInput).first->second;
}

UITextInputStyle& resolveTextInput(UIStyle& style, const std::string& styleClass, const std::string& id)
{
    if (!id.empty()) {
        const UITextInputStyle base = styleClass.empty()
            ? style.textInput
            : style.textInputClasses.try_emplace(styleClass, style.textInput).first->second;
        return style.textInputIds.try_emplace(id, base).first->second;
    }

    if (styleClass.empty()) {
        return style.textInput;
    }

    return style.textInputClasses.try_emplace(styleClass, style.textInput).first->second;
}

UITextStyle& resolveText(UIStyle& style, const std::string& styleClass, const std::string& id)
{
    if (!id.empty()) {
        const UITextStyle base = styleClass.empty()
            ? style.text
            : style.textClasses.try_emplace(styleClass, style.text).first->second;
        return style.textIds.try_emplace(id, base).first->second;
    }

    if (styleClass.empty()) {
        return style.text;
    }

    return style.textClasses.try_emplace(styleClass, style.text).first->second;
}

bool applyButtonProperty(UIButtonStyle& button, const std::string_view property, const std::string_view value)
{
    if (applyBoxProperty(button.normal, property, value)) {
        return true;
    }
    if (property == "hover-fill") {
        return parseVec4(value, button.hovered);
    }
    if (property == "pressed-fill") {
        return parseVec4(value, button.pressed);
    }
    if (property == "selected-fill") {
        return parseVec4(value, button.selected);
    }
    if (property == "selected-border-color") {
        return parseVec4(value, button.selectedBorder);
    }
    if (property == "icon-color") {
        return parseVec4(value, button.icon);
    }
    if (property == "selected-icon-color") {
        return parseVec4(value, button.selectedIcon);
    }
    if (property == "accent") {
        return parseVec4(value, button.accent);
    }

    return false;
}

bool applyCheckboxProperty(UICheckboxStyle& checkbox, const std::string_view property, const std::string_view value)
{
    if (applyBoxProperty(checkbox.box, property, value)) {
        return true;
    }
    if (property == "hover-fill") {
        return parseVec4(value, checkbox.hovered);
    }
    if (property == "accent") {
        return parseVec4(value, checkbox.check);
    }

    return false;
}

bool applySliderProperty(UISliderStyle& slider, const std::string_view property, const std::string_view value)
{
    if (applyBoxProperty(slider.track, property, value)) {
        return true;
    }
    if (property == "hover-fill") {
        return parseVec4(value, slider.hovered);
    }
    if (property == "accent") {
        return parseVec4(value, slider.fill);
    }
    if (property == "knob-color") {
        return parseVec4(value, slider.knob);
    }

    return false;
}

bool applyNumberInputProperty(UINumberInputStyle& numberInput, const std::string_view property, const std::string_view value)
{
    if (applyBoxProperty(numberInput.box, property, value)) {
        return true;
    }
    if (property == "hover-fill") {
        return parseVec4(value, numberInput.hovered);
    }
    if (property == "accent") {
        return parseVec4(value, numberInput.accent);
    }
    if (property == "show-value-fill") {
        return parseBool(value, numberInput.showValueFill);
    }

    return false;
}

bool applyTextInputProperty(UITextInputStyle& textInput, const std::string_view property, const std::string_view value)
{
    if (applyBoxProperty(textInput.box, property, value)) {
        return true;
    }
    if (property == "hover-fill") {
        return parseVec4(value, textInput.hovered);
    }
    if (property == "focused-fill") {
        return parseVec4(value, textInput.focused);
    }
    if (property == "focused-border-color") {
        return parseVec4(value, textInput.focusedBorder);
    }
    if (property == "scrollbar-track-color") {
        return parseVec4(value, textInput.scrollbarTrack);
    }
    if (property == "scrollbar-thumb-color") {
        return parseVec4(value, textInput.scrollbarThumb);
    }

    return false;
}

bool applyTextProperty(UITextStyle& text, const std::string_view property, const std::string_view value)
{
    if (property == "color") {
        return parseVec4(value, text.color);
    }
    if (property == "font") {
        text.font = trim(value);
        return !text.font.empty();
    }
    if (property == "font-scale") {
        return parseFloat(value, text.scale);
    }
    if (property == "opacity") {
        return parseFloat(value, text.opacity);
    }
    if (property == "offset") {
        return parseVec2(value, text.offset);
    }
    if (property == "text-align") {
        if (value == "left") {
            text.horizontalAlignment = UITextHorizontalAlignment::Left;
            return true;
        }
        if (value == "center") {
            text.horizontalAlignment = UITextHorizontalAlignment::Center;
            return true;
        }
        if (value == "right") {
            text.horizontalAlignment = UITextHorizontalAlignment::Right;
            return true;
        }
        return false;
    }
    if (property == "vertical-align") {
        if (value == "top") {
            text.verticalAlignment = UITextVerticalAlignment::Top;
            return true;
        }
        if (value == "center") {
            text.verticalAlignment = UITextVerticalAlignment::Center;
            return true;
        }
        if (value == "bottom") {
            text.verticalAlignment = UITextVerticalAlignment::Bottom;
            return true;
        }
        return false;
    }

    return false;
}

bool applyGlobalProperty(UIStyle& style, const std::string_view property, const std::string_view value)
{
    if (property == "spacing") {
        return parseFloat(value, style.gap);
    }
    if (property == "toolbar-height-min") {
        return parseFloat(value, style.toolbarHeightMin);
    }
    if (property == "toolbar-height-max") {
        return parseFloat(value, style.toolbarHeightMax);
    }

    return false;
}

bool applyProperty(UIStyle& style, const std::string_view selector, const std::string_view property, const std::string_view value)
{
    if (selector == "ui") {
        return applyGlobalProperty(style, property, value);
    }
    if (selector == "toolbar") {
        return applyBoxProperty(style.toolbar, property, value);
    }
    if (selector == "panel") {
        return applyBoxProperty(style.panel, property, value);
    }
    if (selector == "field") {
        return applyBoxProperty(style.field, property, value);
    }

    const StyleSelector parsed = parseSelector(selector);

    if (parsed.type == "button") {
        return applyButtonProperty(resolveButton(style, parsed.styleClass, parsed.id), property, value);
    }
    if (parsed.type == "checkbox") {
        return applyCheckboxProperty(resolveCheckbox(style, parsed.styleClass, parsed.id), property, value);
    }
    if (parsed.type == "slider") {
        return applySliderProperty(resolveSlider(style, parsed.styleClass, parsed.id), property, value);
    }
    if (parsed.type == "number-input") {
        return applyNumberInputProperty(resolveNumberInput(style, parsed.styleClass, parsed.id), property, value);
    }
    if (parsed.type == "text-input") {
        return applyTextInputProperty(resolveTextInput(style, parsed.styleClass, parsed.id), property, value);
    }
    if (parsed.type == "text") {
        return applyTextProperty(resolveText(style, parsed.styleClass, parsed.id), property, value);
    }

    return false;
}

bool parseRule(UIStyle& style, const std::string_view selectorText, const std::string_view body, std::string& error)
{
    const std::string selector = trim(selectorText);
    if (selector.empty()) {
        error = "Stylesheet rule is missing a selector.";
        return false;
    }

    std::size_t declarationStart = 0;
    while (declarationStart < body.size()) {
        const std::size_t declarationEnd = body.find(';', declarationStart);
        const std::string declaration = trim(body.substr(
            declarationStart,
            declarationEnd == std::string_view::npos ? body.size() - declarationStart : declarationEnd - declarationStart));

        if (!declaration.empty()) {
            const std::size_t separator = declaration.find(':');
            if (separator == std::string::npos) {
                error = "Stylesheet declaration is missing ':' in selector '" + selector + "'.";
                return false;
            }

            const std::string property = trim(std::string_view(declaration).substr(0, separator));
            const std::string value = trim(std::string_view(declaration).substr(separator + 1));
            if (!applyProperty(style, selector, property, value)) {
                error = "Unsupported or invalid property '" + property + "' in selector '" + selector + "'.";
                return false;
            }
        }

        if (declarationEnd == std::string_view::npos) {
            break;
        }
        declarationStart = declarationEnd + 1;
    }

    return true;
}

} // namespace

bool UIStyleParser::loadFile(const std::filesystem::path& path, UIStyle& style, std::string& error)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        error = "Could not open UI stylesheet: " + path.string();
        return false;
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    return parse(contents.str(), style, error);
}

bool UIStyleParser::parse(const std::string_view source, UIStyle& style, std::string& error)
{
    error.clear();
    const std::string stripped = stripComments(source, error);
    if (!error.empty()) {
        return false;
    }

    UIStyle parsed = style;
    std::size_t ruleStart = 0;
    while (ruleStart < stripped.size()) {
        const std::size_t openBrace = stripped.find('{', ruleStart);
        if (openBrace == std::string::npos) {
            if (!trim(std::string_view(stripped).substr(ruleStart)).empty()) {
                error = "Stylesheet has trailing text outside a rule.";
                return false;
            }
            break;
        }

        const std::size_t closeBrace = stripped.find('}', openBrace + 1);
        if (closeBrace == std::string::npos) {
            error = "Stylesheet rule is missing '}'.";
            return false;
        }

        if (!parseRule(
                parsed,
                std::string_view(stripped).substr(ruleStart, openBrace - ruleStart),
                std::string_view(stripped).substr(openBrace + 1, closeBrace - openBrace - 1),
                error)) {
            return false;
        }

        ruleStart = closeBrace + 1;
    }

    style = std::move(parsed);
    return true;
}

} // namespace Engine
