
#include "stdafx.h"

/*
* Vulkan Example base class
*
* Copyright (C) 2016-2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "triangleexamplebase.h"


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


std::vector<const char*> VulkanExampleBase::_args;

VkResult VulkanExampleBase::createInstance(bool enableValidation)
{
	this->_settings.validation = enableValidation;

	// Validation can also be forced via a define
#if defined(_VALIDATION)
	this->settings.validation = true;
#endif	

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = _name.c_str();
	appInfo.pEngineName = _name.c_str();
	appInfo.apiVersion = _apiVersion;

	std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

	// Enable surface extensions depending on os
#if defined(_WIN32)
	instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	if (_enabledInstanceExtensions.size() > 0) {
		for (auto enabledExtension : _enabledInstanceExtensions) {
			instanceExtensions.push_back(enabledExtension);
		}
	}

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = NULL;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	if (instanceExtensions.size() > 0)
	{
		if (_settings.validation)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}
		instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
	}
	if (_settings.validation)
	{
		instanceCreateInfo.enabledLayerCount = vks::debug::validationLayerCount;
		instanceCreateInfo.ppEnabledLayerNames = vks::debug::validationLayerNames;
	}
	return vkCreateInstance(&instanceCreateInfo, nullptr, &_instance);
}

std::string VulkanExampleBase::getWindowTitle()
{
	std::string device(_deviceProperties.deviceName);
	std::string windowTitle;
	windowTitle = _title + " - " + device;
	if (!_settings.overlay) {
		windowTitle += " - " + std::to_string(_frameCounter) + " fps";
	}
	return windowTitle;
}

#if !(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
// iOS & macOS: VulkanExampleBase::getAssetPath() implemented externally to allow access to Objective-C components
const std::string VulkanExampleBase::getAssetPath()
{
#if defined(VK_EXAMPLE_DATA_DIR)
	return VK_EXAMPLE_DATA_DIR;
#else
	return "./data/";
#endif
}
#endif

bool VulkanExampleBase::checkCommandBuffers()
{
	for (auto& cmdBuffer : _drawCmdBuffers)
	{
		if (cmdBuffer == VK_NULL_HANDLE)
		{
			return false;
		}
	}
	return true;
}


// This function is used to request a device memory type that supports all the property flags we request (e.g. device local, host visibile)
// Upon success it will return the index of the memory type that fits our requestes memory properties
// This is necessary as implementations can offer an arbitrary number of memory types with different
// memory properties. 
// You can check http://vulkan.gpuinfo.org/ for details on different memory configurations
uint32_t VulkanExampleBase::getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
{
	// Iterate over all memory types available for the device used in this example
	for (uint32_t i = 0; i < _deviceMemoryProperties.memoryTypeCount; i++)
	{
		if ((typeBits & 1) == 1)
		{
			if ((_deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		typeBits >>= 1;
	}

	throw "Could not find a suitable memory type!";
}

// Get a new command buffer from the command pool
  // If begin is true, the command buffer is also started so we can start adding commands
VkCommandBuffer VulkanExampleBase::getCommandBuffer(bool begin)
{
	VkCommandBuffer cmdBuffer;


	VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
	cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocateInfo.commandPool = _cmdPool;
	cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufAllocateInfo.commandBufferCount = 1;

	VK_CHECK_RESULT(vkAllocateCommandBuffers(_device, &cmdBufAllocateInfo, &cmdBuffer));

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
void VulkanExampleBase::flushCommandBuffer(VkCommandBuffer commandBuffer)
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
	VK_CHECK_RESULT(vkCreateFence(_device, &fenceCreateInfo, nullptr, &fence));

	// Submit to the queue
	VK_CHECK_RESULT(vkQueueSubmit(_queue, 1, &submitInfo, fence));

	// Wait for the fence to signal that command buffer has finished executing
	VK_CHECK_RESULT(vkWaitForFences(_device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

	vkDestroyFence(_device, fence, nullptr);
	vkFreeCommandBuffers(_device, _cmdPool, 1, &commandBuffer);
}



void VulkanExampleBase::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = getCommandBuffer(true);

	// ...

	flushCommandBuffer(commandBuffer);
}


void VulkanExampleBase::createTextureImage() {
	int texWidth, texHeight, texChannels;

	std::string s = getAssetPath() + "textures/texture.jpg";

	stbi_uc* pixels = stbi_load(s.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = uint64_t(texWidth) * uint64_t(texHeight) * 4;

	if (pixels) {
		;
	}
	else {
		throw std::runtime_error("failed to load texture image");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkResult r = _vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		imageSize,
		&stagingBuffer,
		&stagingBufferMemory);

	VK_CHECK_RESULT(r);

	void* data;
	vkMapMemory(_device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(_device, stagingBufferMemory);

	stbi_image_free(pixels);


	VkImageCreateInfo imageInfo{};

	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(texWidth);
	imageInfo.extent.height = static_cast<uint32_t>(texHeight);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;

	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;

	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional

	if (vkCreateImage(_device, &imageInfo, nullptr, &_textureImage) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(_device, _textureImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = getMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(_device, &allocInfo, nullptr, &_textureImageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(_device, _textureImage, _textureImageMemory, 0);


}


void VulkanExampleBase::createCommandBuffers()
{
	// Create one command buffer for each swap chain image and reuse for rendering
	_drawCmdBuffers.resize(_swapChain.imageCount);

	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vks::initializers::commandBufferAllocateInfo(
			_cmdPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			static_cast<uint32_t>(_drawCmdBuffers.size()));

	VK_CHECK_RESULT(vkAllocateCommandBuffers(_device, &cmdBufAllocateInfo, _drawCmdBuffers.data()));
}

void VulkanExampleBase::destroyCommandBuffers()
{
	vkFreeCommandBuffers(_device, _cmdPool, static_cast<uint32_t>(_drawCmdBuffers.size()), _drawCmdBuffers.data());
}

VkCommandBuffer VulkanExampleBase::createCommandBuffer(VkCommandBufferLevel level, bool begin)
{
	VkCommandBuffer cmdBuffer;

	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vks::initializers::commandBufferAllocateInfo(
			_cmdPool,
			level,
			1);

	VK_CHECK_RESULT(vkAllocateCommandBuffers(_device, &cmdBufAllocateInfo, &cmdBuffer));

	// If requested, also start the new command buffer
	if (begin)
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
	}

	return cmdBuffer;
}

void VulkanExampleBase::createPipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VK_CHECK_RESULT(vkCreatePipelineCache(_device, &pipelineCacheCreateInfo, nullptr, &_pipelineCache));
}

void VulkanExampleBase::prepare()
{
	if (_vulkanDevice->enableDebugMarkers) {
		vks::debugmarker::setup(_device);
	}
	initSwapchain();
	createCommandPool();
	setupSwapChain();
	createCommandBuffers();
	createTextureImage();

	// createSynchronizationPrimitives();
	setupDepthStencil();
	setupRenderPass();
	createPipelineCache();
	setupFrameBuffer();
	_settings.overlay = _settings.overlay && (!_benchmark.active);
	if (_settings.overlay) {
		_UIOverlay.device = _vulkanDevice;
		_UIOverlay.queue = _queue;
		_UIOverlay.shaders = {
			loadShader(getAssetPath() + "shaders/base/uioverlay.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(getAssetPath() + "shaders/base/uioverlay.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
		};
		_UIOverlay.prepareResources();
		_UIOverlay.preparePipeline(_pipelineCache, _renderPass);
	}
}

VkPipelineShaderStageCreateInfo VulkanExampleBase::loadShader(std::string fileName, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;

	shaderStage.module = vks::tools::loadShader(fileName.c_str(), _device);
	shaderStage.pName = "main"; // todo : make param
	assert(shaderStage.module != VK_NULL_HANDLE);
	_shaderModules.push_back(shaderStage.module);
	return shaderStage;
}

void VulkanExampleBase::renderFrame()
{
	auto tStart = std::chrono::high_resolution_clock::now();
	if (_viewUpdated)
	{
		_viewUpdated = false;
		viewChanged();
	}
	viewChanged();
	render();
	_frameCounter++;
	auto tEnd = std::chrono::high_resolution_clock::now();
	auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
	_frameTimer = (float)tDiff / 1000.0f;
	_camera.update(_frameTimer);
	if (_camera.moving())
	{
		_viewUpdated = true;
	}
	// Convert to clamped timer value
	if (!_paused)
	{
		_timer += _timerSpeed * _frameTimer;
		if (_timer > 1.0)
		{
			_timer -= 1.0f;
		}
	}
	_fpsTimer += (float)tDiff;
	if (_fpsTimer > 1000.0f)
	{
		_lastFPS = static_cast<uint32_t>((float)_frameCounter * (1000.0f / _fpsTimer));

		if (!_settings.overlay)	{
			std::string windowTitle = getWindowTitle();
			SetWindowText(_window, windowTitle.c_str());
		}
		_fpsTimer = 0.0f;
		_frameCounter = 0;
	}
	// TODO: Cap UI overlay update rates
	updateOverlay();
}

void VulkanExampleBase::renderLoop()
{
	if (_benchmark.active) {
		_benchmark.run([=] { render(); }, _vulkanDevice->properties);
		vkDeviceWaitIdle(_device);
		if (_benchmark.filename != "") {
			_benchmark.saveResults();
		}
		return;
	}

	_destWidth = _width;
	_destHeight = _height;

	MSG msg;
	bool quitMessageReceived = false;
	while (!quitMessageReceived) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				quitMessageReceived = true;
				break;
			}
		}
		if (!IsIconic(_window)) {
			renderFrame();
		}
	}

	// Flush device to make sure all resources can be freed
	if (_device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(_device);
	}
}

void VulkanExampleBase::updateOverlay()
{
	if (!_settings.overlay)
		return;

	ImGuiIO& io = ImGui::GetIO();

	io.DisplaySize = ImVec2((float)_width, (float)_height);
	io.DeltaTime = _frameTimer;

	io.MousePos = ImVec2(_mousePos.x, _mousePos.y);
	io.MouseDown[0] = _mouseButtons.left;
	io.MouseDown[1] = _mouseButtons.right;

	ImGui::NewFrame();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Vulkan Example", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::TextUnformatted(_title.c_str());
	ImGui::TextUnformatted(_deviceProperties.deviceName);
	ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / _lastFPS), _lastFPS);

	ImGui::PushItemWidth(110.0f * _UIOverlay.scale);
	OnUpdateUIOverlay(&_UIOverlay);
	ImGui::PopItemWidth();

	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::Render();

	if (_UIOverlay.update() || _UIOverlay.updated) {
		buildCommandBuffers();
		_UIOverlay.updated = false;
	}

}

void VulkanExampleBase::drawUI(const VkCommandBuffer commandBuffer)
{
	if (_settings.overlay) {
		const VkViewport viewport = vks::initializers::viewport((float)_width, (float)_height, 0.0f, 1.0f);
		const VkRect2D scissor = vks::initializers::rect2D(_width, _height, 0, 0);
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		_UIOverlay.draw(commandBuffer);
	}
}

VulkanExampleBase::VulkanExampleBase(bool enableValidation)
{
#if !defined(VK_USE_PLATFORM_ANDROID_KHR)
	// Check for a valid asset path
	struct stat info;
	if (stat(getAssetPath().c_str(), &info) != 0)
	{
#if defined(_WIN32)
		std::string msg = "Could not locate asset path in \"" + getAssetPath() + "\" !";
		MessageBox(NULL, msg.c_str(), "Fatal error", MB_OK | MB_ICONERROR);
#else
		std::cerr << "Error: Could not find asset path in " << getAssetPath() << std::endl;
#endif
		exit(-1);
	}
#endif

	_settings.validation = enableValidation;

	char* numConvPtr;

	// Parse command line arguments
	for (size_t i = 0; i < _args.size(); i++)
	{
		if (_args[i] == std::string("-validation")) {
			_settings.validation = true;
		}
		if (_args[i] == std::string("-vsync")) {
			_settings.vsync = true;
		}
		if ((_args[i] == std::string("-f")) || (_args[i] == std::string("--fullscreen"))) {
			_settings.fullscreen = true;
		}
		if ((_args[i] == std::string("-w")) || (_args[i] == std::string("-width"))) {
			uint32_t w = strtol(_args[i + 1], &numConvPtr, 10);
			if (numConvPtr != _args[i + 1]) { _width = w; };
		}
		if ((_args[i] == std::string("-h")) || (_args[i] == std::string("-height"))) {
			uint32_t h = strtol(_args[i + 1], &numConvPtr, 10);
			if (numConvPtr != _args[i + 1]) { _height = h; };
		}
		// Benchmark
		if ((_args[i] == std::string("-b")) || (_args[i] == std::string("--benchmark"))) {
			_benchmark.active = true;
			vks::tools::errorModeSilent = true;
		}
		// Warmup time (in seconds)
		if ((_args[i] == std::string("-bw")) || (_args[i] == std::string("--benchwarmup"))) {
			if (_args.size() > i + 1) {
				uint32_t num = strtol(_args[i + 1], &numConvPtr, 10);
				if (numConvPtr != _args[i + 1]) {
					_benchmark.warmup = num;
				} else {
					std::cerr << "Warmup time for benchmark mode must be specified as a number!" << std::endl;
				}
			}
		}
		// Benchmark runtime (in seconds)
		if ((_args[i] == std::string("-br")) || (_args[i] == std::string("--benchruntime"))) {
			if (_args.size() > i + 1) {
				uint32_t num = strtol(_args[i + 1], &numConvPtr, 10);
				if (numConvPtr != _args[i + 1]) {
					_benchmark.duration = num;
				}
				else {
					std::cerr << "Benchmark run duration must be specified as a number!" << std::endl;
				}
			}
		}
		// Bench result save filename (overrides default)
		if ((_args[i] == std::string("-bf")) || (_args[i] == std::string("--benchfilename"))) {
			if (_args.size() > i + 1) {
				if (_args[i + 1][0] == '-') {
					std::cerr << "Filename for benchmark results must not start with a hyphen!" << std::endl;
				} else {
					_benchmark.filename = _args[i + 1];
				}
			}
		}
		// Output frame times to benchmark result file
		if ((_args[i] == std::string("-bt")) || (_args[i] == std::string("--benchframetimes"))) {
			_benchmark.outputFrameTimes = true;
		}
	}
	
	// Enable console if validation is active
	// Debug message callback will output to it
	if (this->_settings.validation)
	{
		setupConsole("Vulkan validation output");
	}
	setupDPIAwareness();
}

VulkanExampleBase::~VulkanExampleBase()
{
	// Clean up Vulkan resources
	_swapChain.cleanup();
	if (_descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
	}
	destroyCommandBuffers();
	vkDestroyRenderPass(_device, _renderPass, nullptr);
	for (uint32_t i = 0; i < _frameBuffers.size(); i++)
	{
		vkDestroyFramebuffer(_device, _frameBuffers[i], nullptr);
	}

	for (auto& shaderModule : _shaderModules)
	{
		vkDestroyShaderModule(_device, shaderModule, nullptr);
	}
	vkDestroyImageView(_device, _depthStencil.view, nullptr);
	vkDestroyImage(_device, _depthStencil.image, nullptr);
	vkFreeMemory(_device, _depthStencil.mem, nullptr);

	vkDestroyPipelineCache(_device, _pipelineCache, nullptr);

	vkDestroyCommandPool(_device, _cmdPool, nullptr);

	//vkDestroySemaphore(_device, _semaphores.presentComplete, nullptr);
	//vkDestroySemaphore(_device, _semaphores.renderComplete, nullptr);

	/*
	for (auto& fence : _waitFences) {
		vkDestroyFence(_device, fence, nullptr);
	}
	*/

	if (_settings.overlay) {
		_UIOverlay.freeResources();
	}

	delete _vulkanDevice;

	if (_settings.validation)
	{
		vks::debug::freeDebugCallback(_instance);
	}

	vkDestroyInstance(_instance, nullptr);

}

bool VulkanExampleBase::initVulkan()
{
	VkResult err;

	// Vulkan instance
	err = createInstance(_settings.validation);
	if (err) {
		vks::tools::exitFatal("Could not create Vulkan instance : \n" + vks::tools::errorString(err), err);
		return false;
	}

	// If requested, we enable the default validation layers for debugging
	if (_settings.validation)
	{
		// The report flags determine what type of messages for the layers will be displayed
		// For validating (debugging) an appplication the error and warning bits should suffice
		VkDebugReportFlagsEXT debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		// Additional flags include performance info, loader and layer debug messages, etc.
		vks::debug::setupDebugging(_instance, debugReportFlags, VK_NULL_HANDLE);
	}

	// Physical device
	uint32_t gpuCount = 0;
	// Get number of available physical devices
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(_instance, &gpuCount, nullptr));
	assert(gpuCount > 0);
	// Enumerate devices
	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	err = vkEnumeratePhysicalDevices(_instance, &gpuCount, physicalDevices.data());
	if (err) {
		vks::tools::exitFatal("Could not enumerate physical devices : \n" + vks::tools::errorString(err), err);
		return false;
	}

	// GPU selection

	// Select physical device to be used for the Vulkan example
	// Defaults to the first device unless specified by command line
	uint32_t selectedDevice = 0;

	// GPU selection via command line argument
	for (size_t i = 0; i < _args.size(); i++)
	{
		// Select GPU
		if ((_args[i] == std::string("-g")) || (_args[i] == std::string("-gpu")))
		{
			char* endptr;
			uint32_t index = strtol(_args[i + 1], &endptr, 10);
			if (endptr != _args[i + 1]) 
			{ 
				if (index > gpuCount - 1)
				{
					std::cerr << "Selected device index " << index << " is out of range, reverting to device 0 (use -listgpus to show available Vulkan devices)" << std::endl;
				} 
				else
				{
					std::cout << "Selected Vulkan device " << index << std::endl;
					selectedDevice = index;
				}
			};
			break;
		}
		// List available GPUs
		if (_args[i] == std::string("-listgpus"))
		{
			uint32_t gpuCount = 0;
			VK_CHECK_RESULT(vkEnumeratePhysicalDevices(_instance, &gpuCount, nullptr));
			if (gpuCount == 0) 
			{
				std::cerr << "No Vulkan devices found!" << std::endl;
			}
			else 
			{
				// Enumerate devices
				std::cout << "Available Vulkan devices" << std::endl;
				std::vector<VkPhysicalDevice> devices(gpuCount);
				VK_CHECK_RESULT(vkEnumeratePhysicalDevices(_instance, &gpuCount, devices.data()));
				for (uint32_t i = 0; i < gpuCount; i++) {
					VkPhysicalDeviceProperties deviceProperties;
					vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
					std::cout << "Device [" << i << "] : " << deviceProperties.deviceName << std::endl;
					std::cout << " Type: " << vks::tools::physicalDeviceTypeString(deviceProperties.deviceType) << std::endl;
					std::cout << " API: " << (deviceProperties.apiVersion >> 22) << "." << ((deviceProperties.apiVersion >> 12) & 0x3ff) << "." << (deviceProperties.apiVersion & 0xfff) << std::endl;
				}
			}
		}
	}

	_physicalDevice = physicalDevices[selectedDevice];

	// Store properties (including limits), features and memory properties of the phyiscal device (so that examples can check against them)
	vkGetPhysicalDeviceProperties(_physicalDevice, &_deviceProperties);
	vkGetPhysicalDeviceFeatures(_physicalDevice, &_deviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &_deviceMemoryProperties);

	// Derived examples can override this to set actual features (based on above readings) to enable for logical device creation
	getEnabledFeatures();

	// Vulkan device creation
	// This is handled by a separate class that gets a logical device representation
	// and encapsulates functions related to a device
	_vulkanDevice = new vks::VulkanDevice(_physicalDevice);

	VkResult res = _vulkanDevice->createLogicalDevice(_enabledFeatures, _enabledDeviceExtensions);
	if (res != VK_SUCCESS) {
		vks::tools::exitFatal("Could not create Vulkan device: \n" + vks::tools::errorString(res), res);
		return false;
	}
	_device = _vulkanDevice->logicalDevice;

	// Get a graphics queue from the device
	vkGetDeviceQueue(_device, _vulkanDevice->queueFamilyIndices.graphics, 0, &_queue);

	// Find a suitable depth format
	VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(_physicalDevice, &_depthFormat);
	assert(validDepthFormat);

	_swapChain.connect(_instance, _physicalDevice, _device);

	// Create synchronization objects
	VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
	// Create a semaphore used to synchronize image presentation
	// Ensures that the image is displayed before we start submitting new commands to the queu
	// VK_CHECK_RESULT(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_semaphores.presentComplete));
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	// VK_CHECK_RESULT(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_semaphores.renderComplete));

	// Set up submit info structure
	// Semaphores will stay the same during application lifetime
	// Command buffer submission info is set by each example
	/*_submitInfo = vks::initializers::submitInfo();
	_submitInfo.pWaitDstStageMask = &_submitPipelineStages;
	_submitInfo.waitSemaphoreCount = 1;
	_submitInfo.pWaitSemaphores = &_semaphores.presentComplete;
	_submitInfo.signalSemaphoreCount = 1;
	_submitInfo.pSignalSemaphores = &_semaphores.renderComplete;
	*/
	return true;
}

// Win32 : Sets up a console window and redirects standard output to it
void VulkanExampleBase::setupConsole(std::string title)
{
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	FILE *stream;
	freopen_s(&stream, "CONOUT$", "w+", stdout);
	freopen_s(&stream, "CONOUT$", "w+", stderr);
	SetConsoleTitle(TEXT(title.c_str()));
}

void VulkanExampleBase::setupDPIAwareness()
{
	using SetProcessDpiAwarenessFunc = HRESULT(*)(PROCESS_DPI_AWARENESS);

	HMODULE shCore = LoadLibraryA("Shcore.dll");
	if (shCore)
	{
		SetProcessDpiAwarenessFunc setProcessDpiAwareness =
			(SetProcessDpiAwarenessFunc)GetProcAddress(shCore, "SetProcessDpiAwareness");

		if (setProcessDpiAwareness != nullptr)
		{
			setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
		}

		FreeLibrary(shCore);
	}
}

HWND VulkanExampleBase::setupWindow(HINSTANCE hinstance, WNDPROC wndproc)
{
	this->_windowInstance = hinstance;

	WNDCLASSEX wndClass;

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = wndproc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hinstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = _name.c_str();
	wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	if (!RegisterClassEx(&wndClass))
	{
		std::cout << "Could not register window class!\n";
		fflush(stdout);
		exit(1);
	}

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	if (_settings.fullscreen)
	{
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = screenWidth;
		dmScreenSettings.dmPelsHeight = screenHeight;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		if ((_width != (uint32_t)screenWidth) && (_height != (uint32_t)screenHeight))
		{
			if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			{
				if (MessageBox(NULL, "Fullscreen Mode not supported!\n Switch to window mode?", "Error", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
				{
					_settings.fullscreen = false;
				}
				else
				{
					return nullptr;
				}
			}
		}

	}

	DWORD dwExStyle;
	DWORD dwStyle;

	if (_settings.fullscreen)
	{
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}

	RECT windowRect;
	windowRect.left = 0L;
	windowRect.top = 0L;
	windowRect.right = _settings.fullscreen ? (long)screenWidth : (long)_width;
	windowRect.bottom = _settings.fullscreen ? (long)screenHeight : (long)_height;

	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

	std::string windowTitle = getWindowTitle();

	_window = CreateWindowEx(0,
		_name.c_str(),
		windowTitle.c_str(),
		dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0,
		0,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		hinstance,
		NULL);

	if (!_settings.fullscreen)
	{
		// Center on screen
		uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
		uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
		SetWindowPos(_window, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}

	if (!_window)
	{
		printf("Could not create window!\n");
		fflush(stdout);
		return nullptr;
		exit(1);
	}

	ShowWindow(_window, SW_SHOW);
	SetForegroundWindow(_window);
	SetFocus(_window);

	return _window;
}

void VulkanExampleBase::handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		_prepared = false;
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		ValidateRect(_window, NULL);
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case KEY_P:
			_paused = !_paused;
			break;
		case KEY_F1:
			if (_settings.overlay) {
				_UIOverlay.visible = !_UIOverlay.visible;
			}
			break;
		case KEY_ESCAPE:
			PostQuitMessage(0);
			break;
		}

		if (_camera.firstperson)
		{
			switch (wParam)
			{
			case KEY_W:
				_camera.keys.up = true;
				break;
			case KEY_S:
				_camera.keys.down = true;
				break;
			case KEY_A:
				_camera.keys.left = true;
				break;
			case KEY_D:
				_camera.keys.right = true;
				break;
			}
		}

		keyPressed((uint32_t)wParam);
		break;
	case WM_KEYUP:
		if (_camera.firstperson)
		{
			switch (wParam)
			{
			case KEY_W:
				_camera.keys.up = false;
				break;
			case KEY_S:
				_camera.keys.down = false;
				break;
			case KEY_A:
				_camera.keys.left = false;
				break;
			case KEY_D:
				_camera.keys.right = false;
				break;
			}
		}
		break;
	case WM_LBUTTONDOWN:
		_mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		_mouseButtons.left = true;
		break;
	case WM_RBUTTONDOWN:
		_mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		_mouseButtons.right = true;
		break;
	case WM_MBUTTONDOWN:
		_mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		_mouseButtons.middle = true;
		break;
	case WM_LBUTTONUP:
		_mouseButtons.left = false;
		break;
	case WM_RBUTTONUP:
		_mouseButtons.right = false;
		break;
	case WM_MBUTTONUP:
		_mouseButtons.middle = false;
		break;
	case WM_MOUSEWHEEL:
	{
		short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		_zoom += (float)wheelDelta * 0.005f * _zoomSpeed;
		_camera.translate(glm::vec3(0.0f, 0.0f, (float)wheelDelta * 0.005f * _zoomSpeed));
		_viewUpdated = true;
		break;
	}
	case WM_MOUSEMOVE:
	{
		handleMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;
	}
	case WM_SIZE:
		if ((_prepared) && (wParam != SIZE_MINIMIZED))
		{
			if ((_resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)))
			{
				_destWidth = LOWORD(lParam);
				_destHeight = HIWORD(lParam);
				windowResize();
			}
		}
		break;
	case WM_ENTERSIZEMOVE:
		_resizing = true;
		break;
	case WM_EXITSIZEMOVE:
		_resizing = false;
		break;
	}
}

void VulkanExampleBase::viewChanged() {}

void VulkanExampleBase::keyPressed(uint32_t) {}

void VulkanExampleBase::mouseMoved(double x, double y, bool & handled) {}

void VulkanExampleBase::buildCommandBuffers() {}

void VulkanExampleBase::createCommandPool()
{
	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = _swapChain.queueNodeIndex;
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_CHECK_RESULT(vkCreateCommandPool(_device, &cmdPoolInfo, nullptr, &_cmdPool));
}

void VulkanExampleBase::setupDepthStencil()
{
	VkImageCreateInfo image = {};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.pNext = NULL;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = _depthFormat;
	image.extent = { _width, _height, 1 };
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	image.flags = 0;

	VkMemoryAllocateInfo mem_alloc = {};
	mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc.pNext = NULL;
	mem_alloc.allocationSize = 0;
	mem_alloc.memoryTypeIndex = 0;

	VkImageViewCreateInfo depthStencilView = {};
	depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthStencilView.pNext = NULL;
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = _depthFormat;
	depthStencilView.flags = 0;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;

	VkMemoryRequirements memReqs;

	VK_CHECK_RESULT(vkCreateImage(_device, &image, nullptr, &_depthStencil.image));
	vkGetImageMemoryRequirements(_device, _depthStencil.image, &memReqs);
	mem_alloc.allocationSize = memReqs.size;
	mem_alloc.memoryTypeIndex = _vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(_device, &mem_alloc, nullptr, &_depthStencil.mem));
	VK_CHECK_RESULT(vkBindImageMemory(_device, _depthStencil.image, _depthStencil.mem, 0));

	depthStencilView.image = _depthStencil.image;
	VK_CHECK_RESULT(vkCreateImageView(_device, &depthStencilView, nullptr, &_depthStencil.view));
}


void VulkanExampleBase::getEnabledFeatures()
{
	// Can be overriden in derived class
}

void VulkanExampleBase::windowResize()
{
	if (!_prepared)
	{
		return;
	}
	_prepared = false;

	// Ensure all operations on the device have been finished before destroying resources
	vkDeviceWaitIdle(_device);

	// Recreate swap chain
	_width = _destWidth;
	_height = _destHeight;

	setupSwapChain();

	// Recreate the frame buffers
	vkDestroyImageView(_device, _depthStencil.view, nullptr);
	vkDestroyImage(_device, _depthStencil.image, nullptr);
	vkFreeMemory(_device, _depthStencil.mem, nullptr);
	setupDepthStencil();	
	for (uint32_t i = 0; i < _frameBuffers.size(); i++) {
		vkDestroyFramebuffer(_device, _frameBuffers[i], nullptr);
	}
	setupFrameBuffer();

	if ((_width > 0.0f) && (_height > 0.0f)) {
		if (_settings.overlay) {
			_UIOverlay.resize(_width, _height);
		}
	}

	// Command buffers need to be recreated as they may store
	// references to the recreated frame buffer
	destroyCommandBuffers();
	createCommandBuffers();
	buildCommandBuffers();

	vkDeviceWaitIdle(_device);

	if ((_width > 0.0f) && (_height > 0.0f)) {
		_camera.updateAspectRatio((float)_width / (float)_height);
	}

	// Notify derived class
	windowResized();
	viewChanged();

	_prepared = true;
}

void VulkanExampleBase::handleMouseMove(int32_t x, int32_t y)
{
	int32_t dx = (int32_t)_mousePos.x - x;
	int32_t dy = (int32_t)_mousePos.y - y;

	bool handled = false;

	if (_settings.overlay) {
		ImGuiIO& io = ImGui::GetIO();
		handled = io.WantCaptureMouse;
	}
	mouseMoved((float)x, (float)y, handled);

	if (handled) {
		_mousePos = glm::vec2((float)x, (float)y);
		return;
	}

	if (_mouseButtons.left) {
		_rotation.x += dy * 1.25f * _rotationSpeed;
		_rotation.y -= dx * 1.25f * _rotationSpeed;
		_camera.rotate(glm::vec3(dy * _camera.rotationSpeed, -dx * _camera.rotationSpeed, 0.0f));
		_viewUpdated = true;
	}
	if (_mouseButtons.right) {
		_zoom += dy * .005f * _zoomSpeed;
		_camera.translate(glm::vec3(-0.0f, 0.0f, dy * .005f * _zoomSpeed));
		_viewUpdated = true;
	}
	if (_mouseButtons.middle) {
		_cameraPos.x -= dx * 0.01f;
		_cameraPos.y -= dy * 0.01f;
		_camera.translate(glm::vec3(-dx * 0.01f, -dy * 0.01f, 0.0f));
		_viewUpdated = true;
	}
	_mousePos = glm::vec2((float)x, (float)y);
}

void VulkanExampleBase::windowResized()
{
	// Can be overriden in derived class
}

void VulkanExampleBase::initSwapchain()
{
	_swapChain.initSurface(_windowInstance, _window);
}

void VulkanExampleBase::setupSwapChain()
{
	_swapChain.create(&_width, &_height, _settings.vsync);
}

void VulkanExampleBase::OnUpdateUIOverlay(vks::UIOverlay *overlay) {}