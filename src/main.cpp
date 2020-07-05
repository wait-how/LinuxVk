//#include "main.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <cstring> // for strcmp
#include <optional> // C++17, for device queue querying

constexpr unsigned int screenWidth = 3840;
constexpr unsigned int screenHeight = 2160;

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
		w = glfwCreateWindow(screenWidth, screenHeight, "Demo", nullptr, nullptr); // test for creation failure
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
			// add debug extension in case we need it later, but it's printed to stdout by default.
			// use if we want to control debug info or redirect it to stdout or something.
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
		
		cerr << " ~ " << data->pMessage << std::endl;
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
		// verbose messages generate too much information
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
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
		
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
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

		if (verbose) {
			// print global extensions
			uint32_t numExtensions = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, nullptr);
			std::vector<VkExtensionProperties> extensions(numExtensions);
			vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, extensions.data());
			
			cout << numExtensions << " extensions available:" << endl;
			for (const auto& extension : extensions) {
				cout << "\t" << extension.extensionName << endl;
			}
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

	void pickPhysicalDevice() {
		uint32_t numDevices;
		vkEnumeratePhysicalDevices(instance, &numDevices, nullptr);
		if (numDevices == 0) {
			throw std::runtime_error("no vulkan-capable gpu found!");
		}
		std::vector<VkPhysicalDevice> devices(numDevices);
		vkEnumeratePhysicalDevices(instance, &numDevices, devices.data());
		
		for (const auto& device : devices) {
			if (checkDevice(device, false)) {
				pdev = device;
				break;
			}
		}

		if (pdev == VK_NULL_HANDLE) {
			throw std::runtime_error("no usable graphics device found!");
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
			if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
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
	
	VkDevice dev = VK_NULL_HANDLE;
	VkQueue gQueue = VK_NULL_HANDLE;

	void createLogicalDevice() {
		queueAvailable qa = findQueueFamily(pdev);
		
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
		
		// enabledLayerCount and ppEnabledLayerNames are no longer used by drivers in vk 1.1
		
		if (vkCreateDevice(pdev, &createInfo, nullptr, &dev)) {
			throw std::runtime_error("cannot create virtual device!");
		}

		vkGetDeviceQueue(dev, *(qa.graphics), 0, &gQueue);
	}
	
	void init() {
		initwindow();
		createInstance();
		if (verify) {
			setupDebugMessenger();
		}
		pickPhysicalDevice();
		createLogicalDevice();
	}

	void loop() {
		while (!glfwWindowShouldClose(w)) {
			glfwPollEvents();
			if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
				glfwSetWindowShouldClose(w, GLFW_TRUE);
			}

			// render stuff!

		}
	}

	void cleanup() {
		
		if (verify) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroyDevice(dev, nullptr);
		
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
