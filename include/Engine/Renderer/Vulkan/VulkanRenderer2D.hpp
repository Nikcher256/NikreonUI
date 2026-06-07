#pragma once

#include "Engine/Renderer/Renderer2D.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <volk.h>

namespace Engine {

class VulkanRenderer2D final : public Renderer2D {
public:
    struct Vertex {
        glm::vec2 position{};
        glm::vec4 color{};
        glm::vec2 uv{};
    };

    struct SdfVertex {
        glm::vec2 position{};
        glm::vec2 localPosition{};
    };

    struct SdfInstance {
        glm::vec2 position{};
        glm::vec2 ndcSize{};
        glm::vec2 rectSize{};
        glm::vec4 fillColor{};
        glm::vec4 borderColor{};
        float radius{0.0f};
        float borderWidth{0.0f};
    };

    VulkanRenderer2D(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkQueue graphicsQueue,
        VkCommandPool commandPool,
        VkRenderPass renderPass,
        std::size_t maxQuads = 4096);
    ~VulkanRenderer2D() override;

    VulkanRenderer2D(const VulkanRenderer2D&) = delete;
    VulkanRenderer2D& operator=(const VulkanRenderer2D&) = delete;
    VulkanRenderer2D(VulkanRenderer2D&&) = delete;
    VulkanRenderer2D& operator=(VulkanRenderer2D&&) = delete;

    void begin(const glm::uvec2& viewportSize) override;
    [[nodiscard]] std::uint64_t reserveRenderOrder() override;
    void beginCompositeRenderItem(std::uint64_t renderOrder) override;
    void endCompositeRenderItem() override;
    void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) override;
    void drawGradientQuad(
    const glm::vec2& position,
    const glm::vec2& size,
    const glm::vec4& topLeft,
    const glm::vec4& topRight,
        const glm::vec4& bottomRight,
        const glm::vec4& bottomLeft) override;
    void drawImage(
        UITextureId texture,
        const glm::vec2& position,
        const glm::vec2& size,
        const glm::vec2& uvMinimum = {0.0f, 0.0f},
        const glm::vec2& uvMaximum = {1.0f, 1.0f},
        const glm::vec4& tint = {1.0f, 1.0f, 1.0f, 1.0f}) override;
    void uploadImage(UITextureId texture, std::uint32_t width, std::uint32_t height, const std::uint8_t* rgba8, std::size_t byteCount) override;
    void drawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, float thickness = 1.0f) override;
    void drawSdfRect(
        const glm::vec2& position,
        const glm::vec2& size,
        float radius,
        const glm::vec4& fillColor,
        const glm::vec4& borderColor,
        float borderWidth = 0.0f) override;
    void pushClipRect(const UIClipRect& clipRect) override;
    void popClipRect() override;
    void end() override;
    void record(VkCommandBuffer commandBuffer);
    void record(VkCommandBuffer commandBuffer, std::uint64_t renderOrder) const;
    void appendRenderOrders(std::vector<std::uint64_t>& renderOrders) const;

    [[nodiscard]] std::size_t quadCount() const;
    [[nodiscard]] std::size_t maxQuads() const;

private:
    void createPipeline(VkRenderPass renderPass);
    void createSdfPipeline(VkRenderPass renderPass);
    void createDescriptorResources();
    void createFallbackTexture();
    void destroyTextureResources();
    void createVertexBuffer();
    void createSdfBuffers();
    void destroy();
    void uploadVertices();
    void uploadSdfInstances();
    void setViewportAndScissor(VkCommandBuffer commandBuffer) const;
    void setScissor(VkCommandBuffer commandBuffer, const UIClipRect& clipRect) const;
    [[nodiscard]] UIClipRect currentClipRect() const;

    enum class DrawCommandType {
        Quad,
        SdfRect,
    };

    struct DrawCommand {
        DrawCommandType type{DrawCommandType::Quad};
        std::uint64_t renderOrder{0};
        UIClipRect clipRect;
        std::uint32_t first{0};
        std::uint32_t count{0};
        VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
    };

    struct TextureResource {
        VkImage image{VK_NULL_HANDLE};
        VkDeviceMemory memory{VK_NULL_HANDLE};
        VkImageView imageView{VK_NULL_HANDLE};
        VkSampler sampler{VK_NULL_HANDLE};
        VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
        std::uint32_t width{0};
        std::uint32_t height{0};
    };

    void appendDrawCommand(DrawCommandType type, UIClipRect clipRect, std::uint32_t first, std::uint32_t count, VkDescriptorSet descriptorSet = VK_NULL_HANDLE);
    [[nodiscard]] std::uint64_t currentRenderOrder();
    [[nodiscard]] VkDescriptorSet descriptorSetForTexture(UITextureId texture) const;
    TextureResource createTextureResource(std::uint32_t width, std::uint32_t height, const std::uint8_t* rgba8, std::size_t byteCount);
    void destroyTextureResource(TextureResource& texture);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& memory);
    void createDeviceBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory);
    void createImage(std::uint32_t width, std::uint32_t height, VkImage& image, VkDeviceMemory& memory);
    void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, std::uint32_t width, std::uint32_t height);
    [[nodiscard]] VkCommandBuffer beginSingleUseCommands() const;
    void endSingleUseCommands(VkCommandBuffer commandBuffer) const;
    [[nodiscard]] glm::vec2 toNdc(const glm::vec2& pixelPosition) const;
    [[nodiscard]] VkShaderModule createShaderModule(const std::vector<char>& bytecode) const;
    [[nodiscard]] std::uint32_t findMemoryType(std::uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    static std::vector<char> readFile(const char* path);
    static VkVertexInputBindingDescription vertexBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescriptions();
    static std::array<VkVertexInputBindingDescription, 2> sdfVertexBindingDescriptions();
    static std::array<VkVertexInputAttributeDescription, 9> sdfVertexAttributeDescriptions();

    VkDevice m_device{VK_NULL_HANDLE};
    VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
    VkQueue m_graphicsQueue{VK_NULL_HANDLE};
    VkCommandPool m_commandPool{VK_NULL_HANDLE};
    VkDescriptorSetLayout m_descriptorSetLayout{VK_NULL_HANDLE};
    VkDescriptorPool m_descriptorPool{VK_NULL_HANDLE};
    VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
    VkPipeline m_pipeline{VK_NULL_HANDLE};
    VkPipelineLayout m_sdfPipelineLayout{VK_NULL_HANDLE};
    VkPipeline m_sdfPipeline{VK_NULL_HANDLE};
    VkBuffer m_vertexBuffer{VK_NULL_HANDLE};
    VkDeviceMemory m_vertexBufferMemory{VK_NULL_HANDLE};
    VkDeviceSize m_vertexBufferSize{0};
    VkBuffer m_sdfVertexBuffer{VK_NULL_HANDLE};
    VkDeviceMemory m_sdfVertexBufferMemory{VK_NULL_HANDLE};
    VkDeviceSize m_sdfVertexBufferSize{0};
    VkBuffer m_sdfInstanceBuffer{VK_NULL_HANDLE};
    VkDeviceMemory m_sdfInstanceBufferMemory{VK_NULL_HANDLE};
    VkDeviceSize m_sdfInstanceBufferSize{0};
    glm::uvec2 m_viewportSize{};
    std::size_t m_maxQuads{0};
    std::vector<Vertex> m_vertices;
    std::vector<SdfInstance> m_sdfInstances;
    std::vector<DrawCommand> m_drawCommands;
    std::vector<UIClipRect> m_clipStack;
    TextureResource m_fallbackTexture{};
    std::unordered_map<UITextureId, TextureResource> m_textures;
    std::uint64_t m_nextRenderOrder{1};
    std::uint64_t m_currentCompositeRenderOrder{0};
    bool m_compositeRenderItemActive{false};
};

} // namespace Engine
