#include "Engine/Renderer/Vulkan/VulkanTextRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

#include <glm/common.hpp>
#include <utility>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace Engine {

namespace {

constexpr std::size_t VerticesPerGlyph = 6;

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
constexpr std::uint32_t AtlasWidth = 512;
constexpr std::uint32_t GlyphPadding = 1;

#ifndef NIKREON_SHADER_DIR
#define NIKREON_SHADER_DIR "shaders"
#endif

void checkVk(const VkResult result, const char* message)
{
    if (result != VK_SUCCESS) {
        throw std::runtime_error(message);
    }
}

std::uint32_t nextPowerOfTwo(std::uint32_t value)
{
    value = std::max(value, 1U);
    --value;
    value |= value >> 1U;
    value |= value >> 2U;
    value |= value >> 4U;
    value |= value >> 8U;
    value |= value >> 16U;
    return value + 1U;
}

std::vector<char32_t> decodeUtf8(const std::string_view text)
{
    std::vector<char32_t> codepoints;
    for (std::size_t index = 0; index < text.size();) {
        const auto first = static_cast<unsigned char>(text[index]);
        char32_t codepoint = 0;
        std::size_t length = 1;
        if ((first & 0x80U) == 0) {
            codepoint = first;
        } else if ((first & 0xE0U) == 0xC0U && index + 1 < text.size()) {
            codepoint = first & 0x1FU;
            length = 2;
        } else if ((first & 0xF0U) == 0xE0U && index + 2 < text.size()) {
            codepoint = first & 0x0FU;
            length = 3;
        } else if ((first & 0xF8U) == 0xF0U && index + 3 < text.size()) {
            codepoint = first & 0x07U;
            length = 4;
        } else {
            ++index;
            continue;
        }

        bool valid = true;
        for (std::size_t offset = 1; offset < length; ++offset) {
            const auto continuation = static_cast<unsigned char>(text[index + offset]);
            if ((continuation & 0xC0U) != 0x80U) {
                valid = false;
                break;
            }
            codepoint = (codepoint << 6U) | (continuation & 0x3FU);
        }
        if (valid) {
            codepoints.push_back(codepoint);
        }
        index += valid ? length : 1;
    }
    return codepoints;
}

std::vector<char32_t> defaultGlyphSet()
{
    std::vector<char32_t> codepoints;
    const auto appendRange = [&codepoints](const char32_t first, const char32_t last) {
        for (char32_t codepoint = first; codepoint <= last; ++codepoint) {
            codepoints.push_back(codepoint);
        }
    };
    appendRange(32, 255);
    appendRange(0x0400, 0x04FF);
    return codepoints;
}

} // namespace

VulkanTextRenderer::VulkanTextRenderer(
    const VkDevice device,
    const VkPhysicalDevice physicalDevice,
    const VkQueue graphicsQueue,
    const VkCommandPool commandPool,
    const VkRenderPass renderPass,
    const std::size_t maxGlyphs)
    : m_device(device)
    , m_physicalDevice(physicalDevice)
    , m_graphicsQueue(graphicsQueue)
    , m_commandPool(commandPool)
    , m_maxGlyphs(maxGlyphs)
{
    if (FT_Init_FreeType(&m_freetype) != 0) {
        throw std::runtime_error("Failed to initialize FreeType.");
    }

    m_vertices.reserve(m_maxGlyphs * VerticesPerGlyph);
    m_batches.reserve(32);
    createDescriptorResources();
    createVertexBuffer();
    createPipeline(renderPass);
}

VulkanTextRenderer::~VulkanTextRenderer()
{
    destroy();
}

void VulkanTextRenderer::begin(const glm::uvec2& viewportSize)
{
    m_viewportSize = viewportSize;
    m_vertices.clear();
    m_batches.clear();
    m_clipStack.clear();
}

bool VulkanTextRenderer::loadFont(const std::string_view name, const std::filesystem::path& path, const float pixelSize)
{
    if (name.empty() || pixelSize <= 0.0f) {
        return false;
    }

    const std::string fontName{name};
    if (m_fonts.contains(fontName)) {
        return true;
    }

    FT_Face face = nullptr;
    if (FT_New_Face(m_freetype, path.string().c_str(), 0, &face) != 0) {
        return false;
    }

    const auto destroyFace = [&face]() {
        if (face != nullptr) {
            FT_Done_Face(face);
            face = nullptr;
        }
    };

    if (FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(std::ceil(pixelSize))) != 0) {
        destroyFace();
        return false;
    }

    struct PackedGlyph {
        char32_t character{0};
        std::uint32_t x{0};
        std::uint32_t y{0};
        std::uint32_t width{0};
        std::uint32_t height{0};
    };

    std::vector<PackedGlyph> packedGlyphs;
    const std::vector<char32_t> codepoints = defaultGlyphSet();
    packedGlyphs.reserve(codepoints.size());

    std::uint32_t cursorX = GlyphPadding;
    std::uint32_t cursorY = GlyphPadding;
    std::uint32_t rowHeight = 0;

    for (const char32_t character : codepoints) {
        if (FT_Load_Char(face, static_cast<FT_ULong>(character), FT_LOAD_RENDER) != 0) {
            continue;
        }

        const std::uint32_t width = face->glyph->bitmap.width;
        const std::uint32_t height = face->glyph->bitmap.rows;
        if (cursorX + width + GlyphPadding > AtlasWidth) {
            cursorX = GlyphPadding;
            cursorY += rowHeight + GlyphPadding;
            rowHeight = 0;
        }

        packedGlyphs.push_back({
            character,
            cursorX,
            cursorY,
            width,
            height,
        });
        cursorX += width + GlyphPadding;
        rowHeight = std::max(rowHeight, height);
    }

    const std::uint32_t atlasHeight = nextPowerOfTwo(cursorY + rowHeight + GlyphPadding);
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(AtlasWidth) * atlasHeight, 0);

    Font font;
    font.ascender = static_cast<float>(face->size->metrics.ascender >> 6);
    font.lineHeight = static_cast<float>(face->size->metrics.height >> 6);

    for (const PackedGlyph& packed : packedGlyphs) {
        if (FT_Load_Char(face, static_cast<FT_ULong>(packed.character), FT_LOAD_RENDER) != 0) {
            continue;
        }

        const FT_Bitmap& bitmap = face->glyph->bitmap;
        for (std::uint32_t row = 0; row < packed.height; ++row) {
            const int sourceRow = bitmap.pitch >= 0
                ? static_cast<int>(row)
                : static_cast<int>(packed.height - row - 1);
            const auto* source = bitmap.buffer + sourceRow * std::abs(bitmap.pitch);
            auto* destination = pixels.data() + static_cast<std::size_t>(packed.y + row) * AtlasWidth + packed.x;
            std::memcpy(destination, source, packed.width);
        }

        font.glyphs.emplace(packed.character, Glyph{
            {static_cast<float>(packed.width), static_cast<float>(packed.height)},
            {static_cast<float>(face->glyph->bitmap_left), static_cast<float>(face->glyph->bitmap_top)},
            {
                static_cast<float>(packed.x) / static_cast<float>(AtlasWidth),
                static_cast<float>(packed.y) / static_cast<float>(atlasHeight),
            },
            {
                static_cast<float>(packed.x + packed.width) / static_cast<float>(AtlasWidth),
                static_cast<float>(packed.y + packed.height) / static_cast<float>(atlasHeight),
            },
            static_cast<float>(face->glyph->advance.x >> 6),
        });
    }

    destroyFace();
    const auto solidPixel = std::max_element(pixels.begin(), pixels.end());
    if (solidPixel != pixels.end()) {
        const std::size_t solidIndex = static_cast<std::size_t>(std::distance(pixels.begin(), solidPixel));
        font.solidUv = {
            (static_cast<float>(solidIndex % AtlasWidth) + 0.5f) / static_cast<float>(AtlasWidth),
            (static_cast<float>(solidIndex / AtlasWidth) + 0.5f) / static_cast<float>(atlasHeight),
        };
    }
    createFontTexture(font, pixels, AtlasWidth, atlasHeight);
    m_fonts.emplace(fontName, std::move(font));
    return true;
}

void VulkanTextRenderer::drawText(
    const std::string_view text,
    const glm::vec2& position,
    const glm::vec4& color,
    const std::string_view fontName,
    const float scale,
    const TextAlignment alignment,
    const TextLayout& layout)
{
    const Font* font = findFont(fontName);
    if (font == nullptr || scale <= 0.0f || text.empty()) {
        return;
    }

    glm::vec2 pen = position;
    const float measuredWidth = measureText(text, fontName, scale, layout).x;
    if (alignment == TextAlignment::Center) {
        pen.x -= measuredWidth * 0.5f;
    } else if (alignment == TextAlignment::Right) {
        pen.x -= measuredWidth;
    }

    const UIClipRect clipRect = currentClipRect();
    if (m_batches.empty() || m_batches.back().font != font || !sameClipRect(m_batches.back().clipRect, clipRect)) {
        m_batches.push_back({
            font,
            clipRect,
            static_cast<std::uint32_t>(m_vertices.size()),
            0,
        });
    }

    Batch& batch = m_batches.back();
    const float lineAdvance = font->lineHeight * scale * std::max(layout.lineSpacing, 0.0f);
    const float lineStartX = pen.x;
    for (const char32_t character : decodeUtf8(text)) {
        if (character == '\n') {
            pen.x = lineStartX;
            pen.y += lineAdvance;
            continue;
        }

        const auto found = font->glyphs.find(character);
        if (found == font->glyphs.end()) {
            continue;
        }

        const Glyph& glyph = found->second;
        if (layout.wordWrap && layout.maxWidth > 0.0f && pen.x > lineStartX && pen.x + glyph.advance * scale > lineStartX + layout.maxWidth) {
            pen.x = lineStartX;
            pen.y += lineAdvance;
        }
        if (m_vertices.size() + VerticesPerGlyph > m_maxGlyphs * VerticesPerGlyph) {
            return;
        }

        const glm::vec2 glyphPosition{
            pen.x + glyph.bearing.x * scale,
            pen.y + (font->ascender - glyph.bearing.y) * scale,
        };
        const glm::vec2 glyphSize = glyph.size * scale;

        if (glyphSize.x > 0.0f && glyphSize.y > 0.0f) {
            const glm::vec2 p0 = toNdc(glyphPosition);
            const glm::vec2 p1 = toNdc({glyphPosition.x + glyphSize.x, glyphPosition.y});
            const glm::vec2 p2 = toNdc(glyphPosition + glyphSize);
            const glm::vec2 p3 = toNdc({glyphPosition.x, glyphPosition.y + glyphSize.y});

            m_vertices.push_back({p0, glyph.uvMinimum, color});
            m_vertices.push_back({p1, {glyph.uvMaximum.x, glyph.uvMinimum.y}, color});
            m_vertices.push_back({p2, glyph.uvMaximum, color});
            m_vertices.push_back({p2, glyph.uvMaximum, color});
            m_vertices.push_back({p3, {glyph.uvMinimum.x, glyph.uvMaximum.y}, color});
            m_vertices.push_back({p0, glyph.uvMinimum, color});
            batch.vertexCount += static_cast<std::uint32_t>(VerticesPerGlyph);
        }

        pen.x += glyph.advance * scale;
    }
}

void VulkanTextRenderer::drawSolidRect(
    const glm::vec2& position,
    const glm::vec2& size,
    const glm::vec4& color,
    const std::string_view fontName)
{
    const Font* font = findFont(fontName);
    if (font == nullptr || size.x <= 0.0f || size.y <= 0.0f ||
        m_vertices.size() + VerticesPerGlyph > m_maxGlyphs * VerticesPerGlyph) {
        return;
    }

    const UIClipRect clipRect = currentClipRect();
    if (m_batches.empty() || m_batches.back().font != font || !sameClipRect(m_batches.back().clipRect, clipRect)) {
        m_batches.push_back({
            font,
            clipRect,
            static_cast<std::uint32_t>(m_vertices.size()),
            0,
        });
    }

    const glm::vec2 p0 = toNdc(position);
    const glm::vec2 p1 = toNdc({position.x + size.x, position.y});
    const glm::vec2 p2 = toNdc(position + size);
    const glm::vec2 p3 = toNdc({position.x, position.y + size.y});
    m_vertices.push_back({p0, font->solidUv, color});
    m_vertices.push_back({p1, font->solidUv, color});
    m_vertices.push_back({p2, font->solidUv, color});
    m_vertices.push_back({p2, font->solidUv, color});
    m_vertices.push_back({p3, font->solidUv, color});
    m_vertices.push_back({p0, font->solidUv, color});
    m_batches.back().vertexCount += static_cast<std::uint32_t>(VerticesPerGlyph);
}

void VulkanTextRenderer::pushClipRect(const UIClipRect& clipRect)
{
    m_clipStack.push_back(m_clipStack.empty() ? clipRect : intersectClipRects(m_clipStack.back(), clipRect));
}

void VulkanTextRenderer::popClipRect()
{
    if (!m_clipStack.empty()) {
        m_clipStack.pop_back();
    }
}

glm::vec2 VulkanTextRenderer::measureText(
    const std::string_view text,
    const std::string_view fontName,
    const float scale,
    const TextLayout& layout) const
{
    const Font* font = findFont(fontName);
    if (font == nullptr || scale <= 0.0f) {
        return {0.0f, 0.0f};
    }

    float width = 0.0f;
    float maxWidth = 0.0f;
    const float lineAdvance = font->lineHeight * std::max(layout.lineSpacing, 0.0f);
    float height = font->lineHeight;
    for (const char32_t character : decodeUtf8(text)) {
        if (character == '\n') {
            maxWidth = std::max(maxWidth, width);
            width = 0.0f;
            height += lineAdvance;
            continue;
        }

        const auto found = font->glyphs.find(character);
        if (found != font->glyphs.end()) {
            if (layout.wordWrap && layout.maxWidth > 0.0f && width > 0.0f && width + found->second.advance > layout.maxWidth / scale) {
                maxWidth = std::max(maxWidth, width);
                width = 0.0f;
                height += lineAdvance;
            }
            width += found->second.advance;
        }
    }

    return {std::max(maxWidth, width) * scale, height * scale};
}

void VulkanTextRenderer::end()
{
    uploadVertices();
}

void VulkanTextRenderer::record(const VkCommandBuffer commandBuffer) const
{
    if (m_vertices.empty()) {
        return;
    }

    setViewportAndScissor(commandBuffer);
    const VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffer, &offset);

    for (const Batch& batch : m_batches) {
        if (batch.vertexCount == 0) {
            continue;
        }

        setScissor(commandBuffer, batch.clipRect);
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipelineLayout,
            0,
            1,
            &batch.font->descriptorSet,
            0,
            nullptr);
        vkCmdDraw(commandBuffer, batch.vertexCount, 1, batch.firstVertex, 0);
    }
}

void VulkanTextRenderer::createDescriptorResources()
{
    VkDescriptorSetLayoutBinding atlasBinding{};
    atlasBinding.binding = 0;
    atlasBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    atlasBinding.descriptorCount = 1;
    atlasBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &atlasBinding;
    checkVk(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout), "Failed to create TextRenderer descriptor set layout.");

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 16;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 16;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    checkVk(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool), "Failed to create TextRenderer descriptor pool.");
}

void VulkanTextRenderer::createPipeline(const VkRenderPass renderPass)
{
    const auto vertexShaderCode = readFile(NIKREON_SHADER_DIR "/text.vert.spv");
    const auto fragmentShaderCode = readFile(NIKREON_SHADER_DIR "/text.frag.spv");
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

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    blendAttachment.blendEnable = VK_TRUE;
    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &blendAttachment;

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
    checkVk(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout), "Failed to create TextRenderer pipeline layout.");

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

    checkVk(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline), "Failed to create TextRenderer pipeline.");
    vkDestroyShaderModule(m_device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertexShaderModule, nullptr);
}

void VulkanTextRenderer::createVertexBuffer()
{
    m_vertexBufferSize = static_cast<VkDeviceSize>(m_maxGlyphs * VerticesPerGlyph * sizeof(Vertex));
    createBuffer(
        m_vertexBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_vertexBuffer,
        m_vertexBufferMemory);
}

void VulkanTextRenderer::destroy()
{
    for (auto& [name, font] : m_fonts) {
        (void)name;
        destroyFont(font);
    }
    m_fonts.clear();

    if (m_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    }
    if (m_vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
    }
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    }
    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    }
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
    }
    if (m_freetype != nullptr) {
        FT_Done_FreeType(m_freetype);
        m_freetype = nullptr;
    }
}

void VulkanTextRenderer::destroyFont(Font& font)
{
    if (font.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, font.sampler, nullptr);
    }
    if (font.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, font.imageView, nullptr);
    }
    if (font.image != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, font.image, nullptr);
    }
    if (font.imageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, font.imageMemory, nullptr);
    }
}

void VulkanTextRenderer::uploadVertices()
{
    if (m_vertices.empty()) {
        return;
    }

    const VkDeviceSize uploadSize = static_cast<VkDeviceSize>(m_vertices.size() * sizeof(Vertex));
    if (uploadSize > m_vertexBufferSize) {
        throw std::runtime_error("TextRenderer vertex upload exceeded vertex buffer capacity.");
    }

    void* mappedMemory = nullptr;
    checkVk(vkMapMemory(m_device, m_vertexBufferMemory, 0, uploadSize, 0, &mappedMemory), "Failed to map TextRenderer vertex buffer memory.");
    std::memcpy(mappedMemory, m_vertices.data(), static_cast<std::size_t>(uploadSize));
    vkUnmapMemory(m_device, m_vertexBufferMemory);
}

void VulkanTextRenderer::createFontTexture(Font& font, const std::vector<std::uint8_t>& pixels, const std::uint32_t width, const std::uint32_t height)
{
    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(pixels.size());
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    createBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

    void* mappedMemory = nullptr;
    checkVk(vkMapMemory(m_device, stagingMemory, 0, imageSize, 0, &mappedMemory), "Failed to map TextRenderer staging buffer memory.");
    std::memcpy(mappedMemory, pixels.data(), pixels.size());
    vkUnmapMemory(m_device, stagingMemory);

    createImage(width, height, font.image, font.imageMemory);
    transitionImageLayout(font.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, font.image, width, height);
    transitionImageLayout(font.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = font.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    checkVk(vkCreateImageView(m_device, &viewInfo, nullptr, &font.imageView), "Failed to create TextRenderer atlas image view.");

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = 0.0f;
    checkVk(vkCreateSampler(m_device, &samplerInfo, nullptr, &font.sampler), "Failed to create TextRenderer atlas sampler.");

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = m_descriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &m_descriptorSetLayout;
    checkVk(vkAllocateDescriptorSets(m_device, &allocateInfo, &font.descriptorSet), "Failed to allocate TextRenderer descriptor set.");

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = font.sampler;
    imageInfo.imageView = font.imageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = font.descriptorSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
}

void VulkanTextRenderer::createBuffer(
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
    checkVk(vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer), "Failed to create TextRenderer buffer.");

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(m_device, buffer, &requirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = requirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, properties);
    checkVk(vkAllocateMemory(m_device, &allocateInfo, nullptr, &memory), "Failed to allocate TextRenderer buffer memory.");
    checkVk(vkBindBufferMemory(m_device, buffer, memory, 0), "Failed to bind TextRenderer buffer memory.");
}

void VulkanTextRenderer::createImage(const std::uint32_t width, const std::uint32_t height, VkImage& image, VkDeviceMemory& memory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    checkVk(vkCreateImage(m_device, &imageInfo, nullptr, &image), "Failed to create TextRenderer atlas image.");

    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(m_device, image, &requirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = requirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    checkVk(vkAllocateMemory(m_device, &allocateInfo, nullptr, &memory), "Failed to allocate TextRenderer atlas image memory.");
    checkVk(vkBindImageMemory(m_device, image, memory, 0), "Failed to bind TextRenderer atlas image memory.");
}

void VulkanTextRenderer::transitionImageLayout(const VkImage image, const VkImageLayout oldLayout, const VkImageLayout newLayout)
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

    VkPipelineStageFlags sourceStage = 0;
    VkPipelineStageFlags destinationStage = 0;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::runtime_error("Unsupported TextRenderer image layout transition.");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    endSingleUseCommands(commandBuffer);
}

void VulkanTextRenderer::copyBufferToImage(const VkBuffer buffer, const VkImage image, const std::uint32_t width, const std::uint32_t height)
{
    const VkCommandBuffer commandBuffer = beginSingleUseCommands();

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleUseCommands(commandBuffer);
}

VkCommandBuffer VulkanTextRenderer::beginSingleUseCommands() const
{
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = m_commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    checkVk(vkAllocateCommandBuffers(m_device, &allocateInfo, &commandBuffer), "Failed to allocate TextRenderer upload command buffer.");

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    checkVk(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin TextRenderer upload command buffer.");
    return commandBuffer;
}

void VulkanTextRenderer::endSingleUseCommands(const VkCommandBuffer commandBuffer) const
{
    checkVk(vkEndCommandBuffer(commandBuffer), "Failed to end TextRenderer upload command buffer.");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    checkVk(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE), "Failed to submit TextRenderer upload command buffer.");
    checkVk(vkQueueWaitIdle(m_graphicsQueue), "Failed to wait for TextRenderer upload command buffer.");
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void VulkanTextRenderer::setViewportAndScissor(const VkCommandBuffer commandBuffer) const
{
    VkViewport viewport{};
    viewport.width = static_cast<float>(m_viewportSize.x);
    viewport.height = static_cast<float>(m_viewportSize.y);
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.extent = {m_viewportSize.x, m_viewportSize.y};

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void VulkanTextRenderer::setScissor(const VkCommandBuffer commandBuffer, const UIClipRect& clipRect) const
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

UIClipRect VulkanTextRenderer::currentClipRect() const
{
    return m_clipStack.empty()
        ? UIClipRect{{0.0f, 0.0f}, {static_cast<float>(m_viewportSize.x), static_cast<float>(m_viewportSize.y)}}
        : m_clipStack.back();
}

glm::vec2 VulkanTextRenderer::toNdc(const glm::vec2& pixelPosition) const
{
    const float width = static_cast<float>(std::max(m_viewportSize.x, 1U));
    const float height = static_cast<float>(std::max(m_viewportSize.y, 1U));
    return {
        (pixelPosition.x / width) * 2.0f - 1.0f,
        (pixelPosition.y / height) * 2.0f - 1.0f,
    };
}

VkShaderModule VulkanTextRenderer::createShaderModule(const std::vector<char>& bytecode) const
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytecode.size();
    createInfo.pCode = reinterpret_cast<const std::uint32_t*>(bytecode.data());

    VkShaderModule module = VK_NULL_HANDLE;
    checkVk(vkCreateShaderModule(m_device, &createInfo, nullptr, &module), "Failed to create TextRenderer shader module.");
    return module;
}

std::uint32_t VulkanTextRenderer::findMemoryType(const std::uint32_t typeFilter, const VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);

    for (std::uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
        if ((typeFilter & (1U << index)) != 0 &&
            (memoryProperties.memoryTypes[index].propertyFlags & properties) == properties) {
            return index;
        }
    }

    throw std::runtime_error("Failed to find suitable TextRenderer memory type.");
}

const VulkanTextRenderer::Font* VulkanTextRenderer::findFont(const std::string_view name) const
{
    const auto found = m_fonts.find(std::string(name));
    return found == m_fonts.end() ? nullptr : &found->second;
}

std::vector<char> VulkanTextRenderer::readFile(const char* path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Failed to open text shader file: ") + path);
    }

    const auto fileSize = file.tellg();
    std::vector<char> buffer(static_cast<std::size_t>(fileSize));
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}

VkVertexInputBindingDescription VulkanTextRenderer::vertexBindingDescription()
{
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding;
}

std::array<VkVertexInputAttributeDescription, 3> VulkanTextRenderer::vertexAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 3> attributes{};
    attributes[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position)};
    attributes[1] = {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)};
    attributes[2] = {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color)};
    return attributes;
}

} // namespace Engine
