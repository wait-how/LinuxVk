#pragma once

#include "extensions.hpp"
#include "glfw_wrapper.hpp"

#include <array>
#include <vector>

// Base class that initializes a window and creates a context.
// Validation layers are turned on if options::debug is set to true.
class basevk {
protected:
	
	GLFWwindow* w;
	VkSurfaceKHR surf = VK_NULL_HANDLE;
    VkInstance instance = VK_NULL_HANDLE;

    bool resizeOccurred = false;

    basevk(bool fullscreen);
    ~basevk();
    
private:
    constexpr static std::array<const char*, 1> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    void createWindow(bool fullscreen);

    static void windowSizeCallback(GLFWwindow* w, int width, int height);

    void checkValidation();
    const std::vector<const char*> getExtensions();

    static VKAPI_ATTR VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT mSev,
        VkDebugUtilsMessageTypeFlagsEXT mType,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* userData);
    
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    void populateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    
    void createInstance();
};