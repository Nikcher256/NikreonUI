#pragma once

#include "Engine/Renderer/TextRenderer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <volk.h>

struct FT_LibraryRec_;
using FT_Library = FT_LibraryRec_*;

namespace Engine {

class VulkanTextRenderer final : public TextRenderer {
public:
    struct Vertex {
        glm::vec2 position{};
        glm::vec2 uv{};
        glm::vec4 color{};
    };

    VulkanTextRenderer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkQueue graphicsQueue,
        VkCommandPool commandPool,
        VkRenderPass renderPass,
        std::size_t maxGlyphs = 8192);
    ~VulkanTextRenderer() override;

    VulkanTextRenderer(const VulkanTextRenderer&) = delete;
    VulkanTextRenderer& operator=(const VulkanTextRenderer&) = delete;
    VulkanTextRenderer(VulkanTextRenderer&&) = delete;
    VulkanTextRenderer& operator=(VulkanTextRenderer&&) = delete;

    void begin(const glm::uvec2& viewportSize) override;
    bool loadFont(std::string_view name, const std::filesystem::path& path, float pixelSize) override;
    void drawText(
        std::string_view text,
        const glm::vec2& position,
        const glm::vec4& color,
        std::string_view fontName = "default",
        float scale = 1.0f,
        TextAlignment alignment = TextAlignment::Left,
        const TextLayout& layout = {}) override;
    void drawSolidRect(
        const glm::vec2& position,
        const glm::vec2& size,
        const glm::vec4& color,
        std::string_view fontName = "default") override;
    [[nodiscard]] glm::vec2 measureText(
        std::string_view text,
        std::string_view fontName = "default",
        float scale = 1.0f,
        const TextLayout& layout = {}) const override;
    void pushClipRect(const UIClipRect& clipRect) override;
    void popClipRect() override;
    void end() override;

    void record(VkCommandBuffer commandBuffer) const;

private:
    struct Glyph {
        glm::vec2 size{0.0f, 0.0f};
        glm::vec2 bearing{0.0f, 0.0f};
        glm::vec2 uvMinimum{0.0f, 0.0f};
        glm::vec2 uvMaximum{0.0f, 0.0f};
        float advance{0.0f};
    };

    struct Font {
        std::unordered_map<char32_t, Glyph> glyphs;
        float ascender{0.0f};
        float lineHeight{0.0f};
        glm::vec2 solidUv{0.0f, 0.0f};
        VkImage image{VK_NULL_HANDLE};
        VkDeviceMemory imageMemory{VK_NULL_HANDLE};
        VkImageView imageView{VK_NULL_HANDLE};
        VkSampler sampler{VK_NULL_HANDLE};
        VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
    };

    struct Batch {
        const Font* font{nullptr};
        UIClipRect clipRect;
        std::uint32_t firstVertex{0};
        std::uint32_t vertexCount{0};
    };

    void createDescriptorResources();
    void createPipeline(VkRenderPass renderPass);
    void createVertexBuffer();
    void destroy();
    void destroyFont(Font& font);
    void uploadVertices();
    void createFontTexture(Font& font, const std::vector<std::uint8_t>& pixels, std::uint32_t width, std::uint32_t height);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory);
    void createImage(std::uint32_t width, std::uint32_t height, VkImage& image, VkDeviceMemory& memory);
    void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, std::uint32_t width, std::uint32_t height);
    [[nodiscard]] VkCommandBuffer beginSingleUseCommands() const;
    void endSingleUseCommands(VkCommandBuffer commandBuffer) const;
    void setViewportAndScissor(VkCommandBuffer commandBuffer) const;
    void setScissor(VkCommandBuffer commandBuffer, const UIClipRect& clipRect) const;
    [[nodiscard]] UIClipRect currentClipRect() const;
    [[nodiscard]] glm::vec2 toNdc(const glm::vec2& pixelPosition) const;
    [[nodiscard]] VkShaderModule createShaderModule(const std::vector<char>& bytecode) const;
    [[nodiscard]] std::uint32_t findMemoryType(std::uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    [[nodiscard]] const Font* findFont(std::string_view name) const;

    static std::vector<char> readFile(const char* path);
    static VkVertexInputBindingDescription vertexBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescriptions();

    VkDevice m_device{VK_NULL_HANDLE};
    VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
    VkQueue m_graphicsQueue{VK_NULL_HANDLE};
    VkCommandPool m_commandPool{VK_NULL_HANDLE};
    VkDescriptorSetLayout m_descriptorSetLayout{VK_NULL_HANDLE};
    VkDescriptorPool m_descriptorPool{VK_NULL_HANDLE};
    VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
    VkPipeline m_pipeline{VK_NULL_HANDLE};
    VkBuffer m_vertexBuffer{VK_NULL_HANDLE};
    VkDeviceMemory m_vertexBufferMemory{VK_NULL_HANDLE};
    VkDeviceSize m_vertexBufferSize{0};
    FT_Library m_freetype{nullptr};
    glm::uvec2 m_viewportSize{};
    std::size_t m_maxGlyphs{0};
    std::vector<Vertex> m_vertices;
    std::vector<Batch> m_batches;
    std::vector<UIClipRect> m_clipStack;
    std::unordered_map<std::string, Font> m_fonts;
};

} // namespace Engine
