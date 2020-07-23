#include "main.h"

constexpr unsigned int screenWidth = 1920;
constexpr unsigned int screenHeight = 1080;

using std::cout;
using std::cerr;
using std::endl;

class appvk {
public:
	void run() {
		init();
		loop();
		cleanup();
	}

private:
	const bool verbose = false;
	const bool verify = true;

	GLFWwindow* w;
	
	void initwindow() {
		glfwInit();
		
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // turn off window resizing until we register callbacks later
		w = glfwCreateWindow(screenWidth, screenHeight, "Demo", nullptr, nullptr);
		if (!w) {
			throw std::runtime_error("cannot create window!");
		}
	}

	const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	
	void checkValidation() {
		if (verify) {
			uint32_t numLayers = 0;
			vkEnumerateInstanceLayerProperties(&numLayers, nullptr);
			std::vector<VkLayerProperties> layers(numLayers);
			vkEnumerateInstanceLayerProperties(&numLayers, layers.data());
			
			for (const auto& validationLayer : validationLayers) {
				bool found = false;
				for (const auto& currentLayer : layers) {
					if (!strcmp(validationLayer, currentLayer.layerName)) {
						found = true;
					}
				}
				if (!found) {
					throw std::runtime_error("can't find all validation layers!");
				}
			}
			cout << "found all validation layers." << endl;
		}
	}

	const std::vector<const char*> getExtensions() {
		// glfw helper function that specifies the extension needed to draw stuff
		uint32_t glfwNumExtensions = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwNumExtensions);
		
		// TODO: is this a range constructor?
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwNumExtensions);
		if (verify) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	VkInstance instance = VK_NULL_HANDLE;

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT mSev,
		VkDebugUtilsMessageTypeFlagsEXT mType,
		const VkDebugUtilsMessengerCallbackDataEXT* data,
		void* userData) {
		
		cerr << " ~ " << data->pMessage << endl;
		return VK_FALSE; // don't abort on error in callback
	}
	
	// load extensions for creating and destroying debug messengers ourselves
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
		auto f = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (f) {
			return f(instance, pCreateInfo, pAllocator, pDebugMessenger);
		} else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
		auto f = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (f) {
			f(instance, debugMessenger, pAllocator);
		}
	}

	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
	
	void populateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		
		createInfo.messageSeverity = //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
									 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
									 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;
	}

	void setupDebugMessenger() {
		if (!verify) {
			return;
		}
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		populateDebugMessenger(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("cannot create debug messenger!");
		}
	}

	void createInstance() {
		checkValidation();

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Demo";
		appInfo.apiVersion = VK_API_VERSION_1_2;
		
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{}; // outside if statement to avoid getting deallocated early
		if (verify) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
			
			populateDebugMessenger(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
		}
		
		auto extensions = getExtensions();

		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkResult r = vkCreateInstance(&createInfo, nullptr, &instance);
		if (r != VK_SUCCESS) {
			throw std::runtime_error("instance creation failed!");
		}
	}

	VkSurfaceKHR surf = VK_NULL_HANDLE;

	void createSurface() {
		// platform-agnostic version of vulkan create surface extension
		if (glfwCreateWindowSurface(instance, w, nullptr, &surf) != VK_SUCCESS) {
			throw std::runtime_error("cannot create window surface!");
		}
	}
	
	VkPhysicalDevice pdev = VK_NULL_HANDLE;
	
	bool checkDevice(VkPhysicalDevice pd, bool perf) {
		VkPhysicalDeviceProperties dprop{}; // basic information
		VkPhysicalDeviceFeatures dfeat{}; // detailed feature list
		
		vkGetPhysicalDeviceProperties(pd, &dprop);
		vkGetPhysicalDeviceFeatures(pd, &dfeat);
		
		VkBool32 goodEnough;

		if (perf) { // get NVIDIA
			goodEnough = std::string(dprop.deviceName).find("GeForce") != std::string::npos;
		} else { // if GS and TS exist, gpu is probably fine
			goodEnough = dfeat.geometryShader && dfeat.tessellationShader;
		}
		
		if (goodEnough) {
			cout << "Vulkan Version: " << VK_VERSION_MAJOR(dprop.apiVersion) << "." << VK_VERSION_MINOR(dprop.apiVersion) << "." << VK_VERSION_PATCH(dprop.apiVersion) << endl;
			cout << "Hardware: " << dprop.deviceName << endl;
			cout << "Max Texture Memory: " << dprop.limits.maxImageDimension2D << " bytes" << endl;
			cout << "GS and TS present: " << ((goodEnough) ? "yes" : "no") << endl << endl;
		}

		return goodEnough;
	}

	void pickPhysicalDevice(bool perf) {
		uint32_t numDevices;
		vkEnumeratePhysicalDevices(instance, &numDevices, nullptr);
		if (numDevices == 0) {
			throw std::runtime_error("no vulkan-capable gpu found!");
		}
		std::vector<VkPhysicalDevice> devices(numDevices);
		vkEnumeratePhysicalDevices(instance, &numDevices, devices.data());
		
		for (const auto& device : devices) {
			if (checkDevice(device, perf)) {
				pdev = device;
				break;
			}
		}

		if (pdev == VK_NULL_HANDLE) {
			throw std::runtime_error("no usable graphics device found!");
		}
	}
	
	std::vector<const char*> requiredExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	// swapchain support is device-specific, so check for it here
	void checkDeviceExtensions(VkPhysicalDevice pdev) {
		uint32_t numExtensions;
		vkEnumerateDeviceExtensionProperties(pdev, nullptr, &numExtensions, nullptr);
		std::vector<VkExtensionProperties> deviceExtensions(numExtensions);
		vkEnumerateDeviceExtensionProperties(pdev, nullptr, &numExtensions, deviceExtensions.data());
		
		std::set<std::string> tempExtensionList(requiredExtensions.begin(), requiredExtensions.end());
		
		// erase any extensions found
		for (const auto& extension : deviceExtensions) {
			tempExtensionList.erase(extension.extensionName);
		}

		if (!tempExtensionList.empty()) {
			throw std::runtime_error("cannot find a device with all extensions!");
		}
	}

	struct queueAvailable {
		std::optional<uint32_t> graphics;
		std::optional<uint32_t> compute;
		std::optional<uint32_t> transfer;
	};
	
	queueAvailable findQueueFamily(VkPhysicalDevice pd) {
		queueAvailable qa;
		uint32_t numQueues;
		vkGetPhysicalDeviceQueueFamilyProperties(pd, &numQueues, nullptr);
		std::vector<VkQueueFamilyProperties> queues(numQueues);
		vkGetPhysicalDeviceQueueFamilyProperties(pd, &numQueues, queues.data());
		
		for (size_t i = 0; i < numQueues; i++) {
			VkBool32 presSupported = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(pdev, i, surf, &presSupported);
			
			if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presSupported) {
				qa.graphics = i;
			}
			if (queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
				qa.compute = i;
			}
			if (queues[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
				qa.transfer = i;
			}
			
			if (verbose) {
				cout << "Queue family " << i << " has " << (qa.graphics.has_value() ? "graphics " : "") << (qa.compute.has_value() ? "compute " : "") << (qa.transfer.has_value() ? "transfer " : "") << endl;
			}
		}
		
		return qa;
	}

	struct swapChainSupportDetails {
		VkSurfaceCapabilitiesKHR cap;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formatList) {
		for (const auto& format : formatList) {
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return format;
			}
		}

		return formatList[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modeList) {
		for (const auto& mode : modeList) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR) { // triple buffer if possible
				return mode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& cap) {
		if (cap.currentExtent.width != UINT32_MAX) {
			return cap.currentExtent; // if the surface has a width and height already, use that.
		} else {
			VkExtent2D newV{};
			// clamp width and height to [min, max] extent height
			newV.width = std::max(cap.minImageExtent.width, std::min(cap.maxImageExtent.width, screenWidth));
			newV.height = std::max(cap.minImageExtent.height, std::min(cap.minImageExtent.height, screenHeight));
			return newV;
		}
	}

	swapChainSupportDetails querySwapChainSupport(VkPhysicalDevice pdev) {
		swapChainSupportDetails d;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, surf, &d.cap);

		uint32_t numFormats;
		vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surf, &numFormats, nullptr);
		
		if (numFormats > 0) {
			d.formats.resize(numFormats);
			vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surf, &numFormats, d.formats.data());
		}

		uint32_t numPresentModes;
		vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surf, &numPresentModes, nullptr);
		
		if (numPresentModes > 0) {
			d.presentModes.resize(numPresentModes);
			vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surf, &numPresentModes, d.presentModes.data());
		}
		
		return d;
	}
	
	VkDevice dev = VK_NULL_HANDLE;
	VkQueue gQueue = VK_NULL_HANDLE;
	
	void createLogicalDevice() {
		queueAvailable qa = findQueueFamily(pdev); // check for the proper queue
		checkDeviceExtensions(pdev); // check for vk_khr_swapchain at least
		swapChainSupportDetails d = querySwapChainSupport(pdev); // get swap chain information

		if (!qa.graphics.has_value() || d.formats.size() == 0 || d.presentModes.size() == 0) {
			throw std::runtime_error("cannot find a suitable logical device!");
		}

		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = *(qa.graphics);
		queueInfo.queueCount = 1;
		float pri = 1.0f;
		queueInfo.pQueuePriorities = &pri; // highest priority
		
		VkPhysicalDeviceFeatures feat{}; // not using anything fancy right now, so set to false
		
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = &queueInfo;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pEnabledFeatures = &feat;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();
		
		// enabledLayerCount and ppEnabledLayerNames are no longer used by drivers in vk 1.1
		
		if (vkCreateDevice(pdev, &createInfo, nullptr, &dev)) {
			throw std::runtime_error("cannot create virtual device!");
		}

		vkGetDeviceQueue(dev, *(qa.graphics), 0, &gQueue);
	}
	
	VkSwapchainKHR swap = VK_NULL_HANDLE;
	std::vector<VkImage> swapImages;
	VkFormat swapFormat;
	VkExtent2D swapExtent;

	void createSwapChain() {
		swapChainSupportDetails sdet = querySwapChainSupport(pdev);

		VkSurfaceFormatKHR f = chooseSwapSurfaceFormat(sdet.formats);
		VkPresentModeKHR p = chooseSwapPresentMode(sdet.presentModes);
		VkExtent2D e = chooseSwapExtent(sdet.cap);
		
		uint32_t numImages = sdet.cap.minImageCount + 1; // perf improvement - don't have to wait for the driver to complete stuff to continue rendering
		numImages = std::max(numImages, sdet.cap.maxImageCount);

		VkSwapchainCreateInfoKHR sInfo{};
		sInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

		sInfo.surface = surf;
		sInfo.minImageCount = numImages;
		sInfo.imageFormat = f.format;
		sInfo.imageExtent = e;
		sInfo.imageColorSpace = f.colorSpace;
		sInfo.imageArrayLayers = 1; // no stereoscopic viewing rn
		sInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		sInfo.presentMode = p;
		sInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		// if the presentation and graphics queues are different, then both have to access the swapchain and we have to set that behavior here.
		sInfo.preTransform = sdet.cap.currentTransform;
		sInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // no alpha
		sInfo.clipped = VK_TRUE; // don't render pixels covered by another window
		sInfo.oldSwapchain = VK_NULL_HANDLE; // no previous swapchain exists rn

		if (vkCreateSwapchainKHR(dev, &sInfo, nullptr, &swap) != VK_SUCCESS) {
			throw std::runtime_error("unable to create swapchain!");
		}
		
		uint32_t imageCount;
		vkGetSwapchainImagesKHR(dev, swap, &imageCount, nullptr);
		swapImages.resize(imageCount);
		vkGetSwapchainImagesKHR(dev, swap, &imageCount, swapImages.data());
		
		swapFormat = f.format;
		swapExtent = e;
	}

	std::vector<VkImageView> swapImageViews;
	
	void createImageViews() {
		swapImageViews.resize(swapImages.size());
		for (size_t i = 0; i < swapImages.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapFormat;
			
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // or r, g, b, a
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;
			
			if (vkCreateImageView(dev, &createInfo, nullptr, &swapImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("cannot create image views!");
			}
		}
	}
	
	VkRenderPass renderPass = VK_NULL_HANDLE;
	
	// stores framebuffer config
	void createRenderPass() {
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 1 sample per pixel
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // layout of image before render pass - don't care since we'll be clearing it anyways
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // layout of image at end of render pass

		VkAttachmentReference colorAttachmentRef{}; // for each render pass, subpasses exist
		colorAttachmentRef.attachment = 0; // which attachment we're talking about in pAttachments
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // use a color attachment for our only subpass
		
		VkSubpassDescription sub{};
		sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		sub.colorAttachmentCount = 1; // color attachments are FS outputs, can also specify input / depth attachments, etc.
		sub.pColorAttachments = &colorAttachmentRef;
		
		VkSubpassDependency sdep{}; // there's a dependency between reading an image in and writing to it, so describe it here
		sdep.srcSubpass = VK_SUBPASS_EXTERNAL; // implicit subpass at start of render pass
		sdep.dstSubpass = 0; // index into pSubpasses

		sdep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // stage we're waiting on
		sdep.srcAccessMask = 0; // what we're using that input for

		sdep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // stage we write to
		sdep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // what we're using that output for

		VkRenderPassCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &colorAttachment;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &sub;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &sdep;

		if (vkCreateRenderPass(dev, &createInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("cannot create render pass!");
		}
	}

	static std::vector<char> readFile(const std::string& path) {
		std::ifstream file(path, std::ios::ate | std::ios::binary);

		if (!file) {
			throw std::runtime_error("cannot open file " + path + "!");
		}
		
		size_t len = static_cast<size_t>(file.tellg());
		file.seekg(0);
		
		std::vector<char> contents(len); // using vector of char because spir-v isn't ascii
		file.read(contents.data(), len);
		
		file.close();

		return contents;
	}

	VkShaderModule createShaderModule(const std::vector<char>& spv) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = spv.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(spv.data());

		VkShaderModule mod;
		if (vkCreateShaderModule(dev, &createInfo, nullptr, &mod) != VK_SUCCESS) {
			throw std::runtime_error("cannot create shader module!");
		}
		return mod;
	}
	
	VkPipelineLayout pipeLayout = VK_NULL_HANDLE;
	VkPipeline gpipe = VK_NULL_HANDLE;

	void createGraphicsPipeline() {
		auto vertspv = readFile("shader/vert.spv");
		auto fragspv = readFile("shader/frag.spv");

		VkShaderModule vmod = createShaderModule(vertspv);
		VkShaderModule fmod = createShaderModule(fragspv);
		
		VkPipelineShaderStageCreateInfo vCreateInfo{};
		vCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vCreateInfo.module = vmod;
		vCreateInfo.pName = "main";
		
		VkPipelineShaderStageCreateInfo fCreateInfo{};
		fCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fCreateInfo.module = fmod;
		fCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaders[] = {vCreateInfo, fCreateInfo}; // create all shaders in one go
		
		VkPipelineVertexInputStateCreateInfo vinCreateInfo{};
		vinCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vinCreateInfo.vertexBindingDescriptionCount = 0;
		vinCreateInfo.vertexAttributeDescriptionCount = 0; // no loading of vertex data for now

		VkPipelineInputAssemblyStateCreateInfo inAsmCreateInfo{};
		inAsmCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inAsmCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inAsmCreateInfo.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = swapExtent.width;
		viewport.height = swapExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapExtent;

		VkPipelineViewportStateCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewCreateInfo.viewportCount = 1;
		viewCreateInfo.pViewports = &viewport;
		viewCreateInfo.scissorCount = 1;
		viewCreateInfo.pScissors = &scissor;
		
		VkPipelineRasterizationStateCreateInfo rasterCreateInfo{};
		rasterCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterCreateInfo.depthClampEnable = VK_FALSE; // clamps depth to range instead of discarding it
		rasterCreateInfo.rasterizerDiscardEnable = VK_FALSE; // disables rasterization if true
		rasterCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterCreateInfo.cullMode = VK_CULL_MODE_NONE; // no culling for now
		rasterCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterCreateInfo.depthBiasEnable = VK_FALSE;
		rasterCreateInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo msCreateInfo{};
		msCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		msCreateInfo.sampleShadingEnable = VK_FALSE;
		msCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		
		// TODO: we'll eventually create a depth buffer here

		VkPipelineColorBlendAttachmentState colorAttachment{}; // blending information per fb
		colorAttachment.blendEnable = VK_FALSE;
		colorAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
										 VK_COLOR_COMPONENT_G_BIT | 
										 VK_COLOR_COMPONENT_B_BIT |
										 VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo colorCreateInfo{};
		colorCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorCreateInfo.logicOpEnable = VK_FALSE;
		colorCreateInfo.attachmentCount = 1;
		colorCreateInfo.pAttachments = &colorAttachment;

		VkDynamicState dynamStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH,
		};
		
		VkPipelineDynamicStateCreateInfo dynCreateInfo{};
		dynCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynCreateInfo.dynamicStateCount = 2;
		dynCreateInfo.pDynamicStates = dynamStates;
		
		VkPipelineLayoutCreateInfo pipeLayoutCreateInfo{}; // uniform creation struct
		pipeLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		if (vkCreatePipelineLayout(dev, &pipeLayoutCreateInfo, nullptr, &pipeLayout) != VK_SUCCESS) {
			throw std::runtime_error("cannot create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipeCreateInfo{};
		pipeCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeCreateInfo.stageCount = 2;
		pipeCreateInfo.pStages = shaders;
		pipeCreateInfo.pVertexInputState = &vinCreateInfo;
		pipeCreateInfo.pInputAssemblyState = &inAsmCreateInfo;
		pipeCreateInfo.pViewportState = &viewCreateInfo;
		pipeCreateInfo.pRasterizationState = &rasterCreateInfo;
		pipeCreateInfo.pMultisampleState = &msCreateInfo;
		pipeCreateInfo.pColorBlendState = &colorCreateInfo;
		pipeCreateInfo.layout = pipeLayout; // handle, not a struct.
		pipeCreateInfo.renderPass = renderPass;
		pipeCreateInfo.subpass = 0;
		
		// not using dynamic or depth stencil state
		
		if (vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &pipeCreateInfo, nullptr, &gpipe) != VK_SUCCESS) {
			throw std::runtime_error("cannot create graphics pipeline!");
		}

		vkDestroyShaderModule(dev, vmod, nullptr); // we can destroy shader modules once the graphics pipeline is created.
		vkDestroyShaderModule(dev, fmod, nullptr);
	}

	std::vector<VkFramebuffer> swapFramebuffers; // ties render attachments to image views in the swapchain

	void createFramebuffers() {
		swapFramebuffers.resize(swapImageViews.size());

		for (size_t i = 0; i < swapFramebuffers.size(); i++) {
			VkImageView attachments[] = {
				swapImageViews[i]
			};

			VkFramebufferCreateInfo fCreateInfo{};
			fCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fCreateInfo.renderPass = renderPass;
			fCreateInfo.attachmentCount = 1;
			fCreateInfo.pAttachments = attachments; // framebuffer attaches to the image view of a swapchain
			fCreateInfo.width = swapExtent.width;
			fCreateInfo.height = swapExtent.height;
			fCreateInfo.layers = 1;

			if (vkCreateFramebuffer(dev, &fCreateInfo, nullptr, &swapFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("cannot create framebuffer!");
			}
		}
	}

	VkCommandPool cp = VK_NULL_HANDLE;

	void createCommandPool() {
		queueAvailable qa = findQueueFamily(pdev);
		
		VkCommandPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.queueFamilyIndex = qa.graphics.value();

		if (vkCreateCommandPool(dev, &createInfo, nullptr, &cp) != VK_SUCCESS) {
			throw std::runtime_error("cannot create command pool!");
		}
	}
	
	std::vector<VkCommandBuffer> commandBuffers;
	
	// need to create a command buffer per swapchain image
	void allocCommandBuffers() {
		commandBuffers.resize(swapFramebuffers.size());
		
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = cp;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // can't be called from other command buffers, but can be submitted directly to queue
		allocInfo.commandBufferCount = commandBuffers.size();
		
		// we don't actually create command buffers, we create a pool and allocate em.
		if (vkAllocateCommandBuffers(dev, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("cannot create command buffers!");
		}

		for (size_t i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("cannot begin recording command buffers!");
			}

			VkRenderPassBeginInfo rBeginInfo{};
			rBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rBeginInfo.renderPass = renderPass;
			rBeginInfo.framebuffer = swapFramebuffers[i];
			rBeginInfo.renderArea.offset = { 0, 0 };
			rBeginInfo.renderArea.extent = swapExtent;
			
			VkClearValue clear = { { { 0.0, 0.0, 0.0, 1.0 } } }; // union initialization...
			
			rBeginInfo.clearValueCount = 1;
			rBeginInfo.pClearValues = &clear;
			
			// actually render stuff!
			vkCmdBeginRenderPass(commandBuffers[i], &rBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, gpipe);
			vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
			vkCmdEndRenderPass(commandBuffers[i]);
			
			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("cannot record into command buffer!");
			}
		}
	}

	constexpr static unsigned int framesInFlight = 2;

	std::vector<VkSemaphore> imageAvailSems; // use seperate semaphores per frame so we can send >1 frame at once
	std::vector<VkSemaphore> renderDoneSems;
	std::vector<VkFence> inFlightFences; // use fences so we actually wait until a frame completes before moving on to the next one
	std::vector<VkFence> imagesInFlight; // track frames in flight because acquireNextImageKHR may not return swapchain indices in order or framesInFlight may be high

	void createSyncs() {
		imageAvailSems.resize(framesInFlight, VK_NULL_HANDLE);
		renderDoneSems.resize(framesInFlight, VK_NULL_HANDLE);
		inFlightFences.resize(framesInFlight, VK_NULL_HANDLE);
		imagesInFlight.resize(swapImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		
		VkFenceCreateInfo fCreateInfo{};
		fCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		for (unsigned int i = 0; i < framesInFlight; i++) {
			VkResult r1 = vkCreateSemaphore(dev, &createInfo, nullptr, &imageAvailSems[i]);
			VkResult r2 = vkCreateSemaphore(dev, &createInfo, nullptr, &renderDoneSems[i]);
			VkResult r3 = vkCreateFence(dev, &fCreateInfo, nullptr, &inFlightFences[i]);
			
			if (r1 != VK_SUCCESS || r2 != VK_SUCCESS || r3 != VK_SUCCESS) {
				throw std::runtime_error("cannot create sync objects!");
			}
		}
	}

	void init() {
		initwindow();
		createInstance();
		if (verify) {
			setupDebugMessenger();
		}
		createSurface();
		pickPhysicalDevice(true);
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		allocCommandBuffers();
		createSyncs();
	}
	
	size_t currFrame = 0;

	void drawFrame() {
		uint32_t index;
		vkAcquireNextImageKHR(dev, swap, UINT64_MAX, imageAvailSems[currFrame], VK_NULL_HANDLE, &index);
		
		if (imagesInFlight[index] != VK_NULL_HANDLE) {
			vkWaitForFences(dev, 1, &imagesInFlight[index], VK_TRUE, UINT64_MAX); // if a previous image is using a fence, wait on it
		}

		imagesInFlight[index] = inFlightFences[currFrame]; // this frame is using the fence at currFrame

		VkSubmitInfo si{};
		si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		
		VkSemaphore renderBeginSems[] = { imageAvailSems[currFrame] };
		VkSemaphore renderEndSems[] = { renderDoneSems[currFrame] };

		si.waitSemaphoreCount = 1;
		si.pWaitSemaphores = renderBeginSems;

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // imageAvailSem waits at this point in the pipeline
		si.pWaitDstStageMask = waitStages;
		
		si.commandBufferCount = 1;
		si.pCommandBuffers = &commandBuffers[index];

		si.signalSemaphoreCount = 1;
		si.pSignalSemaphores = renderEndSems;
		
		vkResetFences(dev, 1, &inFlightFences[currFrame]); // has to be unsignaled for vkQueueSubmit
		if (vkQueueSubmit(gQueue, 1, &si, inFlightFences[currFrame]) != VK_SUCCESS) {
			throw std::runtime_error("cannot submit to queue!");
		}

		VkPresentInfoKHR pInfo{};
		pInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		pInfo.waitSemaphoreCount = 1;
		pInfo.pWaitSemaphores = renderEndSems;

		VkSwapchainKHR swaps[] = { swap };
		pInfo.swapchainCount = 1;
		pInfo.pSwapchains = swaps;
		pInfo.pImageIndices = &index;
		// don't need to check return value if we're only using one swapchain
		vkQueuePresentKHR(gQueue, &pInfo);
		//vkQueueWaitIdle(gQueue); // wait for queue to drain before submitting more work
		
		vkWaitForFences(dev, 1, &inFlightFences[currFrame], VK_TRUE, UINT64_MAX);
		currFrame = (currFrame + 1) % framesInFlight;
	}

	void loop() {
		while (!glfwWindowShouldClose(w)) {
			glfwPollEvents();
			drawFrame();
			
			if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
				vkDeviceWaitIdle(dev);
				glfwSetWindowShouldClose(w, GLFW_TRUE);
			}
		}
	}

	void cleanup() {
		
		for (unsigned int i = 0; i < framesInFlight; i++){
			vkDestroySemaphore(dev, imageAvailSems[i], nullptr);
			vkDestroySemaphore(dev, renderDoneSems[i], nullptr);
			vkDestroyFence(dev, inFlightFences[i], nullptr);
		}
		
		vkDestroyCommandPool(dev, cp, nullptr);

		for (auto framebuffer : swapFramebuffers) {
			vkDestroyFramebuffer(dev, framebuffer, nullptr);
		}

		vkDestroyPipeline(dev, gpipe, nullptr);
		vkDestroyPipelineLayout(dev, pipeLayout, nullptr);
		vkDestroyRenderPass(dev, renderPass, nullptr);

		for (const auto& view : swapImageViews) {
			vkDestroyImageView(dev, view, nullptr);
		}
		vkDestroySwapchainKHR(dev, swap, nullptr);
		
		vkDestroyDevice(dev, nullptr);
		vkDestroySurfaceKHR(instance, surf, nullptr);

		if (verify) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(w);
		glfwTerminate();
	}
};

int main(int argc, char **argv) {
	appvk app;
	try {
		app.run();
	} catch (const std::exception& e) {
		cerr << "Exception thrown: " << e.what() << endl;
		return 1;
	}
}
