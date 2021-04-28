#include <cstring> // for strcmp
#include <set>

#include "options.hpp"
#include "extensions.hpp"

#include "main.hpp"

// extension support is device-specific, so check for it here
bool appvk::checkDeviceExtensions(VkPhysicalDevice pdev) {
    uint32_t numExtensions;
    vkEnumerateDeviceExtensionProperties(pdev, nullptr, &numExtensions, nullptr);
    std::vector<VkExtensionProperties> deviceExtensions(numExtensions);
    vkEnumerateDeviceExtensionProperties(pdev, nullptr, &numExtensions, deviceExtensions.data());
    
    std::set<std::string_view> tempExtensionList(requiredExtensions.begin(), requiredExtensions.end());
    
    // erase any extensions found
    for (const auto& extension : deviceExtensions) {
        tempExtensionList.erase(extension.extensionName);
    }

    return tempExtensionList.empty();
}

VkSampleCountFlagBits appvk::getSamples(unsigned int try_samples) {
    VkPhysicalDeviceProperties dprop;
    vkGetPhysicalDeviceProperties(pdev, &dprop);
    
    // find max sample count that both color and depth buffer support and return it
    VkSampleCountFlags count = dprop.limits.framebufferColorSampleCounts & dprop.limits.framebufferDepthSampleCounts;
    if (count & VK_SAMPLE_COUNT_16_BIT && try_samples == 16) { return VK_SAMPLE_COUNT_16_BIT; }
    if (count & VK_SAMPLE_COUNT_8_BIT && try_samples == 8) { return VK_SAMPLE_COUNT_8_BIT; }
    if (count & VK_SAMPLE_COUNT_4_BIT && try_samples == 4) { return VK_SAMPLE_COUNT_4_BIT; }
    if (count & VK_SAMPLE_COUNT_2_BIT && try_samples == 2) { return VK_SAMPLE_COUNT_2_BIT; }
    
    return VK_SAMPLE_COUNT_1_BIT;
}

void appvk::checkChooseDevice(VkPhysicalDevice pd, manufacturer m) {
    VkPhysicalDeviceProperties dprop{}; // basic information
    VkPhysicalDeviceFeatures dfeat{}; // detailed feature list
    
    vkGetPhysicalDeviceProperties(pd, &dprop);
    vkGetPhysicalDeviceFeatures(pd, &dfeat);

    const std::string_view name = std::string_view(dprop.deviceName);
    cout << "found \"" << name << "\" running " << 
        VK_VERSION_MAJOR(dprop.apiVersion) <<
        "." << VK_VERSION_MINOR(dprop.apiVersion) <<
        "." << VK_VERSION_PATCH(dprop.apiVersion);
    
    VkBool32 correctm;
    switch (m) {
        case nvidia:
            correctm = name.find("GeForce") != std::string_view::npos;
            break;
        case intel:
            correctm = name.find("Intel") != std::string_view::npos;
            break;
        case any:
            correctm = VK_TRUE;
            break;
    }

    VkBool32 correctf = dfeat.geometryShader |
        dfeat.tessellationShader |
        checkDeviceExtensions(pd);
    
    if (correctm && correctf && pdev == VK_NULL_HANDLE) {
        pdev = pd;
        msaaSamples = getSamples(options::msaaSamples);
        cout << " (selected)";
    }

    cout << "\n";
}

void appvk::pickPhysicalDevice(manufacturer m) {
    uint32_t numDevices;
    vkEnumeratePhysicalDevices(instance, &numDevices, nullptr);
    if (numDevices == 0) {
        throw std::runtime_error("no vulkan-capable gpu found!");
    }
    std::vector<VkPhysicalDevice> devices(numDevices);
    vkEnumeratePhysicalDevices(instance, &numDevices, devices.data());
    
    for (const auto& device : devices) {
        checkChooseDevice(device, m);
    }

    if (pdev == VK_NULL_HANDLE) {
        throw std::runtime_error("no usable gpu found!");
    }
}

appvk::queueIndices appvk::findQueueFamily(VkPhysicalDevice pd) {
    queueIndices qi;
    uint32_t numQueues;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &numQueues, nullptr);
    std::vector<VkQueueFamilyProperties> queues(numQueues);
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &numQueues, queues.data());
    
    for (size_t i = 0; i < numQueues; i++) {
        VkBool32 presSupported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(pdev, i, surf, &presSupported);
        
        if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presSupported) {
            qi.graphics = i;
        }
        if (queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            qi.compute = i;
        }
        if (queues[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            qi.transfer = i;
        }
    }
    
    return qi;
}

void appvk::createLogicalDevice() {
    queueIndices qi = findQueueFamily(pdev); // check for the proper queue
    swapChainSupportDetails d = querySwapChainSupport(pdev); // verify swap chain information before creating a new logical device

    if (!qi.graphics.has_value() || d.formats.size() == 0 || d.presentModes.size() == 0) {
        throw std::runtime_error("cannot find a suitable logical device!");
    }

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = *(qi.graphics);
    queueInfo.queueCount = 1;
    float pri = 1.0f;
    queueInfo.pQueuePriorities = &pri; // highest priority

    VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR execProp{};
    execProp.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR;
    execProp.pipelineExecutableInfo = VK_TRUE;

    // this structure is the same as deviceFeatures but has a pNext member too
    VkPhysicalDeviceFeatures2 feat2{};
    feat2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    feat2.pNext = &execProp;
    feat2.features = {}; // set everything not used to zero
    feat2.features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &feat2;
    createInfo.pQueueCreateInfos = &queueInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = nullptr;
    createInfo.enabledExtensionCount = requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();
            
    if (vkCreateDevice(pdev, &createInfo, nullptr, &dev)) {
        throw std::runtime_error("cannot create virtual device!");
    }

    vkGetDeviceQueue(dev, *(qi.graphics), 0, &gQueue); // creating a device also creates queues for it
}
