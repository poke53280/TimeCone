
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



#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION true

#define E_PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062


#include <math.h>

#include "ArenaCubes.h"

#include "stb_font_consolas_24_latin1.inl"


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


// Vulkan loads it's shaders from an immediate binary representation called SPIR-V
	// Shaders are compiled offline from e.g. GLSL using the reference glslang compiler
	// This function loads such a shader from a binary file and returns a shader module structure
VkShaderModule VulkanExampleBase::loadSPIRVShader(std::string filename)
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
		VK_CHECK_RESULT(vkCreateShaderModule(_device, &moduleCreateInfo, NULL, &shaderModule));

		delete[] shaderCode;

		return shaderModule;
	}
	else
	{
		std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
		return VK_NULL_HANDLE;
	}
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

    staticSetupDepthStencil(_device, _depthFormat, _width, _height, _depthStencil, _vulkanDevice);
    staticSetupRenderPass(_device, _swapChain, _depthFormat, _renderPass);
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
	prepareSynchronizationPrimitives();
	arena_prepareVertices();
	prepare_instanced_buffer();
	arena_prepareUniformBuffers();
	arena_setupDescriptorSetLayout();

	// Renderpass this pipeline is attached to
	// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
	arena_pl = arena_createPipeline(_renderPass, arena_pipelineLayout);
	setupDescriptorPool();
	arena_setupDescriptorSet();
	buildCommandBuffers();
	prepareTextOverlay();
	_prepared = true;

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

	sessionTime = new SessionTime();

	uint32_t t0 = sessionTime->getTimeMS();

	robotPark = new RobotPark(1000, t0);
	_zoom = -125.0f;
	_title = "THE GAME";
	// Values not set here are initialized in the base class constructor
	_settings.overlay = false;


}

VulkanExampleBase::~VulkanExampleBase()
{

	// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

		// Clean up texture resources

	vkDestroyPipeline(_device, arena_pl, nullptr);

	vkDestroyPipelineLayout(_device, arena_pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(_device, arena_descriptorSetLayout, nullptr);

	vkDestroyBuffer(_device, arena_vertices.buffer, nullptr);
	vkFreeMemory(_device, arena_vertices.memory, nullptr);

	vkDestroyBuffer(_device, arena_indices.buffer, nullptr);
	vkFreeMemory(_device, arena_indices.memory, nullptr);

	vkDestroyBuffer(_device, arena_uniformBufferVS.buffer, nullptr);
	vkFreeMemory(_device, arena_uniformBufferVS.memory, nullptr);

	vkDestroyBuffer(_device, arena_instance_data.buffer, nullptr);
	vkFreeMemory(_device, arena_instance_data.memory, nullptr);

	if (textOverlay != nullptr)
	{
		delete(textOverlay);
		textOverlay = nullptr;
	}

	vkDestroySemaphore(_device, presentCompleteSemaphore, nullptr);
	vkDestroySemaphore(_device, renderCompleteSemaphore, nullptr);

	for (auto& fence : _waitFences)
	{
		vkDestroyFence(_device, fence, nullptr);
	}

	delete robotPark;
	robotPark = nullptr;

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

// Update the text buffer displayed by the text overlay
void VulkanExampleBase::updateTextOverlay(void)
{
	textOverlay->beginTextUpdate();

	textOverlay->addText(_title, 5.0f, 5.0f, TextOverlay::alignLeft);

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

	textOverlay->addText(_title, 5.0f, 5.0f, TextOverlay::alignLeft);

	std::stringstream ss;
	ss << std::fixed << std::setprecision(2) << (_frameTimer * 1000.0f) << "ms (" << _lastFPS << " fps) " << _deviceProperties.deviceName << " rotation " << rAngle << " nRay " << nRay;
	textOverlay->addText(ss.str(), 5.0f, 25.0f, TextOverlay::alignLeft);

	// Display current model view matrix
	textOverlay->addText("model view matrix", (float)_width, 5.0f, TextOverlay::alignRight);


	for (uint32_t i = 0; i < 4; i++)
	{
		ss.str("");
		ss << std::fixed << std::setprecision(2) << std::showpos;
		ss << arena_uboVS.modelMatrix[0][i] << " " << arena_uboVS.modelMatrix[1][i] << " " << arena_uboVS.modelMatrix[2][i] << " " << arena_uboVS.modelMatrix[3][i];
		textOverlay->addText(ss.str(), (float)_width, 25.0f + (float)i * 20.0f, TextOverlay::alignRight);
	}

	glm::vec3 projected = glm::project(glm::vec3(34.6281052f, 20.0473022f, 11.0037498f), arena_uboVS.modelMatrix, arena_uboVS.projectionMatrix, glm::vec4(0, 0, (float)_width, (float)_height));

	// textOverlay->addText("IRAY7.I5", projected.x, projected.y, TextOverlay::alignCenter);
	textOverlay->addText("[X]", projected.x, projected.y, TextOverlay::alignCenter);

	textOverlay->addText("Info...", 5.0f, 65.0f, TextOverlay::alignLeft);
	textOverlay->addText("---", 5.0f, 85.0f, TextOverlay::alignLeft);

	textOverlay->endTextUpdate();
}


void VulkanExampleBase::prepareTextOverlay()
{
	// Load the text rendering shaders
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	shaderStages.push_back(loadShader(getAssetPath() + "shaders/textoverlay/text.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
	shaderStages.push_back(loadShader(getAssetPath() + "shaders/textoverlay/text.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));

	textOverlay = new TextOverlay(
		_vulkanDevice,
		_queue,
		_frameBuffers,
		_swapChain.colorFormat,
		_depthFormat,
		&_width,
		&_height,
		shaderStages
	);
	updateTextOverlay();
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


void VulkanExampleBase::keyPressed(uint32_t) {}

void VulkanExampleBase::mouseMoved(double x, double y, bool & handled) {}


void VulkanExampleBase::createCommandPool()
{
	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = _swapChain.queueNodeIndex;
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_CHECK_RESULT(vkCreateCommandPool(_device, &cmdPoolInfo, nullptr, &_cmdPool));
}

void VulkanExampleBase::staticSetupDepthStencil(VkDevice device, VkFormat depthFormat, uint32_t width, uint32_t height, depthStencil_t& depthStencil, vks::VulkanDevice* vulkanDevice)
{
	VkImageCreateInfo image = {};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.pNext = NULL;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = depthFormat;
	image.extent = { width, height, 1 };
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
	depthStencilView.format = depthFormat;
	depthStencilView.flags = 0;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;

	VkMemoryRequirements memReqs;

	VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &depthStencil.image));
	vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);
	mem_alloc.allocationSize = memReqs.size;
    mem_alloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device, &mem_alloc, nullptr, &depthStencil.mem));
	VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.mem, 0));

	depthStencilView.image = depthStencil.image;
	VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &depthStencil.view));
}



// Prepare vertex and index buffers for indexed triangles
// Also uploads them to device local memory using staging and initializes vertex input and attribute binding to match the vertex shader
void VulkanExampleBase::arena_prepareVertices()
{

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
	// StagingBuffer instance_stagingBuffer;

	//  Vertex buffer

	// Create a host-visible buffer to copy the vertex data to (staging buffer)


	VK_CHECK_RESULT(_vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vertexBufferSize,
		&vertices_stagingBuffer.buffer,
		&vertices_stagingBuffer.memory,
		lcVertex.data()));


	// Create a device local buffer to which the (host local) vertex data will be copied (below) and which will be used for rendering

	VK_CHECK_RESULT(_vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBufferSize,
		&arena_vertices.buffer,
		&arena_vertices.memory));


	// Index buffer

	// Copy index data to a buffer visible to the host (staging buffer)

	VK_CHECK_RESULT(_vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		indexBufferSize,
		&indices_stagingBuffer.buffer,
		&indices_stagingBuffer.memory,
		lcIndex.data()));

	// Create destination buffer with device only visibility


	VK_CHECK_RESULT(_vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBufferSize,
		&arena_indices.buffer,
		&arena_indices.memory));



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
		//copyRegion.size = instanceBufferSize;
		//vkCmdCopyBuffer(copyCmd, instance_stagingBuffer.buffer, arena_instance_data.buffer, 1, &copyRegion);

		// Flushing the command buffer will also submit it to the queue and uses a fence to ensure that all commands have been executed before returning
		flushCommandBuffer(copyCmd);
	}

	// Destroy staging buffers
	// Note: Staging buffer must not be deleted before the copies have been submitted and executed
	vkDestroyBuffer(_device, vertices_stagingBuffer.buffer, nullptr);
	vkFreeMemory(_device, vertices_stagingBuffer.memory, nullptr);
	vkDestroyBuffer(_device, indices_stagingBuffer.buffer, nullptr);
	vkFreeMemory(_device, indices_stagingBuffer.memory, nullptr);

	//vkDestroyBuffer(device, instance_stagingBuffer.buffer, nullptr);
	//vkFreeMemory(device, instance_stagingBuffer.memory, nullptr);


}


void VulkanExampleBase::arena_updateUniformBuffers()
{
	// Update matrices
	arena_uboVS.projectionMatrix = glm::perspective(glm::radians(35.0f), (float)_width / (float)_height, 0.1f, 4096.0f);

	arena_uboVS.modelMatrix = glm::mat4(1.0f);


	if (_camera.keys.up) {
		y_center -= 0.01f;
	}

	if (_camera.keys.down) {
		y_center += 0.01f;
	}

	if (_camera.keys.left) {
		x_center -= 0.01f;
	}

	if (_camera.keys.right) {
		x_center += 0.01f;
	}



	arena_uboVS.viewMatrix = glm::lookAt(glm::vec3(x_center, y_center, 150), glm::vec3(x_center, y_center, 0), glm::vec3(0, 1, 0));


	float ms = float(sessionTime->getTimeMS());

	// Set color params
	arena_uboVS.colorParams = glm::vec4(ms, 0, 0, 0);


	// Map uniform buffer and update it

	uint8_t* pData;

	VK_CHECK_RESULT(vkMapMemory(_device, arena_uniformBufferVS.memory, 0, sizeof(arena_uboVS), 0, (void**)&pData));

	memcpy(pData, &arena_uboVS, sizeof(arena_uboVS));

	// Unmap after data has been copied
	// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU

	vkUnmapMemory(_device, arena_uniformBufferVS.memory);
}


void VulkanExampleBase::arena_prepareUniformBuffers()
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
	VK_CHECK_RESULT(vkCreateBuffer(_device, &bufferInfo, nullptr, &arena_uniformBufferVS.buffer));
	// Get memory requirements including size, alignment and memory type 
	vkGetBufferMemoryRequirements(_device, arena_uniformBufferVS.buffer, &memReqs);
	allocInfo.allocationSize = memReqs.size;
	// Get the memory type index that supports host visibile memory access
	// Most implementations offer multiple memory types and selecting the correct one to allocate memory from is crucial
	// We also want the buffer to be host coherent so we don't have to flush (or sync after every update.
	// Note: This may affect performance so you might not want to do this in a real world application that updates buffers on a regular base
	allocInfo.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	// Allocate memory for the uniform buffer
	VK_CHECK_RESULT(vkAllocateMemory(_device, &allocInfo, nullptr, &(arena_uniformBufferVS.memory)));
	// Bind memory to buffer
	VK_CHECK_RESULT(vkBindBufferMemory(_device, arena_uniformBufferVS.buffer, arena_uniformBufferVS.memory, 0));

	// Store information in the uniform's descriptor that is used by the descriptor set
	arena_uniformBufferVS.descriptor.buffer = arena_uniformBufferVS.buffer;
	arena_uniformBufferVS.descriptor.offset = 0;
	arena_uniformBufferVS.descriptor.range = sizeof(arena_uboVS);

	arena_updateUniformBuffers();
}



VkPipeline VulkanExampleBase::arena_createPipeline(VkRenderPass rpass, VkPipelineLayout pl_layout)
{
	// Create a graphics pipeline

	// A big setup ending in one vk call: vkCreateGraphicsPipelines

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	pipelineCreateInfo.layout = pl_layout;
	pipelineCreateInfo.renderPass = rpass;

	// Construct the different states making up the pipeline

	// Input assembly state describes how primitives are assembled
	// This pipeline will assemble vertex data as a triangle list 
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

	//
	// In number of fragments. GPU support.
	// 

	// lineWidthGranularity is the granularity of supported line widths. 
	// Not all line widths in the range defined by lineWidthRange are supported.
	// This limit specifies the granularity(or increment) between successive supported line widths.
	//
	// Note: Limited line width support on devices.
	//

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
	pipelineCreateInfo.pDynamicState = &dynamicState;

	// Create rendering pipeline using the specified states

	VkPipeline p;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(_device, _pipelineCache, 1, &pipelineCreateInfo, nullptr, &p));

	// Shader modules are no longer needed once the graphics pipeline has been created
	vkDestroyShaderModule(_device, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(_device, shaderStages[1].module, nullptr);

	return p;
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
    staticSetupDepthStencil(_device, _depthFormat, _width, _height, _depthStencil, _vulkanDevice);
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

void VulkanExampleBase::draw()
{
	// Get next image in the swap chain (back/front buffer)
	VK_CHECK_RESULT(_swapChain.acquireNextImage(presentCompleteSemaphore, &_currentBuffer));

	// Use a fence to wait until the command buffer has finished execution before using it again
	VK_CHECK_RESULT(vkWaitForFences(_device, 1, &_waitFences[_currentBuffer], VK_TRUE, UINT64_MAX));
	VK_CHECK_RESULT(vkResetFences(_device, 1, &_waitFences[_currentBuffer]));

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
	submitInfo.pCommandBuffers = &_drawCmdBuffers[_currentBuffer];					// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;												// One command buffer




	if (textOverlay->_visible)
	{
		VK_CHECK_RESULT(vkQueueSubmit(_queue, 1, &submitInfo, VK_NULL_HANDLE));

		VkSubmitInfo submitInfo_text = {};
		submitInfo_text.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo_text.commandBufferCount = 1;
		submitInfo_text.pCommandBuffers = &textOverlay->_cmdBuffers[_currentBuffer];

		VK_CHECK_RESULT(vkQueueSubmit(_queue, 1, &submitInfo_text, _waitFences[_currentBuffer]));
		VK_CHECK_RESULT(vkQueueWaitIdle(_queue));

	}
	else
	{
		// Submit to the graphics queue passing a wait fence
		VK_CHECK_RESULT(vkQueueSubmit(_queue, 1, &submitInfo, _waitFences[_currentBuffer]));
	}



	// Present the current buffer to the swap chain
	// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
	// This ensures that the image is not presented to the windowing system until all commands have been submitted
	VK_CHECK_RESULT(_swapChain.queuePresent(_queue, _currentBuffer, renderCompleteSemaphore));
}


void VulkanExampleBase::render()
{
	if (!_prepared)
		return;

	uint32_t ms = sessionTime->getTimeMS();

	robotPark->advance(ms);

	draw();
}

// A single render pass is set up with vkCreateRenderPass.
void VulkanExampleBase::staticSetupRenderPass(VkDevice device, VulkanSwapChain swapChain, VkFormat depthFormat, VkRenderPass& renderPass)
{
    // Descriptors for the attachments used by this renderpass
    std::array<VkAttachmentDescription, 2> attachmentDescriptions = {};

    // Color attachment
    VkAttachmentDescription a;

    a.format = swapChain.colorFormat;									// Use the color format selected by the swapchain
    a.samples = VK_SAMPLE_COUNT_1_BIT;									// We don't use multi sampling in this example
    a.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							    // Clear this attachment at the start of the render pass
    a.storeOp = VK_ATTACHMENT_STORE_OP_STORE;							// Keep it's contents after the render pass is finished (for displaying it)
    a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// We don't use stencil, so don't care for load
    a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// Same for store
    a.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;						// Layout at render pass start. Initial doesn't matter, so we use undefined
    a.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;					// Layout to which the attachment is transitioned when the render pass is finished

    attachmentDescriptions[0] = a;
    // As we want to present the color buffer to the swapchain, we transition to PRESENT_KHR	
// Depth attachment
    a.format = depthFormat;											// A proper depth format is selected in the example base
    a.samples = VK_SAMPLE_COUNT_1_BIT;
    a.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							    // Clear depth at start of first subpass
    a.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;						// We don't need depth after render pass has finished (DONT_CARE may result in better performance)
    a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// No stencil
    a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// No Stencil
    a.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;						// Layout at render pass start. Initial doesn't matter, so we use undefined
    a.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;	// Transition to depth/stencil attachment

    attachmentDescriptions[1] = a;

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
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());		// Number of attachments used by this render pass
    renderPassInfo.pAttachments = attachmentDescriptions.data();								// Descriptions of the attachments used by the render pass
    renderPassInfo.subpassCount = 1;												// We only use one subpass in this example
    renderPassInfo.pSubpasses = &subpassDescription;								// Description of that subpass
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());	// Number of subpass dependencies
    renderPassInfo.pDependencies = dependencies.data();								// Subpass dependencies used by the render pass

    VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

void VulkanExampleBase::viewChanged()
{
	// This function is called by the base example class each time the view is changed by user input
	arena_updateUniformBuffers();

	uint32_t ms = sessionTime->getTimeMS();

	if (ms % 115 == 0) {
		update_instanced_buffer();
	}
	updateTextOverlay();
}

void VulkanExampleBase::update_instanced_buffer() {

	std::vector<instance_data> lcInstance;

	robotPark->get_instance_data(lcInstance);

	uint32_t instanceBufferSize = static_cast<uint32_t>(lcInstance.size()) * sizeof(instance_data);

	uint8_t* pData;

	VK_CHECK_RESULT(vkMapMemory(_device, arena_instance_data.memory, 0, instanceBufferSize, 0, (void**)&pData));

	memcpy(pData, lcInstance.data(), instanceBufferSize);

	// Unmap after data has been copied
	vkUnmapMemory(_device, arena_instance_data.memory);
}


// Create the Vulkan synchronization primitives used in this example
void VulkanExampleBase::prepareSynchronizationPrimitives()
{
	// Semaphores (Used for correct command ordering)
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;

	// Semaphore used to ensures that image presentation is complete before starting to submit again
	VK_CHECK_RESULT(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &presentCompleteSemaphore));

	// Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
	VK_CHECK_RESULT(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &renderCompleteSemaphore));

	// Fences (Used to check draw command buffer completion)
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	// Create in signaled state so we don't wait on first render of each command buffer
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	_waitFences.resize(_drawCmdBuffers.size());
	for (auto& fence : _waitFences)
	{
		VK_CHECK_RESULT(vkCreateFence(_device, &fenceCreateInfo, nullptr, &fence));
	}
}


// Command buffer

void VulkanExampleBase::buildSingleCommandBuffer(VkCommandBuffer cmdBuffer, VkFramebuffer fb) {

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
	renderPassBeginInfo.renderPass = _renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = _width;
	renderPassBeginInfo.renderArea.extent.height = _height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = fb;

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

	// Start the first sub pass specified in our default render pass setup by the base class
	// This will clear the color and depth attachment
	vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Update dynamic viewport state
	VkViewport viewport = {};
	viewport.height = (float)_height;
	viewport.width = (float)_width;
	viewport.minDepth = (float)0.0f;
	viewport.maxDepth = (float)1.0f;
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	// Update dynamic scissor state
	VkRect2D scissor = {};
	scissor.extent.width = _width;
	scissor.extent.height = _height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	// Bind descriptor sets describing shader binding points

	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, arena_pipelineLayout, 0, 1, &arena_descriptorSet, 0, nullptr);


	// Bind the rendering pipeline
	// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, arena_pl);

	// Bind triangle vertex buffer (contains position and colors)
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &arena_vertices.buffer, offsets);

	// Bind instance data
	vkCmdBindVertexBuffers(cmdBuffer, 1, 1, &arena_instance_data.buffer, offsets);

	// Bind triangle index buffer
	vkCmdBindIndexBuffer(cmdBuffer, arena_indices.buffer, 0, VK_INDEX_TYPE_UINT32);

	// Draw indexed triangle
	vkCmdDrawIndexed(cmdBuffer, arena_indices.count, robotPark->instances(), 0, 0, 0);

	vkCmdEndRenderPass(cmdBuffer);

	// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to 
	// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

	VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

}

// Build separate command buffers for every framebuffer image

void VulkanExampleBase::buildCommandBuffers()
{

	for (uint32_t iCmdBuffer = 0; iCmdBuffer < _drawCmdBuffers.size(); ++iCmdBuffer)
	{

		VkCommandBuffer cmdBuffer = _drawCmdBuffers[iCmdBuffer];
		VkFramebuffer fb = _frameBuffers[iCmdBuffer];

		buildSingleCommandBuffer(cmdBuffer, fb);
	}
}


void VulkanExampleBase::setupDescriptorPool()
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

	VK_CHECK_RESULT(vkCreateDescriptorPool(_device, &descriptorPoolInfo, nullptr, &_descriptorPool));
}

void VulkanExampleBase::arena_setupDescriptorSetLayout()
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

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(_device, &descriptorLayout, nullptr, &arena_descriptorSetLayout));

	// Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
	// In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
	pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.pNext = nullptr;
	pPipelineLayoutCreateInfo.setLayoutCount = 1;
	pPipelineLayoutCreateInfo.pSetLayouts = &arena_descriptorSetLayout;

	VK_CHECK_RESULT(vkCreatePipelineLayout(_device, &pPipelineLayoutCreateInfo, nullptr, &arena_pipelineLayout));
}

void VulkanExampleBase::arena_setupDescriptorSet()
{
	// Allocate a new descriptor set from the global descriptor pool
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &arena_descriptorSetLayout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(_device, &allocInfo, &arena_descriptorSet));

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

	vkUpdateDescriptorSets(_device, 1, &writeDescriptorSet, 0, nullptr);
}

// Create a frame buffer for each swap chain image
// Note: Implementation of pure virtual function in the base class and called from within VulkanExampleBase::prepare
void VulkanExampleBase::setupFrameBuffer()
{
	// Create a frame buffer for every image in the swapchain
	_frameBuffers.resize(_swapChain.imageCount);
	for (size_t i = 0; i < _frameBuffers.size(); i++)
	{
		std::array<VkImageView, 2> attachments;
		attachments[0] = _swapChain.buffers[i].view;									// Color attachment is the view of the swapchain image			
		attachments[1] = _depthStencil.view;											// Depth/Stencil attachment is the same for all frame buffers			

		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		// All frame buffers use the same renderpass setup
		frameBufferCreateInfo.renderPass = _renderPass;
		frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		frameBufferCreateInfo.pAttachments = attachments.data();
		frameBufferCreateInfo.width = _width;
		frameBufferCreateInfo.height = _height;
		frameBufferCreateInfo.layers = 1;
		// Create the framebuffer
		VK_CHECK_RESULT(vkCreateFramebuffer(_device, &frameBufferCreateInfo, nullptr, &_frameBuffers[i]));
	}
}

// Render pass 
// They describe the attachments used during rendering.
// They may contain multiple subpasses with attachment dependencies.

// This allows the driver to know up-front what the rendering will look like and is a good opportunity to
// optimize especially on tile-based renderers (with multiple subpasses)

// Using sub pass dependencies also adds implicit layout transitions for the attachment used,
// so we don't need to add explicit image memory barriers to transform them

// Called from within VulkanExampleBase::prepare






void VulkanExampleBase::prepare_instanced_buffer() {

	std::vector<instance_data> lcInstance;

	robotPark->get_instance_data(lcInstance);

	uint32_t instanceBufferSize = static_cast<uint32_t>(lcInstance.size()) * sizeof(instance_data);

	VkMemoryRequirements memReqs;

	// Vertex shader uniform buffer block
	VkBufferCreateInfo bufferInfo = {};
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.allocationSize = 0;
	allocInfo.memoryTypeIndex = 0;

	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = instanceBufferSize;

	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	// Create a new buffer
	VK_CHECK_RESULT(vkCreateBuffer(_device, &bufferInfo, nullptr, &arena_instance_data.buffer));

	vkGetBufferMemoryRequirements(_device, arena_instance_data.buffer, &memReqs);
	allocInfo.allocationSize = memReqs.size;


	allocInfo.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	// Allocate memory for the uniform buffer
	VK_CHECK_RESULT(vkAllocateMemory(_device, &allocInfo, nullptr, &(arena_instance_data.memory)));
	// Bind memory to buffer
	VK_CHECK_RESULT(vkBindBufferMemory(_device, arena_instance_data.buffer, arena_instance_data.memory, 0));

	update_instanced_buffer();

}



VulkanExampleBase* vulkanExample;

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

    for (size_t i = 0; i < __argc; i++) { VulkanExampleBase::_args.push_back(__argv[i]); };
    vulkanExample = new VulkanExampleBase(ENABLE_VALIDATION);

    vulkanExample->initVulkan();
    vulkanExample->setupWindow(hInstance, WndProc);
    vulkanExample->prepare();
    vulkanExample->renderLoop();
    delete(vulkanExample);
    return 0;
}




