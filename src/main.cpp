#include "main.h"

#define STB_IMAGE_IMPLEMENTATION
// take out decoders we don't use to save space
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_FAILURE_STRINGS
#include "stb_image.h"

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

	constexpr static unsigned int screenWidth = 3840;
	constexpr static unsigned int screenHeight = 2160;

	const bool verbose = false;
	const bool verify = true;
	
	GLFWwindow* w;
	
	bool resizeOccurred = false;
	
	static void windowSizeCallback(GLFWwindow* w, int width, int height) {
		appvk* papp = reinterpret_cast<appvk*>(glfwGetWindowUserPointer(w));
		papp->resizeOccurred = true;
	}

	void createWindow() {
		glfwInit();
		
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		w = glfwCreateWindow(screenWidth, screenHeight, "Demo", nullptr, nullptr);
		if (!w) {
			throw std::runtime_error("cannot create window!");
		}

		// this enables us to read/write class members from our static callback
		// (since static members have no "this" pointer)
		glfwSetWindowUserPointer(w, this); 
		glfwSetFramebufferSizeCallback(w, windowSizeCallback);
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
			cout << "found all validation layers" << endl;
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

	static VKAPI_ATTR VkBool32 debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT mSev,
		VkDebugUtilsMessageTypeFlagsEXT mType,
		const VkDebugUtilsMessengerCallbackDataEXT* data,
		void* userData) {
		
		cerr << " ~ " << data->pMessage << endl;
		return VK_FALSE; // don't abort on error in callback
	}
	
	// create extension methods for creating and destroying debug messengers ourselves since extensions aren't loaded by default
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
		
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
									 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		
		if (verbose) {
			createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		}

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

			// passing debugCreateInfo here so that our debug utils handle errors in createInstance or destroyInstance
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

	// want extended dynamic state to control wireframes
	const std::vector<const char*> requiredExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	// extension support is device-specific, so check for it here
	bool checkDeviceExtensions(VkPhysicalDevice pdev) {
		uint32_t numExtensions;
		vkEnumerateDeviceExtensionProperties(pdev, nullptr, &numExtensions, nullptr);
		std::vector<VkExtensionProperties> deviceExtensions(numExtensions);
		vkEnumerateDeviceExtensionProperties(pdev, nullptr, &numExtensions, deviceExtensions.data());
		
		std::set<std::string> tempExtensionList(requiredExtensions.begin(), requiredExtensions.end());
		
		// erase any extensions found
		for (const auto& extension : deviceExtensions) {
			tempExtensionList.erase(extension.extensionName);
		}

		return tempExtensionList.empty();
	}

	enum manufacturer { nvidia, intel, any };
	
	bool checkDevice(VkPhysicalDevice pd, manufacturer m) {
		VkPhysicalDeviceProperties dprop{}; // basic information
		VkPhysicalDeviceFeatures dfeat{}; // detailed feature list
		
		vkGetPhysicalDeviceProperties(pd, &dprop);
		vkGetPhysicalDeviceFeatures(pd, &dfeat);

		const std::string_view name = std::string_view(dprop.deviceName);
		cout << "found " << name << " running vulkan version " << VK_VERSION_MAJOR(dprop.apiVersion) << "." << VK_VERSION_MINOR(dprop.apiVersion) << "." << VK_VERSION_PATCH(dprop.apiVersion) << endl;
		
		VkBool32 goodEnough;
		switch (m) {
			case nvidia:
				goodEnough = name.find("GeForce") != std::string_view::npos;
				break;
			case intel:
				goodEnough = name.find("Intel") != std::string_view::npos;
				break;
			case any:
				// if the gpu has a geometry and tessellation shader, it's good enough
				goodEnough = true;
		}

		goodEnough &= dfeat.geometryShader |
					  dfeat.tessellationShader |
					  checkDeviceExtensions(pd);

		return goodEnough;
	}

	void pickPhysicalDevice(manufacturer m) {
		uint32_t numDevices;
		vkEnumeratePhysicalDevices(instance, &numDevices, nullptr);
		if (numDevices == 0) {
			throw std::runtime_error("no vulkan-capable gpu found!");
		}
		std::vector<VkPhysicalDevice> devices(numDevices);
		vkEnumeratePhysicalDevices(instance, &numDevices, devices.data());
		
		for (const auto& device : devices) {
			if (checkDevice(device, m) && pdev == VK_NULL_HANDLE) {
				pdev = device;
			}
		}

		if (pdev == VK_NULL_HANDLE) {
			throw std::runtime_error("no usable graphics device found!");
		}

		const std::string shortNames[3] = {"nvidia", "intel", "unknown"};
		cout << "using " << shortNames[m] << " gpu" << endl;
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
			
			glfwGetFramebufferSize(w, (int*)&screenWidth, (int*)&screenHeight);
			
			newV.width = std::max(cap.minImageExtent.width, std::min(cap.maxImageExtent.width, static_cast<uint32_t>(screenWidth)));
			newV.height = std::max(cap.minImageExtent.height, std::min(cap.minImageExtent.height, static_cast<uint32_t>(screenHeight)));
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
		swapChainSupportDetails d = querySwapChainSupport(pdev); // verify swap chain information before creating a new logical device

		if (!qa.graphics.has_value() || d.formats.size() == 0 || d.presentModes.size() == 0) {
			throw std::runtime_error("cannot find a suitable logical device!");
		}

		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = *(qa.graphics);
		queueInfo.queueCount = 1;
		float pri = 1.0f;
		queueInfo.pQueuePriorities = &pri; // highest priority
		
		VkPhysicalDeviceFeatures feat{};
		feat.samplerAnisotropy = VK_TRUE;
		
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = &queueInfo;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pEnabledFeatures = &feat;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();
		
		// enabledLayerCount and ppEnabledLayerNames are deprecated
		
		if (vkCreateDevice(pdev, &createInfo, nullptr, &dev)) {
			throw std::runtime_error("cannot create virtual device!");
		}

		vkGetDeviceQueue(dev, *(qa.graphics), 0, &gQueue); // creating a device also creates queues for it
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
		sInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // no alpha compositing
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

	VkImageView createImageView(VkImage im, VkFormat format, VkImageAspectFlags aspectMask) {
		VkImageViewCreateInfo createInfo{};

		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = im;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = format;
		
		VkComponentMapping map;
		map.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		map.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		map.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		map.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.components = map;

		VkImageSubresourceRange range;
		range.aspectMask = aspectMask;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		createInfo.subresourceRange = range;

		VkImageView view = VK_NULL_HANDLE;
		if (vkCreateImageView(dev, &createInfo, nullptr, &view) != VK_SUCCESS) {
			throw std::runtime_error("cannot create image view!");
		}

		return view;
	}

	std::vector<VkImageView> swapImageViews;
	
	void createSwapViews() {
		swapImageViews.resize(swapImages.size());
		for (size_t i = 0; i < swapImages.size(); i++) {
			swapImageViews[i] = createImageView(swapImages[i], swapFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}
	
	VkRenderPass renderPass = VK_NULL_HANDLE;

	VkFormat depthFormat;

	VkFormat findFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (auto format : formats) {
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(pdev, format, &formatProps);
			if (tiling == VK_IMAGE_TILING_LINEAR && (formatProps.linearTilingFeatures & features)) {
				return format;
			} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (formatProps.optimalTilingFeatures & features)) {
				return format;
			}
		}

		return VK_FORMAT_UNDEFINED;
	}

	// stores framebuffer config
	void createRenderPass() {
		VkAttachmentDescription attachments[2] = {};
		attachments[0].format = swapFormat; // format from swapchain image
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // layout of image before render pass - don't care since we'll be clearing it anyways
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // layout of image at end of render pass

		std::vector<VkFormat> formatList = {
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_X8_D24_UNORM_PACK32, // no stencil
		};

		// check to see if we can use a 24-bit depth component
		depthFormat = findFormat(formatList, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

		attachments[1].format = depthFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // depth has to be cleared to something
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // don't care about this since we clear anyways, like above
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0; // which attachment we're talking about in pAttachments
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // layout to transition to at the start of the subpass
		
		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription sub{};
		sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		sub.colorAttachmentCount = 1; // color attachments are FS outputs, can also specify input / depth attachments, etc.
		sub.pColorAttachments = &colorAttachmentRef;
		sub.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency sdep{}; // there's a dependency between reading an image in and writing to it due to where imageAvailSems waits
		// solution here is to delay writing to the framebuffer until the image we need is acquired (and the transition has taken place)
		sdep.srcSubpass = VK_SUBPASS_EXTERNAL; // implicit subpass at start of render pass
		sdep.dstSubpass = 0; // index into pSubpasses

		sdep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // stage we're waiting on
		sdep.srcAccessMask = 0; // what we're using that input for

		sdep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // stage we write to
		sdep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // what we're using that output for

		VkRenderPassCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 2;
		createInfo.pAttachments = attachments;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &sub;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &sdep;

		if (vkCreateRenderPass(dev, &createInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("cannot create render pass!");
		}
	}

	struct ubo {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformMemories;

	void createUniformBuffers() {
		VkDeviceSize bufferSize = sizeof(ubo);
		uniformBuffers.resize(swapImages.size());
		uniformMemories.resize(swapImages.size());

		for (size_t i = 0; i < swapImages.size(); i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniformBuffers[i], uniformMemories[i]);
		}
	}

	VkDescriptorSetLayout dSetLayout = VK_NULL_HANDLE;

	void createDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding bindings[2] = {};

		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		bindings[1].binding = 1;
		// images and samplers can actually be bound separately!
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[1].descriptorCount = 1;
		bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = 2;
		createInfo.pBindings = bindings;

		if (vkCreateDescriptorSetLayout(dev, &createInfo, nullptr, &dSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("cannot create descriptor set!");
		}
	}

	VkDescriptorPool dPool = VK_NULL_HANDLE;

	void createDescriptorPool() {
		VkDescriptorPoolSize poolSizes[2];
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = swapImages.size();

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = swapImages.size();

		VkDescriptorPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.maxSets = swapImages.size();
		createInfo.poolSizeCount = 2;
		createInfo.pPoolSizes = poolSizes;

		if (vkCreateDescriptorPool(dev, &createInfo, nullptr, &dPool) != VK_SUCCESS) {
			throw std::runtime_error("cannot create descriptor pool!");
		}
	}

	std::vector<VkDescriptorSet> dSet;

	void allocDescriptorSets() {
		std::vector<VkDescriptorSetLayout> dLayout(swapImages.size(), dSetLayout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = dPool;
		allocInfo.descriptorSetCount = swapImages.size();
		allocInfo.pSetLayouts = dLayout.data();
		
		dSet.resize(swapImages.size());
		if (vkAllocateDescriptorSets(dev, &allocInfo, dSet.data()) != VK_SUCCESS) {
			throw std::runtime_error("cannot create descriptor set!");
		}

		for (size_t i = 0; i < swapImages.size(); i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(ubo);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.sampler = texSamp;
			imageInfo.imageView = texView;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet sets[2] = {};
			sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			sets[0].dstSet = dSet[i];
			sets[0].dstBinding = 0;
			sets[0].dstArrayElement = 0;
			sets[0].descriptorCount = 1;
			sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			sets[0].pBufferInfo = &bufferInfo;

			sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			sets[1].dstSet = dSet[i];
			sets[1].dstBinding = 1;
			sets[1].dstArrayElement = 0;
			sets[1].descriptorCount = 1;
			sets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			sets[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(dev, 2, sets, 0, nullptr);
		}
	}

	static std::vector<char> readFile(const std::string& path) {
		std::ifstream file(path, std::ios::ate | std::ios::binary);

		if (!file) {
			throw std::runtime_error("cannot open file " + path + "!");
		}
		
		size_t len = static_cast<size_t>(file.tellg());
		file.seekg(0);
		
		std::vector<char> contents(len); // using vector of char because spir-v isn't utf-8
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
		
		VkVertexInputBindingDescription bindDesc;
		bindDesc.binding = 0;
		bindDesc.stride = sizeof(vload::vertex);
		bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription attrDesc[4];
		for (size_t i = 0; i < 4; i++) {
			attrDesc[i].binding = 0;
			attrDesc[i].location = i;
			attrDesc[i].offset = 16 * i; // all offsets are rounded up to 16 bytes due to alignas
		}

		attrDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDesc[2].format = VK_FORMAT_R32G32_SFLOAT;
		attrDesc[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		
		VkPipelineVertexInputStateCreateInfo vinCreateInfo{};
		vinCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vinCreateInfo.vertexBindingDescriptionCount = 1;
		vinCreateInfo.pVertexBindingDescriptions = &bindDesc;
		vinCreateInfo.vertexAttributeDescriptionCount = 4;
		vinCreateInfo.pVertexAttributeDescriptions = attrDesc;

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
		rasterCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterCreateInfo.depthBiasEnable = VK_FALSE;
		rasterCreateInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo msCreateInfo{};
		msCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		msCreateInfo.sampleShadingEnable = VK_FALSE;
		msCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo dCreateInfo{};
		dCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		dCreateInfo.depthTestEnable = VK_TRUE;
		dCreateInfo.depthWriteEnable = VK_TRUE;
		dCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		dCreateInfo.depthBoundsTestEnable = VK_FALSE;
		dCreateInfo.stencilTestEnable = VK_FALSE;

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

		VkDynamicState dynamStates[] = {};
		
		VkPipelineDynamicStateCreateInfo dynCreateInfo{};
		dynCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynCreateInfo.dynamicStateCount = 0;
		dynCreateInfo.pDynamicStates = dynamStates;
		
		VkPipelineLayoutCreateInfo pipeLayoutCreateInfo{}; // for descriptor sets
		pipeLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeLayoutCreateInfo.setLayoutCount = 1;
		pipeLayoutCreateInfo.pSetLayouts = &dSetLayout;

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
		pipeCreateInfo.pDepthStencilState = &dCreateInfo;
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
			
			// having a single depth buffer for every swap image only works if graphics and pres queues are the same.
			// this is due to submissions in a single queue having to respect both submission order and semaphores
			VkImageView attachments[] = {
				swapImageViews[i],
				depthView
			};

			VkFramebufferCreateInfo fCreateInfo{};
			fCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fCreateInfo.renderPass = renderPass;
			fCreateInfo.attachmentCount = 2;
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
	
	// find a memory type that our buffer can use and that has the properties we want
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProp{};
		vkGetPhysicalDeviceMemoryProperties(pdev, &memProp);

		// turn a one-hot typeFilter into an int representing the index we want in memoryTypes
		for (size_t i = 0; i < memProp.memoryTypeCount; i++) {
			// if the type matches one of the allowed types given to us and it has the right flags, return it
			if ((typeFilter & (1 << i)) && (memProp.memoryTypes[i].propertyFlags & properties)) {
				return i;
			}
		}

		throw std::runtime_error("cannot find proper memory type!");
	}

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkBuffer& buf, VkDeviceMemory& bufMem) {
		VkBufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = size;
		createInfo.usage = usage;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(dev, &createInfo, nullptr, &buf) != VK_SUCCESS) {
			throw std::runtime_error("cannot create buffer!");
		}

		VkMemoryRequirements mreq{};
		vkGetBufferMemoryRequirements(dev, buf, &mreq);

		uint32_t memType = findMemoryType(mreq.memoryTypeBits, props);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = mreq.size;
		allocInfo.memoryTypeIndex = memType;

		if (vkAllocateMemory(dev, &allocInfo, nullptr, &bufMem) != VK_SUCCESS) {
			throw std::runtime_error("cannot allocate buffer memory!");
		}

		vkBindBufferMemory(dev, buf, bufMem, 0);
	}

	VkCommandBuffer beginSingleCommand() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = cp;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer buf;

		vkAllocateCommandBuffers(dev, &allocInfo, &buf);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(buf, &beginInfo);

		return buf;
	}

	void endSingleCommand(VkCommandBuffer buf) {
		vkEndCommandBuffer(buf);

		VkSubmitInfo subInfo{};
		subInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		subInfo.commandBufferCount = 1;
		subInfo.pCommandBuffers = &buf;

		vkQueueSubmit(gQueue, 1, &subInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(gQueue);

		vkFreeCommandBuffers(dev, cp, 1, &buf);
	}

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldl, VkImageLayout newl) {
		VkCommandBuffer buf = beginSingleCommand();

		VkImageSubresourceRange range{};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldl;
		barrier.newLayout = newl;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange = range;

		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;
		
		if (oldl == VK_IMAGE_LAYOUT_UNDEFINED && newl == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // pseudo-stage that includes vkCopy and vkClear commands, among others

			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		} else if (oldl == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newl == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		} else {
			throw std::invalid_argument("illegal stage combination!");
		}

		vkCmdPipelineBarrier(buf, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		endSingleCommand(buf);
	}

	void copyBufferToImage(VkBuffer buf, VkImage img, uint32_t width, uint32_t height) {
		VkCommandBuffer cbuf = beginSingleCommand();

		VkImageSubresourceLayers rec{};
		rec.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		rec.mipLevel = 0;
		rec.baseArrayLayer = 0;
		rec.layerCount = 1;

		VkBufferImageCopy copy{};
		copy.bufferOffset = 0;
		copy.imageSubresource = rec;
		copy.imageOffset = {0, 0, 0};
		copy.imageExtent = {width, height, 1};

		vkCmdCopyBufferToImage(cbuf, buf, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

		endSingleCommand(cbuf);
	}

	void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
		VkCommandBuffer buf = beginSingleCommand();

		VkBufferCopy copy{};
		copy.size = size;

		vkCmdCopyBuffer(buf, src, dst, 1, &copy);

		endSingleCommand(buf);
	}

	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexMemory = VK_NULL_HANDLE;

	void createVertexBuffer(const std::vector<vload::vertex>& verts) {

		VkDeviceSize bufferSize = verts.size() * sizeof(vload::vertex);
		createBuffer(verts.size() * sizeof(vload::vertex), 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, stagingMemory);
		
		createBuffer(verts.size() * sizeof(vload::vertex), 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			vertexBuffer, vertexMemory);

		void *data;
		vkMapMemory(dev, stagingMemory, 0, bufferSize, 0, &data);
		memcpy(data, verts.data(), bufferSize);
		vkUnmapMemory(dev, stagingMemory);

		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkFreeMemory(dev, stagingMemory, nullptr);
		vkDestroyBuffer(dev, stagingBuffer, nullptr);
	}

	VkBuffer indexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory indexMemory = VK_NULL_HANDLE;

	void createIndexBuffer(const std::vector<uint32_t>& indices) {
		VkDeviceSize bufferSize = indices.size() * sizeof(uint32_t);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingMemory);

		createBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer, indexMemory);

		void *data;
		vkMapMemory(dev, stagingMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), bufferSize);
		vkUnmapMemory(dev, stagingMemory);

		copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkFreeMemory(dev, stagingMemory, nullptr);
		vkDestroyBuffer(dev, stagingBuffer, nullptr);
	}

	void createImage(unsigned int width, unsigned int height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags props, VkImage& image, VkDeviceMemory& imageMemory) {
		VkImageCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.format = format;
		createInfo.extent = {width, height, 1};
		createInfo.mipLevels = 1;
		createInfo.arrayLayers = 1;
		createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		createInfo.tiling = tiling;
		createInfo.usage = usage;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // discard existing texels when loading
		// initiallayout can only be ..._UNDEFINED or ..._PREINITIALIZED
		if (vkCreateImage(dev, &createInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("cannot create texture image!");
		}

		VkMemoryRequirements memReq;
		vkGetImageMemoryRequirements(dev, image, &memReq);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReq.size;
		allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, props);

		if (vkAllocateMemory(dev, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("cannot allocate texture memory!");
		}

		vkBindImageMemory(dev, image, imageMemory, 0);
	}

	VkImage texImage = VK_NULL_HANDLE;
	VkDeviceMemory texMem = VK_NULL_HANDLE;

	void createTextureImage(std::string path) {
		int width, height, chans;
		unsigned char *data = stbi_load(path.data(), &width, &height, &chans, STBI_rgb_alpha);
		if (!data) {
			throw std::runtime_error("cannot load texture!");
		} else {
			cout << "loaded texture " << path << endl;
		}
		
		VkDeviceSize imageSize = width * height * 4;

		VkBuffer sbuf = VK_NULL_HANDLE;
		VkDeviceMemory smem = VK_NULL_HANDLE;

		createBuffer(imageSize, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sbuf, smem);

		void *map_data;
		vkMapMemory(dev, smem, 0, imageSize, 0, &map_data);
		memcpy(map_data, data, imageSize);
		vkUnmapMemory(dev, smem);

		stbi_image_free(data);

		createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			texImage, texMem);
		
		transitionImageLayout(texImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(sbuf, texImage, uint32_t(width), uint32_t(height));
		transitionImageLayout(texImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkFreeMemory(dev, smem, nullptr);
		vkDestroyBuffer(dev, sbuf, nullptr);
	}

	VkImageView texView = VK_NULL_HANDLE;

	VkSampler texSamp = VK_NULL_HANDLE;

	void createSampler() {
		VkSamplerCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = VK_FILTER_LINEAR;
		createInfo.minFilter = VK_FILTER_LINEAR;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.mipLodBias = 0.0f;
		createInfo.anisotropyEnable = VK_TRUE;
		createInfo.maxAnisotropy = 16.0f;
		createInfo.compareEnable = VK_FALSE;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = 0.25f;

		if (vkCreateSampler(dev, &createInfo, nullptr, &texSamp) != VK_SUCCESS) {
			throw std::runtime_error("cannot create sampler!");
		}
	}

	VkImage depthImage = VK_NULL_HANDLE;
	VkDeviceMemory depthMemory = VK_NULL_HANDLE;
	VkImageView depthView = VK_NULL_HANDLE;

	void createDepthImage() {
		createImage(swapExtent.width, swapExtent.height, 
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			depthImage, depthMemory);

		depthView = createImageView(depthImage, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT);
	}
	
	std::vector<VkCommandBuffer> commandBuffers;
	
	// need to create a command buffer per swapchain image
	void allocCommandBuffers(uint32_t numIndices) {
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

			VkClearValue attachClearValues[2];
			attachClearValues[0].color = { 0.0, 0.0, 0.0, 1.0 };
			attachClearValues[1].depthStencil = {1.0, 0};
			
			rBeginInfo.clearValueCount = 2;
			rBeginInfo.pClearValues = attachClearValues;
			
			// actually render stuff!
			vkCmdBeginRenderPass(commandBuffers[i], &rBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, gpipe);

			VkBuffer buffers[] = { vertexBuffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, buffers, offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeLayout, 0, 1, &dSet[i], 0, nullptr);
			//vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(verts.size()), 1, 0, 0);
			vkCmdDrawIndexed(commandBuffers[i], numIndices, 1, 0, 0, 0);
			vkCmdEndRenderPass(commandBuffers[i]);
			
			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("cannot record into command buffer!");
			}
		}
	}

	constexpr static unsigned int framesInFlight = 2;

	// swapchain image acquisition requires a binary semaphore since it might be hard for implementations to do timeline semaphores
	std::vector<VkSemaphore> imageAvailSems; // use seperate semaphores per frame so we can send >1 frame at once
	std::vector<VkSemaphore> renderDoneSems;
	std::vector<VkFence> inFlightFences; // use fences so we actually wait until a frame completes before moving on to the next one
	std::vector<VkFence> imagesInFlight; // track frames in flight because acquireNextImageKHR may not return swapchain indices in order
	
	void createSyncs() {
		imageAvailSems.resize(framesInFlight, VK_NULL_HANDLE);
		renderDoneSems.resize(framesInFlight, VK_NULL_HANDLE);
		inFlightFences.resize(framesInFlight, VK_NULL_HANDLE);
		imagesInFlight = std::vector<VkFence>(swapImages.size(), VK_NULL_HANDLE); // this needs to be re-created on a window resize
		
		VkSemaphoreCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		
		VkFenceCreateInfo fCreateInfo{};
		fCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (unsigned int i = 0; i < framesInFlight; i++) {
			VkResult r1 = vkCreateSemaphore(dev, &createInfo, nullptr, &imageAvailSems[i]);
			VkResult r2 = vkCreateSemaphore(dev, &createInfo, nullptr, &renderDoneSems[i]);
			VkResult r3 = vkCreateFence(dev, &fCreateInfo, nullptr, &inFlightFences[i]);
			
			if (r1 != VK_SUCCESS || r2 != VK_SUCCESS || r3 != VK_SUCCESS) {
				throw std::runtime_error("cannot create sync objects!");
			}
		}
	}

	uint32_t numIndices = 0;

	void recreateSwapChain() {
		int width, height;
		glfwGetFramebufferSize(w, &width, &height);
		while (width == 0 || height == 0) { // wait until window isn't hidden anymore
			glfwGetFramebufferSize(w, &width, &height);
			glfwWaitEvents(); // put thread to sleep until events exist
		}
		
		vkDeviceWaitIdle(dev);

		cleanupSwapChain();

		createSwapChain();
		createSwapViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createUniformBuffers();
		createDescriptorPool();
		allocDescriptorSets();
		allocCommandBuffers(numIndices);
		createSyncs();
	}

	cam::camera c;

	void init() {
		createWindow();
		//glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		createInstance();
		if (verify) {
			setupDebugMessenger();
		}
		createSurface();
		pickPhysicalDevice(nvidia);
		createLogicalDevice();
		createSwapChain();
		createSwapViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createDepthImage();
		createFramebuffers();
		createCommandPool();
		vload::vloader v("models/cube.obj");
		createVertexBuffer(v.meshList[0].verts);
		createTextureImage("textures/grass/grass02 diffuse 1k.jpg");
		texView = createImageView(texImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
		createSampler();
		createIndexBuffer(v.meshList[0].indices);
		numIndices = v.meshList[0].indices.size();
		createUniformBuffers();
		createDescriptorPool();
		allocDescriptorSets();
		allocCommandBuffers(numIndices);
		createSyncs();
	}

	glm::vec3 cpos = glm::vec3(0.0, 0.0, -3.0);

	void updateUniformBuffer(uint32_t imageIndex) {
		using namespace std::chrono;
		static auto start = high_resolution_clock::now();
		auto current = high_resolution_clock::now();
		float time = duration<float, seconds::period>(current - start).count();

		ubo u1;
		// TODO: the .obj format assumes that +Y is up, not down.
		u1.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		u1.model = glm::rotate(u1.model, time * glm::radians(25.0f), glm::vec3(0.0, 0.0, 1.0));
		u1.view = glm::lookAt(c.pos, c.pos + c.front, glm::vec3(0.0f, 1.0f, 0.0f));
		u1.proj = glm::perspective(glm::radians(25.0f), swapExtent.width / float(swapExtent.height), 0.1f, 100.0f);
		
		// this line is required by the tutorial but breaks stuff?
		//u1.proj[1][1] *= -1.0;

		void* data;
		vkMapMemory(dev, uniformMemories[imageIndex], 0, sizeof(ubo), 0, &data);
		memcpy(data, &u1, sizeof(ubo));
		vkUnmapMemory(dev, uniformMemories[imageIndex]);
	}
	
	size_t currFrame = 0;

	void drawFrame() {

		// NOTE: acquiring an image, writing to it, and presenting it are all async operations.
		// The relevant vulkan calls return before the operation completes.
		
		// wait for the image using the fence to complete
		vkWaitForFences(dev, 1, &inFlightFences[currFrame], VK_TRUE, UINT64_MAX);

		uint32_t index;
		VkResult r = vkAcquireNextImageKHR(dev, swap, UINT64_MAX, imageAvailSems[currFrame], VK_NULL_HANDLE, &index);
		// NOTE: the value of index may jump around - there's no guarantee that it increases linearly
		
		if (r == VK_ERROR_OUT_OF_DATE_KHR || resizeOccurred) {
			recreateSwapChain(); // have to recreate the swapchain here
			resizeOccurred = false;
			return;
		} else if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) { // we can still technically run with a suboptimal swapchain
			throw std::runtime_error("cannot acquire swapchain image!");
		}

		// wait for the previous frame to finish using the swapchain image at index
		if (imagesInFlight[index] != VK_NULL_HANDLE) {
			vkWaitForFences(dev, 1, &imagesInFlight[index], VK_TRUE, UINT64_MAX);
		}

		imagesInFlight[index] = inFlightFences[currFrame]; // this frame is using the fence at currFrame

		updateUniformBuffer(index);

		VkSubmitInfo si{};
		si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		
		VkSemaphore renderBeginSems[] = { imageAvailSems[currFrame] };
		VkSemaphore renderEndSems[] = { renderDoneSems[currFrame] };

		si.waitSemaphoreCount = 1;
		si.pWaitSemaphores = renderBeginSems;

		// imageAvailSem waits at this point in the pipeline
		// NOTE: stages not covered by a semaphore may execute before the semaphore is signaled.
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		si.pWaitDstStageMask = waitStages;
		
		si.commandBufferCount = 1;
		si.pCommandBuffers = &commandBuffers[index];

		si.signalSemaphoreCount = 1;
		si.pSignalSemaphores = renderEndSems;
		
		vkResetFences(dev, 1, &inFlightFences[currFrame]); // has to be unsignaled for vkQueueSubmit
		vkQueueSubmit(gQueue, 1, &si, inFlightFences[currFrame]);

		VkPresentInfoKHR pInfo{};
		pInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		pInfo.waitSemaphoreCount = 1;
		pInfo.pWaitSemaphores = renderEndSems;
		pInfo.swapchainCount = 1;
		pInfo.pSwapchains = &swap;
		pInfo.pImageIndices = &index;
		
		r = vkQueuePresentKHR(gQueue, &pInfo);
		if (r == VK_ERROR_OUT_OF_DATE_KHR || resizeOccurred) {
			recreateSwapChain();
			resizeOccurred = false;
			return;
		} else if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("cannot submit to queue!");
		}
		// (or vkQueueWaitIdle)

		currFrame = (currFrame + 1) % framesInFlight;
	}

	void loop() {
		while (!glfwWindowShouldClose(w)) {
			glfwPollEvents();
			c.update(w);
			drawFrame();
			
			if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
				vkDeviceWaitIdle(dev);
				glfwSetWindowShouldClose(w, GLFW_TRUE);
			}
		}
	}
	
	void cleanupSwapChain() {

		for (unsigned int i = 0; i < framesInFlight; i++){
			vkDestroySemaphore(dev, imageAvailSems[i], nullptr);
			vkDestroySemaphore(dev, renderDoneSems[i], nullptr);
			vkDestroyFence(dev, inFlightFences[i], nullptr);
		}

		vkFreeCommandBuffers(dev, cp, commandBuffers.size(), commandBuffers.data());

		for (size_t i = 0; i < swapImages.size(); i++) {
			vkFreeMemory(dev, uniformMemories[i], nullptr);
			vkDestroyBuffer(dev, uniformBuffers[i], nullptr);
		}

		vkDestroyDescriptorPool(dev, dPool, nullptr);

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
	}

	void cleanup() {

		cleanupSwapChain();

		vkDestroyDescriptorSetLayout(dev, dSetLayout, nullptr);

		vkDestroyCommandPool(dev, cp, nullptr);

		vkDestroyImageView(dev, depthView, nullptr);
		vkFreeMemory(dev, depthMemory, nullptr);
		vkDestroyImage(dev, depthImage, nullptr);

		vkDestroySampler(dev, texSamp, nullptr);
		vkDestroyImageView(dev, texView, nullptr);
		vkFreeMemory(dev, texMem, nullptr);
		vkDestroyImage(dev, texImage, nullptr);

		vkFreeMemory(dev, indexMemory, nullptr);
		vkDestroyBuffer(dev, indexBuffer, nullptr);

		vkFreeMemory(dev, vertexMemory, nullptr);
		vkDestroyBuffer(dev, vertexBuffer, nullptr);

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
	return 0;
}
