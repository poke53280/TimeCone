
#include "stdafx.h"

// READ https://www.jeremyong.com/c++/vulkan/graphics/rendering/2018/03/26/how-to-learn-vulkan/

// https://www.gamedevs.org/uploads/efficient-buffer-management.pdf

// PASSING DATA TO INSTANCES: 
// ehttps://github.com/SaschaWillems/Vulkan/blob/master/examples/instancing/instancing.cpp



// MOVE THINGS:
//
//Each box :
//*maintain a 2 channel data stream updated once in a while (key press).
//
//
// https ://www.gamedevs.org/uploads/efficient-buffer-management.pdf
//
//


/* Based on the tutorial series by Sascha Willems: */

/*
*
* Copyright (C) 2016-2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/


/* Dependant on:*/

/* IMGUI by Omar Cornut:*/
/*
*  This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

/* Dependant on:*/
// External dependency: GLM. https://glm.g-truc.net/index.html


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <vector>
#include <exception>

#include <random>
#include <cmath>

#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>

#include "triangleexamplebase.h"

#include "stb_font_consolas_24_latin1.inl"

#include "TextOverlay.h"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false



#include <math.h>

#include "ArenaCubes.h"

// Set to "true" to enable Vulkan's validation layers (see vulkandebug.cpp for details)
#define ENABLE_VALIDATION false

#define E_PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062

VkBuffer createBuffer(VkDevice device, uint32_t size, uint32_t usage) {

    VkBufferCreateInfo c = {};

    VkStructureType structurType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

    c.sType = structurType;
    c.size = size;

    // Buffer is used as the copy source

    c.usage = usage;

    VkBuffer
        b = 0;

    VK_CHECK_RESULT(vkCreateBuffer(device, &c, nullptr, &b));

    return b;
}



class VulkanExample : public VulkanExampleBase
{
public:

   float x_center = 0.f;
   float y_center = 0.f;

   static const uint32_t static_num_instances = 1000;

  // Vertex buffer and attributes
  struct {
    VkDeviceMemory memory;															// Handle to the device memory for this buffer
    VkBuffer buffer;																// Handle to the Vulkan buffer object that the memory is bound to
  } arena_vertices;

  // Index buffer
  struct
  {
    VkDeviceMemory memory;
    VkBuffer buffer;
    uint32_t count;
  } arena_indices;


  // Instance data
  struct
  {
      VkDeviceMemory memory;
      VkBuffer buffer;
      uint32_t count;
  } arena_instance_data;


  // Uniform buffer block object
  struct {
    VkDeviceMemory memory;
    VkBuffer buffer;
    VkDescriptorBufferInfo descriptor;
  }  arena_uniformBufferVS;

  // For simplicity we use the same uniform block layout as in the shader:
  //
  //	layout(set = 0, binding = 0) uniform UBO
  //	{
  //		mat4 projectionMatrix;
  //		mat4 modelMatrix;
  //		mat4 viewMatrix;
  //	} ubo;
  //
  // This way we can just memcopy the ubo data to the ubo
  // Note: You should use data types that align with the GPU in order to avoid manual padding (vec4, mat4)
  struct {
    glm::mat4 projectionMatrix;
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::vec4 colorParams;
  } arena_uboVS;

  // The pipeline layout is used by a pipeline to access the descriptor sets 
  // It defines interface (without binding any actual data) between the shader stages used by the pipeline and the shader resources
  // A pipeline layout can be shared among multiple pipelines as long as their interfaces match
  VkPipelineLayout arena_pipelineLayout;

  // Pipelines (often called "pipeline state objects") are used to bake all states that affect a pipeline
  // While in OpenGL every state can be changed at (almost) any time, Vulkan requires to layout the graphics (and compute) pipeline states upfront
  // So for each combination of non-dynamic pipeline states you need a new pipeline (there are a few exceptions to this not discussed here)
  // Even though this adds a new dimension of planing ahead, it's a great opportunity for performance optimizations by the driver
  VkPipeline arena_pipeline;

  // The descriptor set layout describes the shader binding layout (without actually referencing descriptor)
  // Like the pipeline layout it's pretty much a blueprint and can be used with different descriptor sets as long as their layout matches
  VkDescriptorSetLayout arena_descriptorSetLayout;

  // The descriptor set stores the resources bound to the binding points in a shader
  // It connects the binding points of the different shaders with the buffers and images used for those bindings
  VkDescriptorSet arena_descriptorSet;


  // Synchronization primitives
  // Synchronization is an important concept of Vulkan that OpenGL mostly hid away. Getting this right is crucial to using Vulkan.

  // Semaphores
  // Used to coordinate operations within the graphics queue and ensure correct command ordering
  VkSemaphore presentCompleteSemaphore;
  VkSemaphore renderCompleteSemaphore;

  // Fences
  // Used to check the completion of queue operations (e.g. command buffer execution)
  std::vector<VkFence> waitFences;

  TextOverlay *textOverlay = nullptr;

  VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
  {

    zoom = -125.0f;
    title = "THE GAME";
    // Values not set here are initialized in the base class constructor
    settings.overlay = false;
  }

  ~VulkanExample()
  {
    // Clean up used Vulkan resources 
    // Note: Inherited destructor cleans up resources stored in base class
    // Clean up used Vulkan resources 
    // Note : Inherited destructor cleans up resources stored in base class

    // Clean up texture resources

    vkDestroyPipeline(device, arena_pipeline, nullptr);

    vkDestroyPipelineLayout(device, arena_pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, arena_descriptorSetLayout, nullptr);

    vkDestroyBuffer(device, arena_vertices.buffer, nullptr);
    vkFreeMemory(device, arena_vertices.memory, nullptr);

    vkDestroyBuffer(device, arena_indices.buffer, nullptr);
    vkFreeMemory(device, arena_indices.memory, nullptr);

    vkDestroyBuffer(device, arena_uniformBufferVS.buffer, nullptr);
    vkFreeMemory(device, arena_uniformBufferVS.memory, nullptr);


    if (textOverlay != nullptr)
    {
      delete(textOverlay);
      textOverlay = nullptr;
    }

    vkDestroySemaphore(device, presentCompleteSemaphore, nullptr);
    vkDestroySemaphore(device, renderCompleteSemaphore, nullptr);

    for (auto& fence : waitFences)
    {
      vkDestroyFence(device, fence, nullptr);
    }
  }

  void prepareTextOverlay()
  {
    // Load the text rendering shaders
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.push_back(loadShader(getAssetPath() + "shaders/textoverlay/text.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
    shaderStages.push_back(loadShader(getAssetPath() + "shaders/textoverlay/text.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));

    textOverlay = new TextOverlay(
      vulkanDevice,
      queue,
      frameBuffers,
      swapChain.colorFormat,
      depthFormat,
      &width,
      &height,
      shaderStages
    );
    updateTextOverlay();
  }


  // Update the text buffer displayed by the text overlay
  void updateTextOverlay(void)
  {
    textOverlay->beginTextUpdate();

    textOverlay->addText(title, 5.0f, 5.0f, TextOverlay::alignLeft);

    float arena_rotationX = arena_uboVS.modelMatrix[0][0];
    float arena_rotationY = arena_uboVS.modelMatrix[1][0];

    float rAngle = std::atan2(arena_rotationY, arena_rotationX);

    if (rAngle < 0.f)
    {
      rAngle += float(2.0 * E_PI);
    }

    rAngle /= float(2.0 * E_PI);   // [0..1]

    rAngle -= 0.75f;   // Some magic to fit matrix values. Clean up.

    if (rAngle < 0.f)
    {
      rAngle += 1.f;
    }

    rAngle *= float(2.0 * E_PI);

    float fRay = rAngle * 2.0f / float(E_PI) / 0.05f;

    int nRay = int(fRay + .5f);

    textOverlay->addText(title, 5.0f, 5.0f, TextOverlay::alignLeft);

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << (frameTimer * 1000.0f) << "ms (" << lastFPS << " fps) " << deviceProperties.deviceName << " rotation " << rAngle << " nRay " << nRay;
    textOverlay->addText(ss.str(), 5.0f, 25.0f, TextOverlay::alignLeft);

    // Display current model view matrix
    textOverlay->addText("model view matrix", (float)width, 5.0f, TextOverlay::alignRight);


    for (uint32_t i = 0; i < 4; i++)
    {
      ss.str("");
      ss << std::fixed << std::setprecision(2) << std::showpos;
      ss << arena_uboVS.modelMatrix[0][i] << " " << arena_uboVS.modelMatrix[1][i] << " " << arena_uboVS.modelMatrix[2][i] << " " << arena_uboVS.modelMatrix[3][i];
      textOverlay->addText(ss.str(), (float)width, 25.0f + (float)i * 20.0f, TextOverlay::alignRight);
    }

    glm::vec3 projected = glm::project(glm::vec3(34.6281052f, 20.0473022f, 11.0037498f), arena_uboVS.modelMatrix, arena_uboVS.projectionMatrix, glm::vec4(0, 0, (float)width, (float)height));

    // textOverlay->addText("IRAY7.I5", projected.x, projected.y, TextOverlay::alignCenter);
    textOverlay->addText("[X]", projected.x, projected.y, TextOverlay::alignCenter);

    textOverlay->addText("Info...", 5.0f, 65.0f, TextOverlay::alignLeft);
    textOverlay->addText("---", 5.0f, 85.0f, TextOverlay::alignLeft);

    textOverlay->endTextUpdate();
  }

  // This function is used to request a device memory type that supports all the property flags we request (e.g. device local, host visibile)
  // Upon success it will return the index of the memory type that fits our requestes memory properties
  // This is necessary as implementations can offer an arbitrary number of memory types with different
  // memory properties. 
  // You can check http://vulkan.gpuinfo.org/ for details on different memory configurations
  uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
  {
    // Iterate over all memory types available for the device used in this example
    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
    {
      if ((typeBits & 1) == 1)
      {
        if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
          return i;
        }
      }
      typeBits >>= 1;
    }

    throw "Could not find a suitable memory type!";
  }

  // Create the Vulkan synchronization primitives used in this example
  void prepareSynchronizationPrimitives()
  {
    // Semaphores (Used for correct command ordering)
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;

    // Semaphore used to ensures that image presentation is complete before starting to submit again
    VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentCompleteSemaphore));

    // Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
    VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderCompleteSemaphore));

    // Fences (Used to check draw command buffer completion)
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Create in signaled state so we don't wait on first render of each command buffer
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    waitFences.resize(drawCmdBuffers.size());
    for (auto& fence : waitFences)
    {
      VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
    }
  }

  // Get a new command buffer from the command pool
  // If begin is true, the command buffer is also started so we can start adding commands
  VkCommandBuffer getCommandBuffer(bool begin)
  {
    VkCommandBuffer cmdBuffer;


    VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool = cmdPool;
    cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocateInfo.commandBufferCount = 1;

    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cmdBuffer));

    // If requested, also start the new command buffer
    if (begin)
    {
      VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
      VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
    }

    return cmdBuffer;
  }

  // End the command buffer and submit it to the queue
  // Uses a fence to ensure command buffer has finished executing before deleting it
  void flushCommandBuffer(VkCommandBuffer commandBuffer)
  {
    assert(commandBuffer != VK_NULL_HANDLE);

    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));

    // Submit to the queue
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));

    // Wait for the fence to signal that command buffer has finished executing
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
  }
  

  // Build separate command buffers for every framebuffer image
  // Unlike in OpenGL all rendering commands are recorded once into command buffers that are then resubmitted to the queue
  // This allows to generate work upfront and from multiple threads, one of the biggest advantages of Vulkan
  void buildCommandBuffers()
  {
    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext = nullptr;

    // Set clear values for all framebuffer attachments with loadOp set to clear
    // We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
    VkClearValue clearValues[2];
    clearValues[0].color = { { 0.1f, 0.1f, 0.3f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    for (uint32_t iCmdBuffer = 0; iCmdBuffer < drawCmdBuffers.size(); ++iCmdBuffer)
    {
      // Set target frame buffer
      renderPassBeginInfo.framebuffer = frameBuffers[iCmdBuffer];

      const VkCommandBuffer& cmdBuffer = drawCmdBuffers[iCmdBuffer];


      VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

      // Start the first sub pass specified in our default render pass setup by the base class
      // This will clear the color and depth attachment
      vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

      // Update dynamic viewport state
      VkViewport viewport = {};
      viewport.height = (float)height;
      viewport.width = (float)width;
      viewport.minDepth = (float) 0.0f;
      viewport.maxDepth = (float) 1.0f;
      vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

      // Update dynamic scissor state
      VkRect2D scissor = {};
      scissor.extent.width = width;
      scissor.extent.height = height;
      scissor.offset.x = 0;
      scissor.offset.y = 0;
      vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

      // Bind descriptor sets describing shader binding points

      vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, arena_pipelineLayout, 0, 1, &arena_descriptorSet, 0, nullptr);


      // Bind the rendering pipeline
      // The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
      vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, arena_pipeline);

      // Bind triangle vertex buffer (contains position and colors)
      VkDeviceSize offsets[1] = { 0 };
      vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &arena_vertices.buffer, offsets);

      // Bind instance data
      vkCmdBindVertexBuffers(cmdBuffer, 1, 1, &arena_instance_data.buffer, offsets);

      // Bind triangle index buffer
      vkCmdBindIndexBuffer(cmdBuffer, arena_indices.buffer, 0, VK_INDEX_TYPE_UINT32);

      // Draw indexed triangle
      vkCmdDrawIndexed(cmdBuffer, arena_indices.count, static_num_instances, 0, 0, 0);

      vkCmdEndRenderPass(cmdBuffer);

      // Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to 
      // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

      VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
    }
  }

  void draw()
  {
    // Get next image in the swap chain (back/front buffer)
    VK_CHECK_RESULT(swapChain.acquireNextImage(presentCompleteSemaphore, &currentBuffer));

    // Use a fence to wait until the command buffer has finished execution before using it again
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFences[currentBuffer], VK_TRUE, UINT64_MAX));
    VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentBuffer]));

    // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // The submit info structure specifices a command buffer queue submission batch
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &waitStageMask;									// Pointer to the list of pipeline stages that the semaphore waits will occur at
    submitInfo.pWaitSemaphores = &presentCompleteSemaphore;							// Semaphore(s) to wait upon before the submitted command buffer starts executing
    submitInfo.waitSemaphoreCount = 1;												// One wait semaphore																				
    submitInfo.pSignalSemaphores = &renderCompleteSemaphore;						// Semaphore(s) to be signaled when command buffers have completed
    submitInfo.signalSemaphoreCount = 1;											// One signal semaphore
    submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];					// Command buffers(s) to execute in this batch (submission)
    submitInfo.commandBufferCount = 1;												// One command buffer




    if (textOverlay->_visible)
    {
      VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

      VkSubmitInfo submitInfo_text = {};
      submitInfo_text.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo_text.commandBufferCount = 1;
      submitInfo_text.pCommandBuffers = &textOverlay->_cmdBuffers[currentBuffer];

      VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo_text, waitFences[currentBuffer]));
      VK_CHECK_RESULT(vkQueueWaitIdle(queue));

    }
    else
    {
      // Submit to the graphics queue passing a wait fence
      VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentBuffer]));
    }



    // Present the current buffer to the swap chain
    // Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
    // This ensures that the image is not presented to the windowing system until all commands have been submitted
    VK_CHECK_RESULT(swapChain.queuePresent(queue, currentBuffer, renderCompleteSemaphore));
  }


  

  // Prepare vertex and index buffers for indexed triangles
  // Also uploads them to device local memory using staging and initializes vertex input and attribute binding to match the vertex shader
  void arena_prepareVertices()
  {
    std::vector<instance_data> lcInstance;

    get_instance_data(lcInstance, static_num_instances);

    uint32_t instanceBufferSize = static_cast<uint32_t>(lcInstance.size()) * sizeof(instance_data);


    // VmaMemoryUsage
    // VMA_MEMORY_USAGE_CPU_TO_GPU
       

    // Setup vertices

    std::vector<arena_vertex> lcVertex;

    // create_cube_arena(lcVertex);
    create_single_cube(lcVertex);

    uint32_t vertexBufferSize = static_cast<uint32_t>(lcVertex.size()) * sizeof(arena_vertex);

    uint32_t num_cubes = uint32_t(lcVertex.size() / 8);

    std::vector<uint32_t> lcIndex;

    setup_indices(lcIndex, num_cubes);

    arena_indices.count = static_cast<uint32_t>(lcIndex.size());
    uint32_t indexBufferSize = arena_indices.count * sizeof(uint32_t);
       


    // Static data like vertex and index buffer should be stored on the device memory 
    // for optimal (and fastest) access by the GPU
    //
    // To achieve this we use so-called "staging buffers" :
    // - Create a buffer that's visible to the host (and can be mapped)
    // - Copy the data to this buffer
    // - Create another buffer that's local on the device (VRAM) with the same size
    // - Copy the data from the host to the device using a command buffer
    // - Delete the host visible (staging) buffer
    // - Use the device local buffers for rendering

    struct StagingBuffer {
      VkDeviceMemory memory;
      VkBuffer buffer;
    };


    StagingBuffer vertices_stagingBuffer;
    StagingBuffer indices_stagingBuffer;
    StagingBuffer instance_stagingBuffer;
    
    //  Vertex buffer
    
    // Create a host-visible buffer to copy the vertex data to (staging buffer)


    VK_CHECK_RESULT(vulkanDevice->createBuffer(
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexBufferSize,
        &vertices_stagingBuffer.buffer,
        &vertices_stagingBuffer.memory,
        lcVertex.data()));


    // Create a device local buffer to which the (host local) vertex data will be copied (below) and which will be used for rendering

    VK_CHECK_RESULT(vulkanDevice->createBuffer(
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertexBufferSize,
        &arena_vertices.buffer,
        &arena_vertices.memory));


    // Index buffer
    
    // Copy index data to a buffer visible to the host (staging buffer)

    VK_CHECK_RESULT(vulkanDevice->createBuffer(
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        indexBufferSize,
        &indices_stagingBuffer.buffer,
        &indices_stagingBuffer.memory,
        lcIndex.data()));

    // Create destination buffer with device only visibility


    VK_CHECK_RESULT(vulkanDevice->createBuffer(
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        indexBufferSize,
        &arena_indices.buffer,
        &arena_indices.memory));

    

    // Instance buffer
    VK_CHECK_RESULT(vulkanDevice->createBuffer(
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        instanceBufferSize,
        &instance_stagingBuffer.buffer,
        &instance_stagingBuffer.memory,
        lcInstance.data()));


    VK_CHECK_RESULT(vulkanDevice->createBuffer(
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        instanceBufferSize,
        &arena_instance_data.buffer,
        &arena_instance_data.memory));


    
    // Buffer copies have to be submitted to a queue, so we need a command buffer for them
    // Note: Some devices offer a dedicated transfer queue (with only the transfer bit set) that may be faster when doing lots of copies
    {
        VkCommandBuffer copyCmd = getCommandBuffer(true);

        // Put buffer region copies into command buffer
        VkBufferCopy copyRegion = {};

        // Vertex buffer
        copyRegion.size = vertexBufferSize;
        vkCmdCopyBuffer(copyCmd, vertices_stagingBuffer.buffer, arena_vertices.buffer, 1, &copyRegion);

        // Index buffer
        copyRegion.size = indexBufferSize;
        vkCmdCopyBuffer(copyCmd, indices_stagingBuffer.buffer, arena_indices.buffer, 1, &copyRegion);

        // Instance data
        copyRegion.size = instanceBufferSize;
        vkCmdCopyBuffer(copyCmd, instance_stagingBuffer.buffer, arena_instance_data.buffer, 1, &copyRegion);

        // Flushing the command buffer will also submit it to the queue and uses a fence to ensure that all commands have been executed before returning
        flushCommandBuffer(copyCmd);
    }

    // Destroy staging buffers
    // Note: Staging buffer must not be deleted before the copies have been submitted and executed
    vkDestroyBuffer(device, vertices_stagingBuffer.buffer, nullptr);
    vkFreeMemory(device, vertices_stagingBuffer.memory, nullptr);
    vkDestroyBuffer(device, indices_stagingBuffer.buffer, nullptr);
    vkFreeMemory(device, indices_stagingBuffer.memory, nullptr);

    vkDestroyBuffer(device, instance_stagingBuffer.buffer, nullptr);
    vkFreeMemory(device, instance_stagingBuffer.memory, nullptr);


  }

  void setupDescriptorPool()
  {
    // We need to tell the API the number of max. requested descriptors per type
    VkDescriptorPoolSize typeCounts[1];
    // This example only uses one descriptor type (uniform buffer) and only requests one descriptor of this type
    typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    typeCounts[0].descriptorCount = 1;
    // For additional types you need to add new entries in the type count list
    // E.g. for two combined image samplers :
    // typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // typeCounts[1].descriptorCount = 2;

    // Create the global descriptor pool
    // All descriptors used in this example are allocated from this pool
    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.pNext = nullptr;
    descriptorPoolInfo.poolSizeCount = 1;
    descriptorPoolInfo.pPoolSizes = typeCounts;
    // Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
    descriptorPoolInfo.maxSets = 1;

    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
  }

  void arena_setupDescriptorSetLayout()
  {
    // Setup layout of descriptors used in this example
    // Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
    // So every shader binding should map to one descriptor set layout binding



    std::array<VkDescriptorSetLayoutBinding, 2> layoutBinding{};


    // Binding 0: Uniform buffer (Vertex shader)
    
    layoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding[0].binding = 0;
    layoutBinding[0].descriptorCount = 1;
    layoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBinding[0].pImmutableSamplers = nullptr;

    // Binding 1: Uniform buffer (Instanced data)
    
    layoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding[0].binding = 1;
    layoutBinding[1].descriptorCount = 1;
    layoutBinding[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBinding[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
    descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayout.pNext = nullptr;
    descriptorLayout.bindingCount = uint32_t(layoutBinding.size());
    descriptorLayout.pBindings = layoutBinding.data();

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &arena_descriptorSetLayout));

    // Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
    // In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
    pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext = nullptr;
    pPipelineLayoutCreateInfo.setLayoutCount = 1;
    pPipelineLayoutCreateInfo.pSetLayouts = &arena_descriptorSetLayout;

    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &arena_pipelineLayout));
  }

  void arena_setupDescriptorSet()
  {
    // Allocate a new descriptor set from the global descriptor pool
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &arena_descriptorSetLayout;

    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &arena_descriptorSet));

    // Update the descriptor set determining the shader binding points
    // For every binding point used in a shader there needs to be one
    // descriptor set matching that binding point

    VkWriteDescriptorSet writeDescriptorSet = {};

    // Binding 0 : Uniform buffer
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = arena_descriptorSet;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.pBufferInfo = &arena_uniformBufferVS.descriptor;
    // Binds this uniform buffer to binding point 0
    writeDescriptorSet.dstBinding = 0;

    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
  }

  // Create the depth (and stencil) buffer attachments used by our framebuffers
  // Note: Override of virtual function in the base class and called from within VulkanExampleBase::prepare
  void setupDepthStencil()
  {
    // Create an optimal image used as the depth stencil attachment
    VkImageCreateInfo image = {};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = depthFormat;
    // Use example's height and width
    image.extent = { width, height, 1 };
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &depthStencil.image));

    // Allocate memory for the image (device local) and bind it to our image
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &depthStencil.mem));
    VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.mem, 0));

    // Create a view for the depth stencil image
    // Images aren't directly accessed in Vulkan, but rather through views described by a subresource range
    // This allows for multiple views of one image with differing ranges (e.g. for different layers)
    VkImageViewCreateInfo depthStencilView = {};
    depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthStencilView.format = depthFormat;
    depthStencilView.subresourceRange = {};
    depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    depthStencilView.subresourceRange.baseMipLevel = 0;
    depthStencilView.subresourceRange.levelCount = 1;
    depthStencilView.subresourceRange.baseArrayLayer = 0;
    depthStencilView.subresourceRange.layerCount = 1;
    depthStencilView.image = depthStencil.image;
    VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &depthStencil.view));
  }

  // Create a frame buffer for each swap chain image
  // Note: Override of virtual function in the base class and called from within VulkanExampleBase::prepare
  void setupFrameBuffer()
  {
    // Create a frame buffer for every image in the swapchain
    frameBuffers.resize(swapChain.imageCount);
    for (size_t i = 0; i < frameBuffers.size(); i++)
    {
      std::array<VkImageView, 2> attachments;
      attachments[0] = swapChain.buffers[i].view;									// Color attachment is the view of the swapchain image			
      attachments[1] = depthStencil.view;											// Depth/Stencil attachment is the same for all frame buffers			

      VkFramebufferCreateInfo frameBufferCreateInfo = {};
      frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      // All frame buffers use the same renderpass setup
      frameBufferCreateInfo.renderPass = renderPass;
      frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
      frameBufferCreateInfo.pAttachments = attachments.data();
      frameBufferCreateInfo.width = width;
      frameBufferCreateInfo.height = height;
      frameBufferCreateInfo.layers = 1;
      // Create the framebuffer
      VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
    }
  }

  // Render pass setup
  // Render passes are a new concept in Vulkan. They describe the attachments used during rendering and may contain multiple subpasses with attachment dependencies 
  // This allows the driver to know up-front what the rendering will look like and is a good opportunity to optimize especially on tile-based renderers (with multiple subpasses)
  // Using sub pass dependencies also adds implicit layout transitions for the attachment used, so we don't need to add explicit image memory barriers to transform them
  // Note: Override of virtual function in the base class and called from within VulkanExampleBase::prepare
  void setupRenderPass()
  {
    // This example will use a single render pass with one subpass

    // Descriptors for the attachments used by this renderpass
    std::array<VkAttachmentDescription, 2> attachments = {};

    // Color attachment
    attachments[0].format = swapChain.colorFormat;									// Use the color format selected by the swapchain
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;									// We don't use multi sampling in this example
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear this attachment at the start of the render pass
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;							// Keep it's contents after the render pass is finished (for displaying it)
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// We don't use stencil, so don't care for load
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// Same for store
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;						// Layout at render pass start. Initial doesn't matter, so we use undefined
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;					// Layout to which the attachment is transitioned when the render pass is finished
                                                                          // As we want to present the color buffer to the swapchain, we transition to PRESENT_KHR	
                                                                          // Depth attachment
    attachments[1].format = depthFormat;											// A proper depth format is selected in the example base
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at start of first subpass
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;						// We don't need depth after render pass has finished (DONT_CARE may result in better performance)
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// No stencil
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// No Stencil
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;						// Layout at render pass start. Initial doesn't matter, so we use undefined
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;	// Transition to depth/stencil attachment

                                                                                    // Setup attachment references
    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0;													// Attachment 0 is color
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;				// Attachment layout used as color during the subpass

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1;													// Attachment 1 is color
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;		// Attachment used as depth/stemcil used during the subpass

                                                                                // Setup a single subpass reference
    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;									// Subpass uses one color attachment
    subpassDescription.pColorAttachments = &colorReference;							// Reference to the color attachment in slot 0
    subpassDescription.pDepthStencilAttachment = &depthReference;					// Reference to the depth attachment in slot 1
    subpassDescription.inputAttachmentCount = 0;									// Input attachments can be used to sample from contents of a previous subpass
    subpassDescription.pInputAttachments = nullptr;									// (Input attachments not used by this example)
    subpassDescription.preserveAttachmentCount = 0;									// Preserved attachments can be used to loop (and preserve) attachments through subpasses
    subpassDescription.pPreserveAttachments = nullptr;								// (Preserve attachments not used by this example)
    subpassDescription.pResolveAttachments = nullptr;								// Resolve attachments are resolved at the end of a sub pass and can be used for e.g. multi sampling

                                                                    // Setup subpass dependencies
                                                                    // These will add the implicit ttachment layout transitionss specified by the attachment descriptions
                                                                    // The actual usage layout is preserved through the layout specified in the attachment reference		
                                                                    // Each subpass dependency will introduce a memory and execution dependency between the source and dest subpass described by
                                                                    // srcStageMask, dstStageMask, srcAccessMask, dstAccessMask (and dependencyFlags is set)
                                                                    // Note: VK_SUBPASS_EXTERNAL is a special constant that refers to all commands executed outside of the actual renderpass)
    std::array<VkSubpassDependency, 2> dependencies;

    // First dependency at the start of the renderpass
    // Does the transition from final to initial layout 
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;								// Producer of the dependency 
    dependencies[0].dstSubpass = 0;													// Consumer is our single subpass that will wait for the execution depdendency
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Second dependency at the end the renderpass
    // Does the transition from the initial to the final layout
    dependencies[1].srcSubpass = 0;													// Producer of the dependency is our single subpass
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;								// Consumer are all commands outside of the renderpass
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Create the actual renderpass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());		// Number of attachments used by this render pass
    renderPassInfo.pAttachments = attachments.data();								// Descriptions of the attachments used by the render pass
    renderPassInfo.subpassCount = 1;												// We only use one subpass in this example
    renderPassInfo.pSubpasses = &subpassDescription;								// Description of that subpass
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());	// Number of subpass dependencies
    renderPassInfo.pDependencies = dependencies.data();								// Subpass dependencies used by the render pass

    VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
  }

  // Vulkan loads it's shaders from an immediate binary representation called SPIR-V
  // Shaders are compiled offline from e.g. GLSL using the reference glslang compiler
  // This function loads such a shader from a binary file and returns a shader module structure
  VkShaderModule loadSPIRVShader(std::string filename)
  {
    size_t shaderSize;
    char* shaderCode = NULL;

    std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

    if (is.is_open())
    {
      shaderSize = is.tellg();
      is.seekg(0, std::ios::beg);
      // Copy file contents into a buffer
      shaderCode = new char[shaderSize];
      is.read(shaderCode, shaderSize);
      is.close();
      assert(shaderSize > 0);
    }

    if (shaderCode)
    {
      // Create a new shader module that will be used for pipeline creation
      VkShaderModuleCreateInfo moduleCreateInfo{};
      moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      moduleCreateInfo.codeSize = shaderSize;
      moduleCreateInfo.pCode = (uint32_t*)shaderCode;

      VkShaderModule shaderModule;
      VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

      delete[] shaderCode;

      return shaderModule;
    }
    else
    {
      std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
      return VK_NULL_HANDLE;
    }
  }

  void arena_preparePipelines()
  {
    // Create the graphics pipeline used in this example
    // Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
    // A pipeline is then stored and hashed on the GPU making pipeline changes very fast
    // Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
    pipelineCreateInfo.layout = arena_pipelineLayout;
    // Renderpass this pipeline is attached to
    pipelineCreateInfo.renderPass = renderPass;

    // Construct the different states making up the pipeline

    // Input assembly state describes how primitives are assembled
    // This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.lineWidth = 4.0f;

    // Color blend state describes how blend factors are calculated (if used)
    // We need one blend attachment state per color attachment (even if blending is not used
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};

    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;


    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

                                            // Viewport state sets the number of viewports and scissor used in this pipeline
                                            // Note: This is actually overriden by the dynamic states (see below)
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Enable dynamic states
    // Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
    // To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
    // For this example we will set the viewport and scissor using dynamic states
    std::vector<VkDynamicState> dynamicStateEnables;
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables.data();
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

    // Depth and stencil state containing depth and stencil compare and test operations
    // We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
    depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
    depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.front = depthStencilState.back;

    // Multi sampling state
    // This example does not make use fo multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.pSampleMask = nullptr;

    // Vertex input descriptions 
    // Specifies the vertex input parameters for a pipeline

    // Vertex input binding

    std::array<VkVertexInputBindingDescription, 2> vertexInputBinding;

    vertexInputBinding[0].binding = 0;
    vertexInputBinding[0].stride = sizeof(arena_vertex);
    vertexInputBinding[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInputBinding[1].binding = 1;
    vertexInputBinding[1].stride = sizeof(instance_data);
    vertexInputBinding[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;



    // Inpute attribute bindings describe shader attribute locations and memory layouts
    std::array<VkVertexInputAttributeDescription, 3> vertexInputAttributs;
    // These match the following shader layout (see triangle.vert):
    //	layout (location = 0) in vec3 inPos;
    //	layout (location = 1) in vec3 inColor;
    // Attribute location 0: Position
    vertexInputAttributs[0].binding = 0;
    vertexInputAttributs[0].location = 0;
    // Position attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
    vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributs[0].offset = offsetof(arena_vertex, position);
    // Attribute location 1: Color
    vertexInputAttributs[1].binding = 0;
    vertexInputAttributs[1].location = 1;

    // Color attribute is four 32 bit signed (SFLOAT) floats (R32 G32 B32 A32)
    vertexInputAttributs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertexInputAttributs[1].offset = offsetof(arena_vertex, colorRGBA);

    //  layout(location = 2) in float instanced_data;
    vertexInputAttributs[2].binding = 1;
    vertexInputAttributs[2].location = 2;
    vertexInputAttributs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertexInputAttributs[2].offset = offsetof(instance_data, data);


    // Vertex input state used for pipeline creation
    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = uint32_t(vertexInputBinding.size());
    vertexInputState.pVertexBindingDescriptions = vertexInputBinding.data();
    vertexInputState.vertexAttributeDescriptionCount = 3;
    vertexInputState.pVertexAttributeDescriptions = vertexInputAttributs.data();

    // Shaders
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

    // Vertex shader
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // Set pipeline stage for this shader
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    // Load binary SPIR-V shader
    shaderStages[0].module = loadSPIRVShader(getAssetPath() + "shaders/triangle/vert.spv");
    // Main entry point for the shader
    shaderStages[0].pName = "main";
    assert(shaderStages[0].module != VK_NULL_HANDLE);

    // Fragment shader
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // Set pipeline stage for this shader
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Load binary SPIR-V shader
    shaderStages[1].module = loadSPIRVShader(getAssetPath() + "shaders/triangle/frag.spv");
    // Main entry point for the shader
    shaderStages[1].pName = "main";
    assert(shaderStages[1].module != VK_NULL_HANDLE);

    // Set pipeline shader stage info
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();

    // Assign the pipeline states to the pipeline creation info structure
    pipelineCreateInfo.pVertexInputState = &vertexInputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlending;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.pDynamicState = &dynamicState;

    // Create rendering pipeline using the specified states
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &arena_pipeline));

    // Shader modules are no longer needed once the graphics pipeline has been created
    vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
  }

  void arena_prepareUniformBuffers()
  {
    // Prepare and initialize a uniform buffer block containing shader uniforms
    // Single uniforms like in OpenGL are no longer present in Vulkan. All Shader uniforms are passed via uniform buffer blocks
    VkMemoryRequirements memReqs;

    // Vertex shader uniform buffer block
    VkBufferCreateInfo bufferInfo = {};
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.allocationSize = 0;
    allocInfo.memoryTypeIndex = 0;

    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(arena_uboVS);
    // This buffer will be used as a uniform buffer
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    // Create a new buffer
    VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &arena_uniformBufferVS.buffer));
    // Get memory requirements including size, alignment and memory type 
    vkGetBufferMemoryRequirements(device, arena_uniformBufferVS.buffer, &memReqs);
    allocInfo.allocationSize = memReqs.size;
    // Get the memory type index that supports host visibile memory access
    // Most implementations offer multiple memory types and selecting the correct one to allocate memory from is crucial
    // We also want the buffer to be host coherent so we don't have to flush (or sync after every update.
    // Note: This may affect performance so you might not want to do this in a real world application that updates buffers on a regular base
    allocInfo.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    // Allocate memory for the uniform buffer
    VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &(arena_uniformBufferVS.memory)));
    // Bind memory to buffer
    VK_CHECK_RESULT(vkBindBufferMemory(device, arena_uniformBufferVS.buffer, arena_uniformBufferVS.memory, 0));

    // Store information in the uniform's descriptor that is used by the descriptor set
    arena_uniformBufferVS.descriptor.buffer = arena_uniformBufferVS.buffer;
    arena_uniformBufferVS.descriptor.offset = 0;
    arena_uniformBufferVS.descriptor.range = sizeof(arena_uboVS);

    arena_updateUniformBuffers();
  }

  void arena_updateUniformBuffers()
  {
    // Update matrices
    arena_uboVS.projectionMatrix = glm::perspective(glm::radians(35.0f), (float)width / (float)height, 0.1f, 4096.0f);

    arena_uboVS.modelMatrix = glm::mat4(1.0f);


    if (camera.keys.up) {
        y_center -= 0.01f;
    }

    if (camera.keys.down) {
        y_center += 0.01f;
    }

    if (camera.keys.left) {
        x_center -= 0.01f;
    }

    if (camera.keys.right) {
        x_center += 0.01f;
    }



    arena_uboVS.viewMatrix = glm::lookAt(glm::vec3(x_center, y_center, 150), glm::vec3(x_center, y_center, 0), glm::vec3(0, 1, 0));
    



    using namespace std::chrono;
    unsigned long long ms = duration_cast< milliseconds >( system_clock::now().time_since_epoch()).count();

    double f = double(ms * 2.0 * 3.1415 / 10000.0);

    float fWave = float((sin(f) + 1) / 2.0);


    // Set color params
    arena_uboVS.colorParams = glm::vec4(fWave, 0, 0, 0);


    // Map uniform buffer and update it

    uint8_t *pData;

    VK_CHECK_RESULT(vkMapMemory(device, arena_uniformBufferVS.memory, 0, sizeof(arena_uboVS), 0, (void **)&pData));

    memcpy(pData, &arena_uboVS, sizeof(arena_uboVS));

    // Unmap after data has been copied
    // Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU

    vkUnmapMemory(device, arena_uniformBufferVS.memory);
  }

  void prepare()
  {
    VulkanExampleBase::prepare();
    prepareSynchronizationPrimitives();
    arena_prepareVertices();
    arena_prepareUniformBuffers();
    arena_setupDescriptorSetLayout();
    arena_preparePipelines();
    setupDescriptorPool();
    arena_setupDescriptorSet();
    buildCommandBuffers();
    prepareTextOverlay();
    prepared = true;
  }

  virtual void render()
  {
    if (!prepared)
      return;
    draw();
  }

  virtual void viewChanged()
  {
    // This function is called by the base example class each time the view is changed by user input
    arena_updateUniformBuffers();
    updateTextOverlay();
  }
};


VulkanExample *vulkanExample;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (vulkanExample != NULL)
  {
    vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
  }
  return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

void load_numpy()
{
	const char* zFilename = "C:\\Users\\T149900\\source\\repos\\TimeCone\\np_array.npy";

	std::ifstream input(zFilename, std::ios::binary);

	// copies all data into buffer
	std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});

	unsigned short int data_start = unsigned short int(buffer[9] * 256 + buffer[8]);

	const unsigned char* data = buffer.data() + 10 + data_start;

	const unsigned char first_data = data[0];

	int num_data_bytes = int(buffer.size() - (10 + int(data_start)));

	int num_float32 = num_data_bytes / 4;

	const float* fData = reinterpret_cast<const float*> (data);
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
	load_numpy();

	for (size_t i = 0; i < __argc; i++) { VulkanExample::args.push_back(__argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow(hInstance, WndProc);
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}


