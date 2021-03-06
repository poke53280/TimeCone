/*
* Vulkan Example base class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#pragma comment(linker, "/subsystem:windows")

#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <ShellScalingAPI.h>

#include <iostream>
#include <chrono>
#include <sys/stat.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <string>
#include <array>
#include <numeric>

#include "vulkan/vulkan.h"

#include "keycodes.hpp"
#include "VulkanTools.h"
#include "VulkanDebug.h"
#include "VulkanUIOverlay.h"

#include "VulkanInitializers.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "camera.hpp"
#include "benchmark.hpp"

#include "SessionTime.h"
#include "RobotPark.h"
#include "TextOverlay.h"



class VulkanExampleBase
{
private:	
	// fps timer (one second interval)
	float _fpsTimer = 0.0f;
	// Get window title with example name, device, et.
	std::string getWindowTitle();
	/** brief Indicates that the view (position, rotation) has changed and buffers containing camera matrices need to be updated */
	bool _viewUpdated = false;
	// Destination dimensions for resizing the window
	uint32_t _destWidth;
	uint32_t _destHeight;
	bool _resizing = false;
	// Called if the window is resized and some resources have to be recreatesd
	void windowResize();
	void handleMouseMove(int32_t x, int32_t y);
protected:
	// Frame counter to display fps
	uint32_t _frameCounter = 0;
	uint32_t _lastFPS = 0;
	
	// Vulkan instance, stores all per-application states
	VkInstance _instance;
	
	// Physical device (GPU) that Vulkan will ise
	VkPhysicalDevice _physicalDevice;
	
	// Stores physical device properties (for e.g. checking device limits)
	VkPhysicalDeviceProperties _deviceProperties;
	
	// Stores the features available on the selected physical device (for e.g. checking if a feature is available)
	VkPhysicalDeviceFeatures _deviceFeatures;
	
	// Stores all available memory (type) properties for the physical device
	VkPhysicalDeviceMemoryProperties _deviceMemoryProperties;
	/**
	* Set of physical device features to be enabled for this example (must be set in the derived constructor)
	*
	* @note By default no phyiscal device features are enabled
	*/
	VkPhysicalDeviceFeatures _enabledFeatures{};
	/** @brief Set of device extensions to be enabled for this example (must be set in the derived constructor) */
	std::vector<const char*> _enabledDeviceExtensions;
	std::vector<const char*> _enabledInstanceExtensions;
	/** @brief Logical device, application's view of the physical device (GPU) */
	// todo: getter? should always point to VulkanDevice->device
	VkDevice _device;
	
	// Handle to the device graphics queue that command buffers are submitted to
	VkQueue _queue;
	
	// Depth buffer format (selected during Vulkan initialization)
	VkFormat _depthFormat;

	// Command buffer pool
	VkCommandPool _cmdPool;

	// Texture image
	VkImage _textureImage;
	VkDeviceMemory _textureImageMemory;


	/** @brief Pipeline stages used to wait at for graphics queue submissions */
	VkPipelineStageFlags _submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	// Contains command buffers and semaphores to be presented to the queue
	// VkSubmitInfo _submitInfo;
	
	// Command buffers used for rendering
	std::vector<VkCommandBuffer> _drawCmdBuffers;
	
	// Global render pass for frame buffer writes
	VkRenderPass _renderPass;
	
	// List of available frame buffers (same as number of swap chain images)
	std::vector<VkFramebuffer> _frameBuffers;
	
	// Active frame buffer index
	uint32_t _currentBuffer = 0;
	
	// Descriptor set pool
	VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
	
	// List of shader modules created (stored for cleanup)
	std::vector<VkShaderModule> _shaderModules;
	
	// Pipeline cache object
	VkPipelineCache _pipelineCache;
	
	// Wraps the swap chain to present images (framebuffers) to the windowing system
	VulkanSwapChain _swapChain;

	std::vector<VkFence> _waitFences;
public: 
	
	bool _prepared = false;
	uint32_t _width = 1280;
	uint32_t _height = 720;

	vks::UIOverlay _UIOverlay;

	/** @brief Last frame time measured using a high performance timer (if available) */
	float _frameTimer = 1.0f;
	/** @brief Returns os specific base asset path (for shaders, models, textures) */
	const std::string getAssetPath();

	vks::Benchmark _benchmark;

	/** @brief Encapsulated physical and logical vulkan device */
	vks::VulkanDevice *_vulkanDevice;

	/** @brief Example settings that can be changed e.g. by command line arguments */
	struct Settings {
		/** @brief Activates validation layers (and message output) when set to true */
		bool validation = false;
		/** @brief Set to true if fullscreen mode has been requested via command line */
		bool fullscreen = false;
		/** @brief Set to true if v-sync will be forced for the swapchain */
		bool vsync = false;
		/** @brief Enable UI overlay */
		bool overlay = false;
	} _settings;

	VkClearColorValue _defaultClearColor = { { 0.025f, 0.025f, 0.025f, 1.0f } };

	float _zoom = 0;

	static std::vector<const char*> _args;

	// Defines a frame rate independent timer value clamped from -1.0...1.0
	// For use in animations, rotations, etc.
	float _timer = 0.0f;
	
	// Multiplier for speeding up (or slowing down) the global timer
	float _timerSpeed = 0.25f;
	
	bool _paused = false;

	// Use to adjust mouse rotation speed
	float _rotationSpeed = 1.0f;
	// Use to adjust mouse zoom speed
	float _zoomSpeed = 1.0f;

	Camera _camera;

	glm::vec3 _rotation = glm::vec3();
	glm::vec3 _cameraPos = glm::vec3();
	glm::vec2 _mousePos;

	float x_center = 0.f;
	float y_center = 0.f;

	SessionTime* sessionTime;
	RobotPark* robotPark;


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

	// Vulkan requires to layout the graphics (and compute) pipeline states upfront
	VkPipeline arena_pl;

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
	// std::vector<VkFence> waitFences;

	TextOverlay* textOverlay = nullptr;





	// For per frame color animation
	float _colorParam = 1.0f;


	std::string _title = "Vulkan Example";
	std::string _name = "vulkanExample";
	uint32_t _apiVersion = VK_API_VERSION_1_0;

	typedef struct
	{
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	} depthStencil_t;
	
	
	depthStencil_t _depthStencil;


	struct {
		glm::vec2 axisLeft = glm::vec2(0.0f);
		glm::vec2 axisRight = glm::vec2(0.0f);
	} _gamePadState;

	struct {
		bool left = false;
		bool right = false;
		bool middle = false;
	} _mouseButtons;

	// OS specific 
	HWND _window;
	HINSTANCE _windowInstance;

	// Default ctor
	VulkanExampleBase(bool enableValidation);

	// dtor
	virtual ~VulkanExampleBase();

	// Setup the vulkan instance, enable required extensions and connect to the physical device (GPU)
	bool initVulkan();

	void setupConsole(std::string title);
	void setupDPIAwareness();
	HWND setupWindow(HINSTANCE hinstance, WNDPROC wndproc);
	void handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/**
	* Create the application wide Vulkan instance
	*
	* @note Virtual, can be overriden by derived example class for custom instance creation
	*/
	virtual VkResult createInstance(bool enableValidation);


	static void staticSetupRenderPass(VkDevice device, VulkanSwapChain swapChain, VkFormat depthFormat, VkRenderPass& renderPass);

	static void staticSetupDepthStencil(VkDevice device, VkFormat depthFormat, uint32_t width, uint32_t height, depthStencil_t& depthStencil, vks::VulkanDevice* vulkanDevice);

	VkPipeline arena_createPipeline(VkRenderPass rpass, VkPipelineLayout pl_layout);
	void arena_prepareVertices();
	void arena_prepareUniformBuffers();
	void arena_updateUniformBuffers();

	void updateTextOverlay();
	void prepareTextOverlay();

	void update_instanced_buffer();

	void prepareSynchronizationPrimitives();
	void buildSingleCommandBuffer(VkCommandBuffer cmdBuffer, VkFramebuffer fb);
	void buildCommandBuffers();
	void setupDescriptorPool();
	void arena_setupDescriptorSetLayout();

	void arena_setupDescriptorSet();
	void setupFrameBuffer();
	void prepare_instanced_buffer();

	VkShaderModule loadSPIRVShader(std::string filename);

	void draw();
	void render();
	// Called when view change occurs
	// Can be overriden in derived class to e.g. update uniform buffers 
	// Containing view dependant matrices
	void viewChanged();
	/** @brief (Virtual) Called after a key was pressed, can be used to do custom key handling */
	virtual void keyPressed(uint32_t);
	/** @brief (Virtual) Called after th mouse cursor moved and before internal events (like camera rotation) is handled */
	virtual void mouseMoved(double x, double y, bool &handled);
	// Called when the window has been resized
	// Can be overriden in derived class to recreate or rebuild resources attached to the frame buffer / swapchain
	virtual void windowResized();
	// Pure virtual function to be overriden by the dervice class
	// Called in case of an event where e.g. the framebuffer has to be rebuild and thus
	// all command buffers that may reference this

	//void createSynchronizationPrimitives();

	// Creates a new (graphics) command pool object storing command buffers
	void createCommandPool();
	
	
	// Create framebuffers for all requested swap chain images
	// Implement in derived class to setup a custom framebuffer (e.g. for MSAA)
	// Setup a default render pass

	/** @brief (Virtual) Called after the physical device features have been read, can be used to set features to enable on the device */
	virtual void getEnabledFeatures();

	// Connect and prepare the swap chain
	void initSwapchain();
	// Create swap chain images
	void setupSwapChain();

	// Check if command buffers are valid (!= VK_NULL_HANDLE)
	bool checkCommandBuffers();
	// Create command buffers for drawing commands
	void createCommandBuffers();

	uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);
	void createTextureImage();

	// Destroy all command buffers and set their handles to VK_NULL_HANDLE
	// May be necessary during runtime if options are toggled 
	void destroyCommandBuffers();



	VkCommandBuffer getCommandBuffer(bool begin);
	void flushCommandBuffer(VkCommandBuffer commandBuffer);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);



	// Command buffer creation
	// Creates and returns a new command buffer
	VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin);
	
	// Create a cache pool for rendering pipelines
	void createPipelineCache();

	// Prepare commonly used Vulkan functions
	void prepare();

	// Load a SPIR-V shader
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);
	
	// Start the main render loop
	void renderLoop();

	// Render one frame of a render loop on platforms that sync rendering
	void renderFrame();

	void updateOverlay();
	void drawUI(const VkCommandBuffer commandBuffer);

	// Prepare the frame for workload submission
	// - Acquires the next image from the swap chain 
	// - Sets the default wait and signal semaphores
	// void prepareFrame();

	// Submit the frames' workload 
	// void submitFrame();

	/** @brief (Virtual) Called when the UI overlay is updating, can be used to add custom elements to the overlay */
	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay);
};




