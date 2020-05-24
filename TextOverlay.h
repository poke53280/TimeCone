#pragma once


#include <vulkan/vulkan.h>
#include "VulkanDevice.hpp"
#include <glm/glm.hpp>

#include "stb_font_consolas_24_latin1.inl"


#include <array>
/*
* TextOverlay:
*
* Copyright (C) 2016-2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

// Max. number of chars the text overlay buffer can hold
#define TEXTOVERLAY_MAX_CHAR_COUNT 2048


class TextOverlay
{
private:
    vks::VulkanDevice* _vulkanDevice;

    VkQueue _queue;
    VkFormat _colorFormat;
    VkFormat _depthFormat;

    uint32_t* _frameBufferWidth;
    uint32_t* _frameBufferHeight;

    VkSampler _sampler;
    VkImage _image;
    VkImageView _view;
    VkBuffer _buffer;
    VkDeviceMemory _memory;
    VkDeviceMemory _imageMemory;
    VkDescriptorPool _descriptorPool;
    VkDescriptorSetLayout _descriptorSetLayout;
    VkDescriptorSet _descriptorSet;
    VkPipelineLayout _pipelineLayout;
    VkPipelineCache _pipelineCache;
    VkPipeline _pipeline;
    VkRenderPass _renderPass;
    VkCommandPool _commandPool;

    std::vector<VkFramebuffer*> _frameBuffers;
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

    // Pointer to mapped vertex buffer
    glm::vec4* _mapped = nullptr;

    stb_fontchar _stbFontData[STB_FONT_consolas_24_latin1_NUM_CHARS];
    uint32_t _numLetters;
public:

    enum TextAlign { alignLeft, alignCenter, alignRight };

    bool _visible = true;
    std::vector<VkCommandBuffer> _cmdBuffers;

    TextOverlay(
        vks::VulkanDevice* vulkanDevice,
        VkQueue queue,
        std::vector<VkFramebuffer>& framebuffers,
        VkFormat colorformat,
        VkFormat depthformat,
        uint32_t* framebufferwidth,
        uint32_t* framebufferheight,
        std::vector<VkPipelineShaderStageCreateInfo> shaderstages);
    
    ~TextOverlay();


    
    void prepareResources();


    // Prepare a separate pipeline for the font rendering decoupled from the main application
    void preparePipeline();

    // Prepare a separate render pass for rendering the text as an overlay
    void prepareRenderPass();

    // Map buffer 
    void beginTextUpdate();

    // Add text to the current buffer
    // todo : drop shadow? color attribute?
    void addText(std::string text, float x, float y, TextAlign align);

    
    void endTextUpdate();
    

    // Needs to be called by the application
    void updateCommandBuffers();

    // Submit the text command buffers to a queue
    // Does a queue wait idle
    void submit(VkQueue queue, uint32_t bufferindex);

};

