#include "base.hpp"

#include "options.hpp"

#include <iostream>
#include <stdexcept>
#include <cstring> // for strcmp

using std::cout;
using std::cerr;

basevk::basevk(bool fullscreen) {
    createWindow(fullscreen);

    createInstance();
	if (options::debug) {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessenger(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("cannot create debug messenger!");
        }
	}
}

basevk::~basevk() {
    if (options::debug) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(w);
    glfwTerminate();
}

void basevk::windowSizeCallback(GLFWwindow* w, int width, int height) {
	(void)width;
    (void)height;

    basevk* papp = reinterpret_cast<basevk*>(glfwGetWindowUserPointer(w));
    papp->resizeOccurred = true;
}

void basevk::createWindow(bool fullscreen) {
    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (!glfwVulkanSupported()) {
        throw std::runtime_error("vulkan not supported!");
    }

	if (fullscreen) {
    	w = glfwCreateWindow(options::screenWidth, options::screenHeight, "demo", glfwGetPrimaryMonitor(), nullptr);
	} else {
    	w = glfwCreateWindow(options::screenWidth, options::screenHeight, "demo", nullptr, nullptr);
	}

    if (!w) {
        throw std::runtime_error("cannot create window!");
    }

    // this enables us to read/write class members from our static callback
    // (since static members have no implicit "*this" parameter)
    glfwSetWindowUserPointer(w, this);
    glfwSetFramebufferSizeCallback(w, windowSizeCallback);
}

void basevk::checkValidation() {
    if (options::debug) {
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
                throw std::runtime_error("cannot find all validation layers!");
            }
        }
        cout << "found validation layers\n";
    }
}

const std::vector<const char*> basevk::getExtensions() {
    // glfw helper function that specifies the extension needed to draw stuff
    uint32_t glfwNumExtensions = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwNumExtensions);
    
    // default insertion constructor
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwNumExtensions);
    if (options::debug) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    }

    return extensions;
}

VKAPI_ATTR VkBool32 basevk::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT mSev,
    VkDebugUtilsMessageTypeFlagsEXT mType,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* userData) {
    
    (void) mSev;
    (void) mType;
    (void) data;
    (void) userData;
    
    cerr << "\t" << data->pMessage << "\n";
    return VK_FALSE; // don't abort on error
}

void basevk::populateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    
    if (options::verbose) {
        createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    }

    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
}

void basevk::createInstance() {
    if (options::debug) {
        checkValidation();
    }
    
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "demo";
    appInfo.apiVersion = VK_API_VERSION_1_2;
    
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{}; // outside if statement to avoid getting deallocated early

    
    VkValidationFeaturesEXT validFeatures{};
    std::array<VkValidationFeatureEnableEXT, 2> feats = {
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT, // enable sync validation to detect missing/incorrect barriers between operations
        // VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT // enable warnings for potential performance problems
    };

    if (options::debug) {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
        
        populateDebugMessenger(debugCreateInfo);

        validFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        validFeatures.enabledValidationFeatureCount = feats.size();
        validFeatures.pEnabledValidationFeatures = feats.data();

        // passing debugCreateInfo here so that our debug utils handle errors in createInstance or destroyInstance
        createInfo.pNext = &debugCreateInfo;

        debugCreateInfo.pNext = &validFeatures;
    }
    
    auto extensions = getExtensions();

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("instance creation failed!");
    }
}