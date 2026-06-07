#include "Engine/Renderer/Vulkan/VulkanRenderer2D.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <optional>
#include <stdexcept>

#include <glm/common.hpp>

namespace Engine {

namespace {

constexpr std::size_t VerticesPerQuad = 6;
constexpr std::uint64_t UncompositedShapeRenderOrder = 0;

bool sameClipRect(const UIClipRect& left, const UIClipRect& right)
{
    return left.position == right.position && left.size == right.size;
}

UIClipRect intersectClipRects(const UIClipRect& left, const UIClipRect& right)
{
    const glm::vec2 minimum = glm::max(left.position, right.position);
    const glm::vec2 maximum = glm::min(left.position + left.size, right.position + right.size);
    return {minimum, glm::max(maximum - minimum, glm::vec2{0.0f, 0.0f})};
}

#ifndef NIKREON_SHADER_DIR
#define NIKREON_SHADER_DIR "shaders"
#endif

void checkVk(const VkResult result, const char* message)
{
    if (result != VK_SUCCESS) {
        throw std::runtime_error(message);
    }
}

} // namespace

VulkanRenderer2D::VulkanRenderer2D(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkQueue graphicsQueue,
    VkCommandPool commandPool,
    VkRenderPass renderPass,
    const std::size_t maxQuads)
    : m_device(device)
    , m_physicalDevice(physicalDevice)
    , m_graphicsQueue(graphicsQueue)
    , m_commandPool(commandPool)
    , m_maxQuads(maxQuads)
{
    m_vertices.reserve(m_maxQuads * VerticesPerQuad);
    m_sdfInstances.reserve(m_maxQuads);
    createDescriptorResources();
    createFallbackTexture();
    createVertexBuffer();
    createSdfBuffers();
    createPipeline(renderPass);
    createSdfPipeline(renderPass);
}

VulkanRenderer2D::~VulkanRenderer2D()
{
    destroy();
}

void VulkanRenderer2D::begin(const glm::uvec2& viewportSize)
{
    m_viewportSize = viewportSize;
    m_vertices.clear();
    m_sdfInstances.clear();
    m_drawCommands.clear();
    m_clipStack.clear();
    m_nextRenderOrder = 1;
    m_currentCompositeRenderOrder = 0;
    m_compositeRenderItemActive = false;
}

std::uint64_t VulkanRenderer2D::reserveRenderOrder()
{
    return m_nextRenderOrder++;
}

void VulkanRenderer2D::beginCompositeRenderItem(const std::uint64_t renderOrder)
{
    m_currentCompositeRenderOrder = renderOrder;
    m_compositeRenderItemActive = true;
}

void VulkanRenderer2D::endCompositeRenderItem()
{
    m_currentCompositeRenderOrder = 0;
    m_compositeRenderItemActive = false;
}

void VulkanRenderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
{
    if (size.x <= 0.0f || size.y <= 0.0f) {
        return;
    }

    if (m_vertices.size() + VerticesPerQuad > m_maxQuads * VerticesPerQuad) {
        return;
    }

    const glm::vec2 p0 = toNdc(position);
    const glm::vec2 p1 = toNdc({position.x + size.x, position.y});
    const glm::vec2 p2 = toNdc({position.x + size.x, position.y + size.y});
    const glm::vec2 p3 = toNdc({position.x, position.y + size.y});
    const std::uint32_t first = static_cast<std::uint32_t>(m_vertices.size());

    m_vertices.push_back({p0, color, {0.0f, 0.0f}});
    m_vertices.push_back({p1, color, {1.0f, 0.0f}});
    m_vertices.push_back({p2, color, {1.0f, 1.0f}});
    m_vertices.push_back({p2, color, {1.0f, 1.0f}});
    m_vertices.push_back({p3, color, {0.0f, 1.0f}});
    m_vertices.push_back({p0, color, {0.0f, 0.0f}});
    appendDrawCommand(DrawCommandType::Quad, currentClipRect(), first, static_cast<std::uint32_t>(VerticesPerQuad), m_fallbackTexture.descriptorSet);
}

void VulkanRenderer2D::drawGradientQuad(
    const glm::vec2& position,
    const glm::vec2& size,
    const glm::vec4& topLeft,
    const glm::vec4& topRight,
    const glm::vec4& bottomRight,
    const glm::vec4& bottomLeft)
{
    if (size.x <= 0.0f || size.y <= 0.0f){
        return;
    }

    if (m_vertices.size() + VerticesPerQuad > m_maxQuads * VerticesPerQuad) {
        return;
    }

    const glm::vec2 p0 = toNdc(position);
    const glm::vec2 p1 = toNdc({position.x + size.x, position.y});
    const glm::vec2 p2 = toNdc({position.x + size.x, position.y + size.y});
    const glm::vec2 p3 = toNdc({position.x, position.y + size.y});
    const std::uint32_t first = static_cast<std::uint32_t>(m_vertices.size());

    m_vertices.push_back({p0, topLeft, {0.0f, 0.0f}});
    m_vertices.push_back({p1, topRight, {1.0f, 0.0f}});
    m_vertices.push_back({p2, bottomRight, {1.0f, 1.0f}});

    m_vertices.push_back({p2, bottomRight, {1.0f, 1.0f}});
    m_vertices.push_back({p3, bottomLeft, {0.0f, 1.0f}});
    m_vertices.push_back({p0, topLeft, {0.0f, 0.0f}});

    appendDrawCommand(DrawCommandType::Quad, currentClipRect(), first, static_cast<std::uint32_t>(VerticesPerQuad), m_fallbackTexture.descriptorSet);
}

void VulkanRenderer2D::drawImage(
    const UITextureId texture,
    const glm::vec2& position,
    const glm::vec2& size,
    const glm::vec2& uvMinimum,
    const glm::vec2& uvMaximum,
    const glm::vec4& tint)
{
    if (size.x <= 0.0f || size.y <= 0.0f || tint.a <= 0.0f) {
        return;
    }

    if (m_vertices.size() + VerticesPerQuad > m_maxQuads * VerticesPerQuad) {
        return;
    }

    const glm::vec2 p0 = toNdc(position);
    const glm::vec2 p1 = toNdc({position.x + size.x, position.y});
    const glm::vec2 p2 = toNdc({position.x + size.x, position.y + size.y});
    const glm::vec2 p3 = toNdc({position.x, position.y + size.y});
    const std::uint32_t first = static_cast<std::uint32_t>(m_vertices.size());

    m_vertices.push_back({p0, tint, {uvMinimum.x, uvMinimum.y}});
    m_vertices.push_back({p1, tint, {uvMaximum.x, uvMinimum.y}});
    m_vertices.push_back({p2, tint, {uvMaximum.x, uvMaximum.y}});
    m_vertices.push_back({p2, tint, {uvMaximum.x, uvMaximum.y}});
    m_vertices.push_back({p3, tint, {uvMinimum.x, uvMaximum.y}});
    m_vertices.push_back({p0, tint, {uvMinimum.x, uvMinimum.y}});
    appendDrawCommand(DrawCommandType::Quad, currentClipRect(), first, static_cast<std::uint32_t>(VerticesPerQuad), descriptorSetForTexture(texture));
}

void VulkanRenderer2D::uploadImage(
    const UITextureId texture,
    const std::uint32_t width,
    const std::uint32_t height,
    const std::uint8_t* rgba8,
    const std::size_t byteCount)
{
    if (texture == 0 || width == 0U || height == 0U || rgba8 == nullptr || byteCount < static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U) {
        return;
    }

    if (m_textures.find(texture) != m_textures.end()) {
        return;
    }

    m_textures.emplace(texture, createTextureResource(width, height, rgba8, byteCount));
}

void VulkanRenderer2D::drawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, const float thickness)
{
    if (size.x <= 0.0f || size.y <= 0.0f) {
        return;
    }

    const float line = std::max(thickness, 1.0f);
    drawQuad(position, {size.x, line}, color);
    drawQuad({position.x, position.y + size.y - line}, {size.x, line}, color);
    drawQuad(position, {line, size.y}, color);
    drawQuad({position.x + size.x - line, position.y}, {line, size.y}, color);
}

// Adds one SDF rounded rectangle to the styled UI batch.
void VulkanRenderer2D::drawSdfRect(
    const glm::vec2& position,
    const glm::vec2& size,
    const float radius,
    const glm::vec4& fillColor,
    const glm::vec4& borderColor,
    const float borderWidth)
{
    if (size.x <= 0.0f || size.y <= 0.0f) {
        return;
    }

    if (m_sdfInstances.size() >= m_maxQuads) {
        return;
    }

    const std::uint32_t first = static_cast<std::uint32_t>(m_sdfInstances.size());
    m_sdfInstances.push_back({
        toNdc(position),
        {
            (size.x / static_cast<float>(std::max(m_viewportSize.x, 1U))) * 2.0f,
            (size.y / static_cast<float>(std::max(m_viewportSize.y, 1U))) * 2.0f,
        },
        size,
        fillColor,
        borderColor,
        radius,
        borderWidth,
    });
    appendDrawCommand(DrawCommandType::SdfRect, currentClipRect(), first, 1);
}

void VulkanRenderer2D::pushClipRect(const UIClipRect& clipRect)
{
    m_clipStack.push_back(m_clipStack.empty() ? clipRect : intersectClipRects(m_clipStack.back(), clipRect));
}

void VulkanRenderer2D::popClipRect()
{
    if (!m_clipStack.empty()) {
        m_clipStack.pop_back();
    }
}

void VulkanRenderer2D::end()
{
    uploadVertices();
    uploadSdfInstances();
}

void VulkanRenderer2D::record(const VkCommandBuffer commandBuffer)
{
    std::vector<std::uint64_t> renderOrders;
    appendRenderOrders(renderOrders);
    std::sort(renderOrders.begin(), renderOrders.end());
    renderOrders.erase(std::unique(renderOrders.begin(), renderOrders.end()), renderOrders.end());

    for (const std::uint64_t renderOrder : renderOrders) {
        record(commandBuffer, renderOrder);
    }
}

void VulkanRenderer2D::record(const VkCommandBuffer commandBuffer, const std::uint64_t renderOrder) const
{
    if (m_drawCommands.empty()) {
        return;
    }

    const VkDeviceSize offsets[] = {0};
    const VkBuffer sdfBuffers[] = {m_sdfVertexBuffer, m_sdfInstanceBuffer};
    const VkDeviceSize sdfOffsets[] = {0, 0};
    setViewportAndScissor(commandBuffer);

    std::optional<DrawCommandType> boundType;
    for (const DrawCommand& command : m_drawCommands) {
        if (command.renderOrder != renderOrder || command.count == 0) {
            continue;
        }

        if (!boundType || *boundType != command.type) {
            if (command.type == DrawCommandType::Quad) {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffer, offsets);
            } else {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_sdfPipeline);
                vkCmdBindVertexBuffers(commandBuffer, 0, 2, sdfBuffers, sdfOffsets);
            }
            boundType = command.type;
        }

        setScissor(commandBuffer, command.clipRect);
        if (command.type == DrawCommandType::Quad) {
            const VkDescriptorSet descriptorSet = command.descriptorSet != VK_NULL_HANDLE ? command.descriptorSet : m_fallbackTexture.descriptorSet;
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_pipelineLayout,
                0,
                1,
                &descriptorSet,
                0,
                nullptr);
            vkCmdDraw(commandBuffer, command.count, 1, command.first, 0);
        } else {
            vkCmdDraw(commandBuffer, static_cast<std::uint32_t>(VerticesPerQuad), command.count, 0, command.first);
        }
    }
}

void VulkanRenderer2D::appendRenderOrders(std::vector<std::uint64_t>& renderOrders) const
{
    for (const DrawCommand& command : m_drawCommands) {
        if (command.count > 0) {
            renderOrders.push_back(command.renderOrder);
        }
    }
}

std::size_t VulkanRenderer2D::quadCount() const
{
    return m_vertices.size() / VerticesPerQuad;
}

std::size_t VulkanRenderer2D::maxQuads() const
{
    return m_maxQuads;
}

void VulkanRenderer2D::appendDrawCommand(
    const DrawCommandType type,
    const UIClipRect clipRect,
    const std::uint32_t first,
    const std::uint32_t count,
    const VkDescriptorSet descriptorSet)
{
    const std::uint64_t renderOrder = currentRenderOrder();
    if (!m_drawCommands.empty()) {
        DrawCommand& previous = m_drawCommands.back();
        if (previous.type == type &&
            previous.renderOrder == renderOrder &&
            sameClipRect(previous.clipRect, clipRect) &&
            previous.first + previous.count == first &&
            previous.descriptorSet == descriptorSet) {
            previous.count += count;
            return;
        }
    }

    m_drawCommands.push_back({type, renderOrder, clipRect, first, count, descriptorSet});
}

std::uint64_t VulkanRenderer2D::currentRenderOrder()
{
    return m_compositeRenderItemActive ? m_currentCompositeRenderOrder : UncompositedShapeRenderOrder;
}

VkDescriptorSet VulkanRenderer2D::descriptorSetForTexture(const UITextureId texture) const
{
    const auto found = m_textures.find(texture);
    if (found != m_textures.end() && found->second.descriptorSet != VK_NULL_HANDLE) {
        return found->second.descriptorSet;
    }

    return m_fallbackTexture.descriptorSet;
}

void VulkanRenderer2D::createPipeline(const VkRenderPass renderPass)
{
    const auto vertexShaderCode = readFile(NIKREON_SHADER_DIR "/renderer2d_quad.vert.spv");
    const auto fragmentShaderCode = readFile(NIKREON_SHADER_DIR "/renderer2d_quad.frag.spv");

    const VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    const VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

    VkPipelineShaderStageCreateInfo vertexStage{};
    vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStage.module = vertexShaderModule;
    vertexStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentStage{};
    fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStage.module = fragmentShaderModule;
    fragmentStage.pName = "main";

    const VkPipelineShaderStageCreateInfo shaderStages[] = {vertexStage, fragmentStage};
    const auto bindingDescription = vertexBindingDescription();
    const auto attributeDescriptions = vertexAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDescription;
    vertexInput.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributeDescriptions.size());
    vertexInput.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    const VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    checkVk(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout), "Failed to create Renderer2D pipeline layout.");

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    checkVk(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline), "Failed to create Renderer2D pipeline.");

    vkDestroyShaderModule(m_device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertexShaderModule, nullptr);
}

// Creates the graphics pipeline used for SDF styled UI rectangles.
void VulkanRenderer2D::createSdfPipeline(const VkRenderPass renderPass)
{
    const auto vertexShaderCode = readFile(NIKREON_SHADER_DIR "/renderer2d_sdf_rect.vert.spv");
    const auto fragmentShaderCode = readFile(NIKREON_SHADER_DIR "/renderer2d_sdf_rect.frag.spv");

    const VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    const VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

    VkPipelineShaderStageCreateInfo vertexStage{};
    vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStage.module = vertexShaderModule;
    vertexStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentStage{};
    fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStage.module = fragmentShaderModule;
    fragmentStage.pName = "main";

    const VkPipelineShaderStageCreateInfo shaderStages[] = {vertexStage, fragmentStage};
    const auto bindingDescriptions = sdfVertexBindingDescriptions();
    const auto attributeDescriptions = sdfVertexAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = static_cast<std::uint32_t>(bindingDescriptions.size());
    vertexInput.pVertexBindingDescriptions = bindingDescriptions.data();
    vertexInput.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributeDescriptions.size());
    vertexInput.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    const VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    checkVk(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_sdfPipelineLayout), "Failed to create Renderer2D SDF pipeline layout.");

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_sdfPipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    checkVk(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_sdfPipeline), "Failed to create Renderer2D SDF pipeline.");

    vkDestroyShaderModule(m_device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertexShaderModule, nullptr);
}

void VulkanRenderer2D::createDescriptorResources()
{
    VkDescriptorSetLayoutBinding textureBinding{};
    textureBinding.binding = 0;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = 1;
    textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &textureBinding;
    checkVk(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout), "Failed to create Renderer2D descriptor set layout.");

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 512U;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 512U;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    checkVk(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool), "Failed to create Renderer2D descriptor pool.");
}

void VulkanRenderer2D::createFallbackTexture()
{
    const std::array<std::uint8_t, 4> whitePixel = {255U, 255U, 255U, 255U};
    m_fallbackTexture = createTextureResource(1U, 1U, whitePixel.data(), whitePixel.size());
}

void VulkanRenderer2D::createVertexBuffer()
{
    m_vertexBufferSize = static_cast<VkDeviceSize>(m_maxQuads * VerticesPerQuad * sizeof(Vertex));

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_vertexBufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    checkVk(vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_vertexBuffer), "Failed to create Renderer2D vertex buffer.");

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(m_device, m_vertexBuffer, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(
        memoryRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    checkVk(vkAllocateMemory(m_device, &allocateInfo, nullptr, &m_vertexBufferMemory), "Failed to allocate Renderer2D vertex buffer memory.");
    checkVk(vkBindBufferMemory(m_device, m_vertexBuffer, m_vertexBufferMemory, 0), "Failed to bind Renderer2D vertex buffer memory.");
}

// Creates a host-visible buffer for simple dynamic 2D data.
void VulkanRenderer2D::createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& memory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    checkVk(vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer), "Failed to create Renderer2D buffer.");

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(m_device, buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(
        memoryRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    checkVk(vkAllocateMemory(m_device, &allocateInfo, nullptr, &memory), "Failed to allocate Renderer2D buffer memory.");
    checkVk(vkBindBufferMemory(m_device, buffer, memory, 0), "Failed to bind Renderer2D buffer memory.");
}

void VulkanRenderer2D::createDeviceBuffer(
    const VkDeviceSize size,
    const VkBufferUsageFlags usage,
    const VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    checkVk(vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer), "Failed to create Renderer2D device buffer.");

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(m_device, buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

    checkVk(vkAllocateMemory(m_device, &allocateInfo, nullptr, &memory), "Failed to allocate Renderer2D device buffer memory.");
    checkVk(vkBindBufferMemory(m_device, buffer, memory, 0), "Failed to bind Renderer2D device buffer memory.");
}

VulkanRenderer2D::TextureResource VulkanRenderer2D::createTextureResource(
    const std::uint32_t width,
    const std::uint32_t height,
    const std::uint8_t* rgba8,
    const std::size_t byteCount)
{
    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U);
    if (rgba8 == nullptr || byteCount < static_cast<std::size_t>(imageSize)) {
        return {};
    }

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    createDeviceBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

    void* mapped = nullptr;
    checkVk(vkMapMemory(m_device, stagingMemory, 0, imageSize, 0, &mapped), "Failed to map Renderer2D image staging buffer.");
    std::memcpy(mapped, rgba8, static_cast<std::size_t>(imageSize));
    vkUnmapMemory(m_device, stagingMemory);

    TextureResource texture;
    texture.width = width;
    texture.height = height;
    createImage(width, height, texture.image, texture.memory);
    transitionImageLayout(texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, texture.image, width, height);
    transitionImageLayout(texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    checkVk(vkCreateImageView(m_device, &viewInfo, nullptr, &texture.imageView), "Failed to create Renderer2D image view.");

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = 0.0f;
    checkVk(vkCreateSampler(m_device, &samplerInfo, nullptr, &texture.sampler), "Failed to create Renderer2D image sampler.");

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = m_descriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &m_descriptorSetLayout;
    checkVk(vkAllocateDescriptorSets(m_device, &allocateInfo, &texture.descriptorSet), "Failed to allocate Renderer2D image descriptor set.");

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = texture.sampler;
    imageInfo.imageView = texture.imageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = texture.descriptorSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

    return texture;
}

void VulkanRenderer2D::createImage(const std::uint32_t width, const std::uint32_t height, VkImage& image, VkDeviceMemory& memory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    checkVk(vkCreateImage(m_device, &imageInfo, nullptr, &image), "Failed to create Renderer2D image.");

    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(m_device, image, &requirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = requirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    checkVk(vkAllocateMemory(m_device, &allocateInfo, nullptr, &memory), "Failed to allocate Renderer2D image memory.");
    checkVk(vkBindImageMemory(m_device, image, memory, 0), "Failed to bind Renderer2D image memory.");
}

void VulkanRenderer2D::transitionImageLayout(const VkImage image, const VkImageLayout oldLayout, const VkImageLayout newLayout)
{
    const VkCommandBuffer commandBuffer = beginSingleUseCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::runtime_error("Unsupported Renderer2D image layout transition.");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    endSingleUseCommands(commandBuffer);
}

void VulkanRenderer2D::copyBufferToImage(const VkBuffer buffer, const VkImage image, const std::uint32_t width, const std::uint32_t height)
{
    const VkCommandBuffer commandBuffer = beginSingleUseCommands();

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleUseCommands(commandBuffer);
}

VkCommandBuffer VulkanRenderer2D::beginSingleUseCommands() const
{
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = m_commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    checkVk(vkAllocateCommandBuffers(m_device, &allocateInfo, &commandBuffer), "Failed to allocate Renderer2D upload command buffer.");

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    checkVk(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin Renderer2D upload command buffer.");

    return commandBuffer;
}

void VulkanRenderer2D::endSingleUseCommands(const VkCommandBuffer commandBuffer) const
{
    checkVk(vkEndCommandBuffer(commandBuffer), "Failed to end Renderer2D upload command buffer.");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    checkVk(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE), "Failed to submit Renderer2D upload command buffer.");
    checkVk(vkQueueWaitIdle(m_graphicsQueue), "Failed to wait for Renderer2D upload command buffer.");

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

// Creates the static quad buffer and dynamic instance buffer used by SDF UI rectangles.
void VulkanRenderer2D::createSdfBuffers()
{
    const std::array<SdfVertex, VerticesPerQuad> quadVertices = {
        SdfVertex{{0.0f, 0.0f}, {0.0f, 0.0f}},
        SdfVertex{{1.0f, 0.0f}, {1.0f, 0.0f}},
        SdfVertex{{1.0f, 1.0f}, {1.0f, 1.0f}},
        SdfVertex{{1.0f, 1.0f}, {1.0f, 1.0f}},
        SdfVertex{{0.0f, 1.0f}, {0.0f, 1.0f}},
        SdfVertex{{0.0f, 0.0f}, {0.0f, 0.0f}},
    };

    m_sdfVertexBufferSize = static_cast<VkDeviceSize>(quadVertices.size() * sizeof(SdfVertex));
    m_sdfInstanceBufferSize = static_cast<VkDeviceSize>(m_maxQuads * sizeof(SdfInstance));

    createBuffer(m_sdfVertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_sdfVertexBuffer, m_sdfVertexBufferMemory);
    createBuffer(m_sdfInstanceBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_sdfInstanceBuffer, m_sdfInstanceBufferMemory);

    void* mappedMemory = nullptr;
    checkVk(vkMapMemory(m_device, m_sdfVertexBufferMemory, 0, m_sdfVertexBufferSize, 0, &mappedMemory), "Failed to map Renderer2D SDF quad vertex buffer memory.");
    std::memcpy(mappedMemory, quadVertices.data(), static_cast<std::size_t>(m_sdfVertexBufferSize));
    vkUnmapMemory(m_device, m_sdfVertexBufferMemory);
}

void VulkanRenderer2D::destroy()
{
    destroyTextureResources();

    if (m_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
    }

    if (m_vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
        m_vertexBufferMemory = VK_NULL_HANDLE;
    }

    if (m_sdfVertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_sdfVertexBuffer, nullptr);
        m_sdfVertexBuffer = VK_NULL_HANDLE;
    }

    if (m_sdfVertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_sdfVertexBufferMemory, nullptr);
        m_sdfVertexBufferMemory = VK_NULL_HANDLE;
    }

    if (m_sdfInstanceBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_sdfInstanceBuffer, nullptr);
        m_sdfInstanceBuffer = VK_NULL_HANDLE;
    }

    if (m_sdfInstanceBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_sdfInstanceBufferMemory, nullptr);
        m_sdfInstanceBufferMemory = VK_NULL_HANDLE;
    }

    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_sdfPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_sdfPipeline, nullptr);
        m_sdfPipeline = VK_NULL_HANDLE;
    }

    if (m_sdfPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_sdfPipelineLayout, nullptr);
        m_sdfPipelineLayout = VK_NULL_HANDLE;
    }

    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }
}

void VulkanRenderer2D::destroyTextureResources()
{
    for (auto& [_, texture] : m_textures) {
        destroyTextureResource(texture);
    }
    m_textures.clear();
    destroyTextureResource(m_fallbackTexture);
}

void VulkanRenderer2D::destroyTextureResource(TextureResource& texture)
{
    if (texture.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, texture.sampler, nullptr);
        texture.sampler = VK_NULL_HANDLE;
    }
    if (texture.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, texture.imageView, nullptr);
        texture.imageView = VK_NULL_HANDLE;
    }
    if (texture.image != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, texture.image, nullptr);
        texture.image = VK_NULL_HANDLE;
    }
    if (texture.memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, texture.memory, nullptr);
        texture.memory = VK_NULL_HANDLE;
    }
    texture.descriptorSet = VK_NULL_HANDLE;
}

void VulkanRenderer2D::uploadVertices()
{
    if (m_vertices.empty()) {
        return;
    }

    const VkDeviceSize uploadSize = static_cast<VkDeviceSize>(m_vertices.size() * sizeof(Vertex));
    if (uploadSize > m_vertexBufferSize) {
        throw std::runtime_error("Renderer2D vertex upload exceeded vertex buffer capacity.");
    }

    void* mappedMemory = nullptr;
    checkVk(vkMapMemory(m_device, m_vertexBufferMemory, 0, uploadSize, 0, &mappedMemory), "Failed to map Renderer2D vertex buffer memory.");
    std::memcpy(mappedMemory, m_vertices.data(), static_cast<std::size_t>(uploadSize));
    vkUnmapMemory(m_device, m_vertexBufferMemory);
}

// Uploads one compact instance record per SDF rectangle.
void VulkanRenderer2D::uploadSdfInstances()
{
    if (m_sdfInstances.empty()) {
        return;
    }

    const VkDeviceSize uploadSize = static_cast<VkDeviceSize>(m_sdfInstances.size() * sizeof(SdfInstance));
    if (uploadSize > m_sdfInstanceBufferSize) {
        throw std::runtime_error("Renderer2D SDF instance upload exceeded instance buffer capacity.");
    }

    void* mappedMemory = nullptr;
    checkVk(vkMapMemory(m_device, m_sdfInstanceBufferMemory, 0, uploadSize, 0, &mappedMemory), "Failed to map Renderer2D SDF instance buffer memory.");
    std::memcpy(mappedMemory, m_sdfInstances.data(), static_cast<std::size_t>(uploadSize));
    vkUnmapMemory(m_device, m_sdfInstanceBufferMemory);
}

// Applies viewport and scissor state shared by the 2D pipelines.
void VulkanRenderer2D::setViewportAndScissor(const VkCommandBuffer commandBuffer) const
{
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_viewportSize.x);
    viewport.height = static_cast<float>(m_viewportSize.y);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {m_viewportSize.x, m_viewportSize.y};

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void VulkanRenderer2D::setScissor(const VkCommandBuffer commandBuffer, const UIClipRect& clipRect) const
{
    const float maxWidth = static_cast<float>(m_viewportSize.x);
    const float maxHeight = static_cast<float>(m_viewportSize.y);
    const float x = std::clamp(clipRect.position.x, 0.0f, maxWidth);
    const float y = std::clamp(clipRect.position.y, 0.0f, maxHeight);
    const float right = std::clamp(clipRect.position.x + clipRect.size.x, x, maxWidth);
    const float bottom = std::clamp(clipRect.position.y + clipRect.size.y, y, maxHeight);

    VkRect2D scissor{};
    scissor.offset = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)};
    scissor.extent = {
        static_cast<std::uint32_t>(right - x),
        static_cast<std::uint32_t>(bottom - y),
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

UIClipRect VulkanRenderer2D::currentClipRect() const
{
    return m_clipStack.empty()
        ? UIClipRect{{0.0f, 0.0f}, {static_cast<float>(m_viewportSize.x), static_cast<float>(m_viewportSize.y)}}
        : m_clipStack.back();
}

glm::vec2 VulkanRenderer2D::toNdc(const glm::vec2& pixelPosition) const
{
    const float width = static_cast<float>(std::max(m_viewportSize.x, 1U));
    const float height = static_cast<float>(std::max(m_viewportSize.y, 1U));

    // Vulkan's viewport transform makes this mapping line up with top-left UI coordinates.
    return {
        (pixelPosition.x / width) * 2.0f - 1.0f,
        (pixelPosition.y / height) * 2.0f - 1.0f,
    };
}

VkShaderModule VulkanRenderer2D::createShaderModule(const std::vector<char>& bytecode) const
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytecode.size();
    createInfo.pCode = reinterpret_cast<const std::uint32_t*>(bytecode.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    checkVk(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule), "Failed to create shader module.");
    return shaderModule;
}

std::uint32_t VulkanRenderer2D::findMemoryType(const std::uint32_t typeFilter, const VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);

    for (std::uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
        if ((typeFilter & (1U << index)) != 0 &&
            (memoryProperties.memoryTypes[index].propertyFlags & properties) == properties) {
            return index;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type.");
}

std::vector<char> VulkanRenderer2D::readFile(const char* path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Failed to open shader file: ") + path);
    }

    const auto fileSize = file.tellg();
    std::vector<char> buffer(static_cast<std::size_t>(fileSize));
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}

VkVertexInputBindingDescription VulkanRenderer2D::vertexBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> VulkanRenderer2D::vertexAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, uv);

    return attributeDescriptions;
}

std::array<VkVertexInputBindingDescription, 2> VulkanRenderer2D::sdfVertexBindingDescriptions()
{
    std::array<VkVertexInputBindingDescription, 2> bindingDescriptions{};

    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(SdfVertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    bindingDescriptions[1].binding = 1;
    bindingDescriptions[1].stride = sizeof(SdfInstance);
    bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    return bindingDescriptions;
}

std::array<VkVertexInputAttributeDescription, 9> VulkanRenderer2D::sdfVertexAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 9> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(SdfVertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(SdfVertex, localPosition);

    attributeDescriptions[2].binding = 1;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(SdfInstance, position);

    attributeDescriptions[3].binding = 1;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(SdfInstance, ndcSize);

    attributeDescriptions[4].binding = 1;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(SdfInstance, rectSize);

    attributeDescriptions[5].binding = 1;
    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[5].offset = offsetof(SdfInstance, fillColor);

    attributeDescriptions[6].binding = 1;
    attributeDescriptions[6].location = 6;
    attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[6].offset = offsetof(SdfInstance, borderColor);

    attributeDescriptions[7].binding = 1;
    attributeDescriptions[7].location = 7;
    attributeDescriptions[7].format = VK_FORMAT_R32_SFLOAT;
    attributeDescriptions[7].offset = offsetof(SdfInstance, radius);

    attributeDescriptions[8].binding = 1;
    attributeDescriptions[8].location = 8;
    attributeDescriptions[8].format = VK_FORMAT_R32_SFLOAT;
    attributeDescriptions[8].offset = offsetof(SdfInstance, borderWidth);

    return attributeDescriptions;
}

} // namespace Engine
