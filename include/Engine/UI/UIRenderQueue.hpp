#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

enum class UIRenderLayer : int {
    Background = 0,
    Content = 100,
    Overlay = 200,
    Popup = 300,
    Tooltip = 400,
};

class UIRenderQueue {
public:
    void clear();
    void add(UIRenderLayer layer, std::function<void()> command);
    void add(int zIndex, std::function<void()> command);
    void flush();

private:
    struct Entry {
        int zIndex{0};
        std::uint64_t order{0};
        std::function<void()> command;
    };

    std::vector<Entry> m_entries;
    std::uint64_t m_nextOrder{0};
};

} // namespace Engine
