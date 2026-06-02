#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace Engine {

struct UIBoxStyle {
    glm::vec4 fill{0.12f, 0.13f, 0.16f, 1.0f};
    glm::vec4 border{0.25f, 0.29f, 0.36f, 1.0f};
    float borderWidth{0.0f};
    float borderRadius{0.0f};
    glm::vec2 padding{8.0f, 6.0f};
};

struct UIButtonStyle {
    UIBoxStyle normal{
        {0.16f, 0.18f, 0.23f, 1.0f},
        {0.26f, 0.30f, 0.38f, 1.0f},
        0.0f,
        4.0f,
        {10.0f, 6.0f},
    };
    glm::vec4 hovered{0.20f, 0.23f, 0.30f, 1.0f};
    glm::vec4 pressed{0.10f, 0.14f, 0.20f, 1.0f};
    glm::vec4 selected{0.20f, 0.48f, 0.82f, 1.0f};
    glm::vec4 selectedBorder{0.44f, 0.70f, 1.0f, 1.0f};
    glm::vec4 icon{0.56f, 0.62f, 0.72f, 1.0f};
    glm::vec4 selectedIcon{0.92f, 0.98f, 1.0f, 1.0f};
    glm::vec4 accent{0.52f, 0.78f, 1.0f, 1.0f};
};

struct UICheckboxStyle {
    UIBoxStyle box{
        {0.13f, 0.145f, 0.18f, 1.0f},
        {0.28f, 0.33f, 0.42f, 1.0f},
        0.0f,
        3.0f,
        {4.0f, 4.0f},
    };
    glm::vec4 hovered{0.18f, 0.21f, 0.27f, 1.0f};
    glm::vec4 check{0.30f, 0.62f, 0.95f, 1.0f};
};

struct UISliderStyle {
    UIBoxStyle track{
        {0.13f, 0.145f, 0.18f, 1.0f},
        {0.24f, 0.29f, 0.36f, 1.0f},
        0.0f,
        4.0f,
        {0.0f, 0.0f},
    };
    glm::vec4 hovered{0.18f, 0.21f, 0.27f, 1.0f};
    glm::vec4 fill{0.30f, 0.52f, 0.78f, 1.0f};
    glm::vec4 knob{0.72f, 0.84f, 0.96f, 1.0f};
};

struct UINumberInputStyle {
    UIBoxStyle box{
        {0.13f, 0.145f, 0.18f, 1.0f},
        {0.24f, 0.29f, 0.36f, 1.0f},
        1.0f,
        4.0f,
        {8.0f, 5.0f},
    };
    glm::vec4 hovered{0.18f, 0.21f, 0.27f, 1.0f};
    glm::vec4 accent{0.36f, 0.58f, 0.82f, 1.0f};
    bool showValueFill{true};
};

struct UITextInputStyle {
    UIBoxStyle box{
        {0.13f, 0.145f, 0.18f, 1.0f},
        {0.24f, 0.29f, 0.36f, 1.0f},
        1.0f,
        4.0f,
        {8.0f, 5.0f},
    };
    glm::vec4 hovered{0.18f, 0.21f, 0.27f, 1.0f};
    glm::vec4 focused{0.16f, 0.19f, 0.25f, 1.0f};
    glm::vec4 focusedBorder{0.44f, 0.70f, 1.0f, 1.0f};
    glm::vec4 scrollbarTrack{0.10f, 0.12f, 0.16f, 1.0f};
    glm::vec4 scrollbarThumb{0.44f, 0.70f, 1.0f, 1.0f};
    glm::vec4 text{0.86f, 0.92f, 1.0f, 1.0f};
    glm::vec4 placeholder{0.48f, 0.55f, 0.66f, 1.0f};
    glm::vec4 selection{0.30f, 0.58f, 0.96f, 0.58f};
    glm::vec4 caret{0.92f, 0.98f, 1.0f, 1.0f};
};

enum class UITextHorizontalAlignment {
    Left,
    Center,
    Right,
};

enum class UITextVerticalAlignment {
    Top,
    Center,
    Bottom,
};

struct UITextStyle {
    glm::vec4 color{0.66f, 0.72f, 0.82f, 1.0f};
    std::string font{"default"};
    float scale{1.0f};
    float opacity{1.0f};
    glm::vec2 offset{0.0f, 0.0f};
    UITextHorizontalAlignment horizontalAlignment{UITextHorizontalAlignment::Left};
    UITextVerticalAlignment verticalAlignment{UITextVerticalAlignment::Top};
};

struct UIProgressBarStyle {
    UIBoxStyle track;
    glm::vec4 fill{0.30f, 0.62f, 0.95f, 1.0f};
};

struct UIScrollbarStyle {
    glm::vec4 track{0.10f, 0.12f, 0.16f, 0.9f};
    glm::vec4 thumb{0.42f, 0.52f, 0.68f, 0.9f};
    float width{6.0f};
    float minimumThumbLength{18.0f};
};

struct UIStyle {
    glm::vec4 windowBackground{0.07f, 0.075f, 0.09f, 1.0f};
    UIBoxStyle panel{
        {0.095f, 0.105f, 0.13f, 1.0f},
        {0.20f, 0.23f, 0.29f, 1.0f},
        0.0f,
        6.0f,
        {12.0f, 12.0f},
    };
    UIBoxStyle field{
        {0.13f, 0.145f, 0.18f, 1.0f},
        {0.22f, 0.26f, 0.32f, 1.0f},
        0.0f,
        4.0f,
        {8.0f, 5.0f},
    };
    UIButtonStyle button;
    UICheckboxStyle checkbox;
    UISliderStyle slider;
    UINumberInputStyle numberInput;
    UITextInputStyle textInput;
    UITextStyle text;
    UIProgressBarStyle progressBar;
    UIScrollbarStyle scrollbar;
    std::unordered_map<std::string, UIButtonStyle> buttonClasses;
    std::unordered_map<std::string, UICheckboxStyle> checkboxClasses;
    std::unordered_map<std::string, UISliderStyle> sliderClasses;
    std::unordered_map<std::string, UINumberInputStyle> numberInputClasses;
    std::unordered_map<std::string, UITextInputStyle> textInputClasses;
    std::unordered_map<std::string, UITextStyle> textClasses;
    std::unordered_map<std::string, UIButtonStyle> buttonIds;
    std::unordered_map<std::string, UICheckboxStyle> checkboxIds;
    std::unordered_map<std::string, UISliderStyle> sliderIds;
    std::unordered_map<std::string, UINumberInputStyle> numberInputIds;
    std::unordered_map<std::string, UITextInputStyle> textInputIds;
    std::unordered_map<std::string, UITextStyle> textIds;
    float gap{8.0f};

    [[nodiscard]] const UIButtonStyle& resolveButton(std::string_view styleClass, std::string_view id = {}) const
    {
        if (!id.empty()) {
            const auto found = buttonIds.find(std::string(id));
            if (found != buttonIds.end()) {
                return found->second;
            }
        }

        if (!styleClass.empty()) {
            const auto found = buttonClasses.find(std::string(styleClass));
            if (found != buttonClasses.end()) {
                return found->second;
            }
        }

        return button;
    }

    [[nodiscard]] const UICheckboxStyle& resolveCheckbox(std::string_view styleClass, std::string_view id = {}) const
    {
        if (!id.empty()) {
            const auto found = checkboxIds.find(std::string(id));
            if (found != checkboxIds.end()) {
                return found->second;
            }
        }

        if (!styleClass.empty()) {
            const auto found = checkboxClasses.find(std::string(styleClass));
            if (found != checkboxClasses.end()) {
                return found->second;
            }
        }

        return checkbox;
    }

    [[nodiscard]] const UISliderStyle& resolveSlider(std::string_view styleClass, std::string_view id = {}) const
    {
        if (!id.empty()) {
            const auto found = sliderIds.find(std::string(id));
            if (found != sliderIds.end()) {
                return found->second;
            }
        }

        if (!styleClass.empty()) {
            const auto found = sliderClasses.find(std::string(styleClass));
            if (found != sliderClasses.end()) {
                return found->second;
            }
        }

        return slider;
    }

    [[nodiscard]] const UINumberInputStyle& resolveNumberInput(std::string_view styleClass, std::string_view id = {}) const
    {
        if (!id.empty()) {
            const auto found = numberInputIds.find(std::string(id));
            if (found != numberInputIds.end()) {
                return found->second;
            }
        }

        if (!styleClass.empty()) {
            const auto found = numberInputClasses.find(std::string(styleClass));
            if (found != numberInputClasses.end()) {
                return found->second;
            }
        }

        return numberInput;
    }

    [[nodiscard]] const UITextInputStyle& resolveTextInput(std::string_view styleClass, std::string_view id = {}) const
    {
        if (!id.empty()) {
            const auto found = textInputIds.find(std::string(id));
            if (found != textInputIds.end()) {
                return found->second;
            }
        }

        if (!styleClass.empty()) {
            const auto found = textInputClasses.find(std::string(styleClass));
            if (found != textInputClasses.end()) {
                return found->second;
            }
        }

        return textInput;
    }

    [[nodiscard]] const UITextStyle& resolveText(std::string_view styleClass, std::string_view id = {}) const
    {
        if (!id.empty()) {
            const auto found = textIds.find(std::string(id));
            if (found != textIds.end()) {
                return found->second;
            }
        }

        if (!styleClass.empty()) {
            const auto found = textClasses.find(std::string(styleClass));
            if (found != textClasses.end()) {
                return found->second;
            }
        }

        return text;
    }
};

} // namespace Engine
