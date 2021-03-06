

#include "stdafx.h"
#include "TextOverlay.h"

///////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   TextOverlay
//
//   Constructor
//

TextOverlay::TextOverlay(
    vks::VulkanDevice* vulkanDevice,
    VkQueue queue,
    std::vector<VkFramebuffer>& framebuffers,
    VkFormat colorformat,
    VkFormat depthformat,
    uint32_t* framebufferwidth,
    uint32_t* framebufferheight,
    std::vector<VkPipelineShaderStageCreateInfo> shaderstages)
{
    this->_vulkanDevice = vulkanDevice;
    this->_queue = queue;
    this->_colorFormat = colorformat;
    this->_depthFormat = depthformat;

    this->_frameBuffers.resize(framebuffers.size());
    for (uint32_t i = 0; i < framebuffers.size(); i++)
    {
        this->_frameBuffers[i] = &framebuffers[i];
    }

    this->_shaderStages = shaderstages;

    this->_frameBufferWidth = framebufferwidth;
    this->_frameBufferHeight = framebufferheight;

    _cmdBuffers.resize(framebuffers.size());
    prepareResources();
    prepareRenderPass();
    preparePipeline();
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   ~TextOverlay
//

TextOverlay:: ~TextOverlay()
{
    // Free up all Vulkan resources requested by the text overlay
    vkDestroySampler(_vulkanDevice->logicalDevice, _sampler, nullptr);
    vkDestroyImage(_vulkanDevice->logicalDevice, _image, nullptr);
    vkDestroyImageView(_vulkanDevice->logicalDevice, _view, nullptr);
    vkDestroyBuffer(_vulkanDevice->logicalDevice, _buffer, nullptr);
    vkFreeMemory(_vulkanDevice->logicalDevice, _memory, nullptr);
    vkFreeMemory(_vulkanDevice->logicalDevice, _imageMemory, nullptr);
    vkDestroyDescriptorSetLayout(_vulkanDevice->logicalDevice, _descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(_vulkanDevice->logicalDevice, _descriptorPool, nullptr);
    vkDestroyPipelineLayout(_vulkanDevice->logicalDevice, _pipelineLayout, nullptr);
    vkDestroyPipelineCache(_vulkanDevice->logicalDevice, _pipelineCache, nullptr);
    vkDestroyPipeline(_vulkanDevice->logicalDevice, _pipeline, nullptr);
    vkDestroyRenderPass(_vulkanDevice->logicalDevice, _renderPass, nullptr);
    vkDestroyCommandPool(_vulkanDevice->logicalDevice, _commandPool, nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   prepareResources
//
//   Prepare all vulkan resources required to render the font
//   The text overlay uses separate resources for descriptors (pool, sets, layouts), pipelines and command buffers
//

void TextOverlay::prepareResources()
{
    const uint32_t fontWidth = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;
    const uint32_t fontHeight = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;

    static unsigned char font24pixels[fontWidth][fontHeight];
    stb_font_consolas_24_latin1(_stbFontData, font24pixels, fontHeight);

    // Command buffer

    // Pool
    VkCommandPoolCreateInfo cmdPoolInfo = {};

    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = _vulkanDevice->queueFamilyIndices.graphics;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(_vulkanDevice->logicalDevice, &cmdPoolInfo, nullptr, &_commandPool));

    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
        vks::initializers::commandBufferAllocateInfo(
            _commandPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            (uint32_t)_cmdBuffers.size());

    VK_CHECK_RESULT(vkAllocateCommandBuffers(_vulkanDevice->logicalDevice, &cmdBufAllocateInfo, _cmdBuffers.data()));

    // Vertex buffer
    VkDeviceSize bufferSize = TEXTOVERLAY_MAX_CHAR_COUNT * sizeof(glm::vec4);

    VkBufferCreateInfo bufferInfo = vks::initializers::bufferCreateInfo(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize);
    VK_CHECK_RESULT(vkCreateBuffer(_vulkanDevice->logicalDevice, &bufferInfo, nullptr, &_buffer));

    VkMemoryRequirements memReqs;
    VkMemoryAllocateInfo allocInfo = vks::initializers::memoryAllocateInfo();

    vkGetBufferMemoryRequirements(_vulkanDevice->logicalDevice, _buffer, &memReqs);
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = _vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VK_CHECK_RESULT(vkAllocateMemory(_vulkanDevice->logicalDevice, &allocInfo, nullptr, &_memory));
    VK_CHECK_RESULT(vkBindBufferMemory(_vulkanDevice->logicalDevice, _buffer, _memory, 0));

    // Font texture
    VkImageCreateInfo imageInfo = vks::initializers::imageCreateInfo();
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8_UNORM;
    imageInfo.extent.width = fontWidth;
    imageInfo.extent.height = fontHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK_RESULT(vkCreateImage(_vulkanDevice->logicalDevice, &imageInfo, nullptr, &_image));

    vkGetImageMemoryRequirements(_vulkanDevice->logicalDevice, _image, &memReqs);
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = _vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK_RESULT(vkAllocateMemory(_vulkanDevice->logicalDevice, &allocInfo, nullptr, &_imageMemory));
    VK_CHECK_RESULT(vkBindImageMemory(_vulkanDevice->logicalDevice, _image, _imageMemory, 0));

    // Staging

    struct {
        VkDeviceMemory memory;
        VkBuffer buffer;
    } stagingBuffer;

    VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo();
    bufferCreateInfo.size = allocInfo.allocationSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK_RESULT(vkCreateBuffer(_vulkanDevice->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer.buffer));

    // Get memory requirements for the staging buffer (alignment, memory type bits)
    vkGetBufferMemoryRequirements(_vulkanDevice->logicalDevice, stagingBuffer.buffer, &memReqs);

    allocInfo.allocationSize = memReqs.size;
    // Get memory type index for a host visible buffer
    allocInfo.memoryTypeIndex = _vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VK_CHECK_RESULT(vkAllocateMemory(_vulkanDevice->logicalDevice, &allocInfo, nullptr, &stagingBuffer.memory));
    VK_CHECK_RESULT(vkBindBufferMemory(_vulkanDevice->logicalDevice, stagingBuffer.buffer, stagingBuffer.memory, 0));

    uint8_t* data;

    VK_CHECK_RESULT(vkMapMemory(_vulkanDevice->logicalDevice, stagingBuffer.memory, 0, allocInfo.allocationSize, 0, (void**)&data));
    // Size of the font texture is WIDTH * HEIGHT * 1 byte (only one channel)
    memcpy(data, &font24pixels[0][0], fontWidth * fontHeight);
    vkUnmapMemory(_vulkanDevice->logicalDevice, stagingBuffer.memory);

    // Copy to image

    VkCommandBuffer copyCmd;
    cmdBufAllocateInfo.commandBufferCount = 1;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(_vulkanDevice->logicalDevice, &cmdBufAllocateInfo, &copyCmd));

    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
    VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));

    // Prepare for transfer
    vks::tools::setImageLayout(
        copyCmd,
        _image,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy bufferCopyRegion = {};
    bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopyRegion.imageSubresource.mipLevel = 0;
    bufferCopyRegion.imageSubresource.layerCount = 1;
    bufferCopyRegion.imageExtent.width = fontWidth;
    bufferCopyRegion.imageExtent.height = fontHeight;
    bufferCopyRegion.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(
        copyCmd,
        stagingBuffer.buffer,
        _image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &bufferCopyRegion
    );

    // Prepare for shader read
    vks::tools::setImageLayout(
        copyCmd,
        _image,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

    VkSubmitInfo submitInfo = vks::initializers::submitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCmd;


    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;

    VkFence fence;
    
    VK_CHECK_RESULT(vkCreateFence(_vulkanDevice->logicalDevice, &fenceCreateInfo, nullptr, &fence));

    VK_CHECK_RESULT(vkQueueSubmit(_queue, 1, &submitInfo, fence));

    // Wait for the fence to signal that command buffer has finished executing
    VK_CHECK_RESULT(vkWaitForFences(_vulkanDevice->logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

    vkDestroyFence(_vulkanDevice->logicalDevice, fence, nullptr);


    vkFreeCommandBuffers(_vulkanDevice->logicalDevice, _commandPool, 1, &copyCmd);
    vkFreeMemory(_vulkanDevice->logicalDevice, stagingBuffer.memory, nullptr);
    vkDestroyBuffer(_vulkanDevice->logicalDevice, stagingBuffer.buffer, nullptr);

    VkImageViewCreateInfo imageViewInfo = vks::initializers::imageViewCreateInfo();
    imageViewInfo.image = _image;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = imageInfo.format;
    imageViewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,	VK_COMPONENT_SWIZZLE_A };
    imageViewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    VK_CHECK_RESULT(vkCreateImageView(_vulkanDevice->logicalDevice, &imageViewInfo, nullptr, &_view));

    // Sampler
    VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo();
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    VK_CHECK_RESULT(vkCreateSampler(_vulkanDevice->logicalDevice, &samplerInfo, nullptr, &_sampler));

    // Descriptor
    // Font uses a separate descriptor pool
    std::array<VkDescriptorPoolSize, 1> poolSizes;
    poolSizes[0] = vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);

    VkDescriptorPoolCreateInfo descriptorPoolInfo =
        vks::initializers::descriptorPoolCreateInfo(
            static_cast<uint32_t>(poolSizes.size()),
            poolSizes.data(),
            1);

    VK_CHECK_RESULT(vkCreateDescriptorPool(_vulkanDevice->logicalDevice, &descriptorPoolInfo, nullptr, &_descriptorPool));

    // Descriptor set layout
    std::array<VkDescriptorSetLayoutBinding, 1> setLayoutBindings;
    setLayoutBindings[0] = vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo =
        vks::initializers::descriptorSetLayoutCreateInfo(
            setLayoutBindings.data(),
            static_cast<uint32_t>(setLayoutBindings.size()));
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(_vulkanDevice->logicalDevice, &descriptorSetLayoutInfo, nullptr, &_descriptorSetLayout));

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo =
        vks::initializers::pipelineLayoutCreateInfo(
            &_descriptorSetLayout,
            1);
    VK_CHECK_RESULT(vkCreatePipelineLayout(_vulkanDevice->logicalDevice, &pipelineLayoutInfo, nullptr, &_pipelineLayout));

    // Descriptor set
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo =
        vks::initializers::descriptorSetAllocateInfo(
            _descriptorPool,
            &_descriptorSetLayout,
            1);

    VK_CHECK_RESULT(vkAllocateDescriptorSets(_vulkanDevice->logicalDevice, &descriptorSetAllocInfo, &_descriptorSet));

    VkDescriptorImageInfo texDescriptor =
        vks::initializers::descriptorImageInfo(
            _sampler,
            _view,
            VK_IMAGE_LAYOUT_GENERAL);

    std::array<VkWriteDescriptorSet, 1> writeDescriptorSets;
    writeDescriptorSets[0] = vks::initializers::writeDescriptorSet(_descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &texDescriptor);
    vkUpdateDescriptorSets(_vulkanDevice->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

    // Pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineCache(_vulkanDevice->logicalDevice, &pipelineCacheCreateInfo, nullptr, &_pipelineCache));
}



///////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   preparePipeline
//

// Prepare a separate pipeline for the font rendering decoupled from the main application
void
TextOverlay::preparePipeline()
{
    // Enable blending, using alpha from red channel of the font texture (see text.frag)
    VkPipelineColorBlendAttachmentState blendAttachmentState{};
    blendAttachmentState.blendEnable = VK_TRUE;
    blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);
    VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
    VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

    std::array<VkVertexInputBindingDescription, 2> vertexInputBindings = {
      vks::initializers::vertexInputBindingDescription(0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX),
      vks::initializers::vertexInputBindingDescription(1, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX),
    };
    std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributes = {
      vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, 0),					// Location 0: Position
      vks::initializers::vertexInputAttributeDescription(1, 1, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec2)),	// Location 1: UV
    };

    VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
    vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
    vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
    vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = vks::initializers::pipelineCreateInfo(_pipelineLayout, _renderPass, 0);
    pipelineCreateInfo.pVertexInputState = &vertexInputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(_shaderStages.size());
    pipelineCreateInfo.pStages = _shaderStages.data();

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(_vulkanDevice->logicalDevice, _pipelineCache, 1, &pipelineCreateInfo, nullptr, &_pipeline));
}


///////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   prepareRenderPass
//

// Prepare a separate render pass for rendering the text as an overlay
void
TextOverlay::prepareRenderPass()
{
    VkAttachmentDescription attachments[2] = {};

    // Color attachment
    attachments[0].format = _colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    // Don't clear the framebuffer (like the renderpass from the example does)
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Depth attachment
    attachments[1].format = _depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Use subpass dependencies for image layout transitions
    VkSubpassDependency subpassDependencies[2] = {};

    // Transition from final to initial (VK_SUBPASS_EXTERNAL refers to all commmands executed outside of the actual renderpass)
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Transition from initial to final
    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.flags = 0;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = NULL;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
    subpassDescription.pResolveAttachments = NULL;
    subpassDescription.pDepthStencilAttachment = &depthReference;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = NULL;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = NULL;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = subpassDependencies;

    VK_CHECK_RESULT(vkCreateRenderPass(_vulkanDevice->logicalDevice, &renderPassInfo, nullptr, &_renderPass));
}


///////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   addText
//

// Add text to the current buffer
    // todo : drop shadow? color attribute?
void
TextOverlay::addText(std::string text, float x, float y, TextAlign align)
{
    const uint32_t firstChar = STB_FONT_consolas_24_latin1_FIRST_CHAR;

    assert(_mapped != nullptr);

    const float charW = 1.5f / *_frameBufferWidth;
    const float charH = 1.5f / *_frameBufferHeight;

    float fbW = (float)*_frameBufferWidth;
    float fbH = (float)*_frameBufferHeight;
    x = (x / fbW * 2.0f) - 1.0f;
    y = (y / fbH * 2.0f) - 1.0f;

    // Calculate text width
    float textWidth = 0;
    for (auto letter : text)
    {
        stb_fontchar* charData = &_stbFontData[(uint32_t)letter - firstChar];
        textWidth += charData->advance * charW;
    }

    switch (align)
    {
    case alignRight:
        x -= textWidth;
        break;
    case alignCenter:
        x -= textWidth / 2.0f;
        break;
    }

    // Generate a uv mapped quad per char in the new text
    for (auto letter : text)
    {
        stb_fontchar* charData = &_stbFontData[(uint32_t)letter - firstChar];

        _mapped->x = (x + (float)charData->x0 * charW);
        _mapped->y = (y + (float)charData->y0 * charH);
        _mapped->z = charData->s0;
        _mapped->w = charData->t0;
        _mapped++;

        _mapped->x = (x + (float)charData->x1 * charW);
        _mapped->y = (y + (float)charData->y0 * charH);
        _mapped->z = charData->s1;
        _mapped->w = charData->t0;
        _mapped++;

        _mapped->x = (x + (float)charData->x0 * charW);
        _mapped->y = (y + (float)charData->y1 * charH);
        _mapped->z = charData->s0;
        _mapped->w = charData->t1;
        _mapped++;

        _mapped->x = (x + (float)charData->x1 * charW);
        _mapped->y = (y + (float)charData->y1 * charH);
        _mapped->z = charData->s1;
        _mapped->w = charData->t1;
        _mapped++;

        x += charData->advance * charW;

        _numLetters++;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   beginTextUpdate
//
//   Map buffer 

void
TextOverlay::beginTextUpdate()
{
    VK_CHECK_RESULT(vkMapMemory(_vulkanDevice->logicalDevice, _memory, 0, VK_WHOLE_SIZE, 0, (void**)&_mapped));
    _numLetters = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   endTextUpdate
//  // Unmap buffer and update command buffers

void
TextOverlay::endTextUpdate()
{
    vkUnmapMemory(_vulkanDevice->logicalDevice, _memory);
    _mapped = nullptr;
    updateCommandBuffers();
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   updateCommandBuffers

// Needs to be called by the application

void
TextOverlay::updateCommandBuffers()
{
    const VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    VkClearValue clearValues[2];
    clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };

    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = _renderPass;
    renderPassBeginInfo.renderArea.extent.width = *_frameBufferWidth;
    renderPassBeginInfo.renderArea.extent.height = *_frameBufferHeight;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    for (int32_t i = 0; i < _cmdBuffers.size(); ++i)
    {

        VkCommandBuffer& cmdBuffer = _cmdBuffers[i];
    
        renderPassBeginInfo.framebuffer = *_frameBuffers[i];

        VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

        vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = vks::initializers::viewport((float)*_frameBufferWidth, (float)*_frameBufferHeight, 0.0f, 1.0f);
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

        VkRect2D scissor = vks::initializers::rect2D(*_frameBufferWidth, *_frameBufferHeight, 0, 0);
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descriptorSet, 0, NULL);

        VkDeviceSize offsets = 0;
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &_buffer, &offsets);
        vkCmdBindVertexBuffers(cmdBuffer, 1, 1, &_buffer, &offsets);
        for (uint32_t j = 0; j < _numLetters; j++)
        {
            vkCmdDraw(_cmdBuffers[i], 4, 1, j * 4, 0);
        }


        vkCmdEndRenderPass(_cmdBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(_cmdBuffers[i]));
    }
}


