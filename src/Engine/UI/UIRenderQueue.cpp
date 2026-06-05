#include "Engine/UI/UIRenderQueue.hpp"

#include <algorithm>

namespace Engine {

void UIRenderQueue::clear()
{
    m_entries.clear();
    m_nextOrder = 0;
}

void UIRenderQueue::add(const UIRenderLayer layer, std::function<void()> command)
{
    add(static_cast<int>(layer), std::move(command));
}

void UIRenderQueue::add(const int zIndex, std::function<void()> command)
{
    m_entries.push_back({zIndex, m_nextOrder++, std::move(command)});
}

void UIRenderQueue::flush()
{
    std::stable_sort(m_entries.begin(), m_entries.end(), [](const Entry& left, const Entry& right) {
        if (left.zIndex != right.zIndex) {
            return left.zIndex < right.zIndex;
        }
        return left.order < right.order;
    });

    for (const Entry& entry : m_entries) {
        if (entry.command) {
            entry.command();
        }
    }

    clear();
}

} // namespace Engine
