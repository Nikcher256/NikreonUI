#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "Engine/UI/UIStyle.hpp"

namespace Engine {

class UIStyleParser {
public:
    [[nodiscard]] static bool loadFile(const std::filesystem::path& path, UIStyle& style, std::string& error);
    [[nodiscard]] static bool parse(std::string_view source, UIStyle& style, std::string& error);
};

} // namespace Engine
